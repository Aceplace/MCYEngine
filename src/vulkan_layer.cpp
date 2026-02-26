#define WIN32_MEAN_AND_LEAN
#include <windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include "mcy_helpers.h"
#include <glm/glm.hpp>

struct Vertex
{
    glm::vec2 pos;
    glm::vec3 color;

};

const Vertex vertices[] = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const u16 indices[] = {
    0, 1, 2, 2, 3, 0
};

VkVertexInputBindingDescription VertexGetBindingDescription()
{
    VkVertexInputBindingDescription result = {};
    result.binding = 0;
    result.stride = sizeof(Vertex);
    result.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return result;
}

VkVertexInputAttributeDescription* VertexGetInputAttributeDescription()
{
    VkVertexInputAttributeDescription* result = new VkVertexInputAttributeDescription[2];

    result[0] = {};
    result[0].location = 0;
    result[0].binding = 0;
    result[0].format = VK_FORMAT_R32G32_SFLOAT;
    result[0].offset = offsetof(Vertex, pos);
    
    result[1] = {};
    result[1].location = 1;
    result[1].binding = 0;
    result[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    result[1].offset = offsetof(Vertex, color);
    
    return result;
}

const u32 MAX_FRAMES_IN_FLIGHT = 2;

HWND _hwnd = NULL;
struct VKMState
{
    VkInstance vkInstance;
    VkDebugUtilsMessengerEXT vKDebugUtilsMessengerExt;
    PFN_vkDestroyDebugUtilsMessengerEXT pfnDestroyDebugUtilsMessengerEXT;
    char* shaderFileBytes;
    VkPhysicalDevice physicalDevice;
    
    VkDevice vkDevice;
    VkSurfaceCapabilitiesKHR surfaceCapabilities; 
    VkSurfaceKHR surface;
    VkSurfaceFormatKHR surfaceFormat;
    VkExtent2D extent;
    
    VkSwapchainKHR swapChain;
    VkFormat swapChainImageFormat;
    u32 swapChainImagesCount = 0;
    u32 imageCount = 0;
    VkImage* swapChainImages;
    VkImageView* swapChainImageViews;
    VkExtent2D swapChainExtent;
    VkPresentModeKHR presentMode;
    bool framebufferResized = false;
    
    VkPipeline graphicsPipeline;
    VkShaderModule shaderModule;
    VkBuffer vertexBuffer;
    
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffers[MAX_FRAMES_IN_FLIGHT];
    s32 graphicsQueueIndex = -1;
    s32 presentQueueIndex = -1; 
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkPipelineLayout pipelineLayout;

    VkSemaphore acquireSemaphores[MAX_FRAMES_IN_FLIGHT];
    VkFence frameFences[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore* submitSemaphores;
    u32 currentFrameInFlightIndex;
    VkBuffer indexBuffer;
    VkBuffer indexBufferBuffer;
};
VKMState vkm = {};

void VkmCopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
s32 VkmFindMemoryTypeIndex(uint32_t typeFilter, VkMemoryPropertyFlags memoryPropertyFlags);
bool VkmCreateAndFillBuffer(VkDeviceSize size, void* data, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkBuffer* buffer, VkDeviceMemory* bufferMemory);
bool VkmCreateBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkBuffer* buffer, VkDeviceMemory* bufferMemory);
bool VkmRecreateSwapChain();
bool VkMVulkanInitialize();

static VKAPI_ATTR VkBool32 VKAPI_CALL VkmDebgCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT   messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT          messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void*                                    pUserData)
{
    char messageBuffer[512];
    sprintf_s(messageBuffer, 512, "Vulkan validation layer: %s\n", pCallbackData->pMessage);
    OutputDebugString(messageBuffer);
    
    return VK_FALSE;
}

bool VkmInitialize()
{
    vkm = {};
    vkm.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    vkm.graphicsQueueIndex = -1;
    vkm.presentQueueIndex = -1;

    VkResult vkResult;
    VkApplicationInfo vkApplicationInfo;
    vkApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    vkApplicationInfo.pNext = nullptr;
    vkApplicationInfo.pApplicationName = "mcy_engine";
    vkApplicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    vkApplicationInfo.pEngineName = nullptr;
    vkApplicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    vkApplicationInfo.apiVersion = VK_API_VERSION_1_4;

    const char* instanceExtensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    };

    static const char* validationLayers[] = {
        "VK_LAYER_KHRONOS_validation"
    };
    const uint32_t validationLayerCount = ARRAY_COUNT(validationLayers);

    uint32_t availableLayerCount = 0;
    vkEnumerateInstanceLayerProperties(&availableLayerCount, NULL);
    VkLayerProperties* availableLayers = new VkLayerProperties[availableLayerCount];
    vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers);

    bool foundAllLayers = true;
    for (u32 i = 0; i < validationLayerCount; i++)
    {
        const char* layerName = validationLayers[i];
        bool foundLayer = false;
        for (u32 j = 0; j < availableLayerCount; j++)
        {
            if (strcmp(availableLayers[j].layerName, layerName) == 0)
            {
                foundLayer = true;
                break;
            }
        }

        if (!foundLayer)
                foundAllLayers = false;
    }

    if (!foundAllLayers)
    {
        OutputDebugString("Did not find all validation layers.");
        return false;
    }
    OutputDebugString("Found all validation layers.");    

    VkInstanceCreateInfo vkInstanceCreateInfo;
    vkInstanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    vkInstanceCreateInfo.pNext = nullptr;
    vkInstanceCreateInfo.flags = 0;
    vkInstanceCreateInfo.pApplicationInfo = &vkApplicationInfo;
    vkInstanceCreateInfo.enabledLayerCount = validationLayerCount;
    vkInstanceCreateInfo.ppEnabledLayerNames = validationLayers;
    vkInstanceCreateInfo.enabledExtensionCount = ARRAY_COUNT(instanceExtensions);
    vkInstanceCreateInfo.ppEnabledExtensionNames = instanceExtensions;

    vkResult = vkCreateInstance(&vkInstanceCreateInfo, nullptr, &vkm.vkInstance);

    if (vkResult != VK_SUCCESS)
    {
        OutputDebugString("Could not create Vulkan Instance\n");
        return false;
    }
    OutputDebugString("Created Vulkan Instance\n");

    VkWin32SurfaceCreateInfoKHR win32SurfaceCreateInfo{};
    win32SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    win32SurfaceCreateInfo.pNext = nullptr;
    win32SurfaceCreateInfo.flags = 0;
    win32SurfaceCreateInfo.hwnd = _hwnd;
    win32SurfaceCreateInfo.hinstance = GetModuleHandle(nullptr);

    vkResult = vkCreateWin32SurfaceKHR(vkm.vkInstance, &win32SurfaceCreateInfo, nullptr, &vkm.surface);
    if (vkResult != VK_SUCCESS)
    {
        OutputDebugString("Could not create win32 surface\n");
        return false;
    }
    
    PFN_vkCreateDebugUtilsMessengerEXT pfnCreateDebugUtilsMessengerEXT = nullptr;
    pfnCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vkm.vkInstance, "vkCreateDebugUtilsMessengerEXT");    
    if (!pfnCreateDebugUtilsMessengerEXT)
    {
        OutputDebugString("Could not get function vkCreateDebugUtilsMessengerEXT\n");
        return false;
    }
    vkm.pfnDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vkm.vkInstance, "vkDestroyDebugUtilsMessengerEXT");
    if (!vkm.pfnDestroyDebugUtilsMessengerEXT)
    {
        OutputDebugString("Could not get function vkDestroyDebugUtilsMessengerEXT\n");
        return false;
    }

    VkDebugUtilsMessengerCreateInfoEXT vkDebugUtilsMessengerCreateInfoEXT;
    vkDebugUtilsMessengerCreateInfoEXT.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    vkDebugUtilsMessengerCreateInfoEXT.pNext = nullptr;
    vkDebugUtilsMessengerCreateInfoEXT.flags = 0;
    vkDebugUtilsMessengerCreateInfoEXT.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | 
                                                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    vkDebugUtilsMessengerCreateInfoEXT.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
                                                     VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    vkDebugUtilsMessengerCreateInfoEXT.pfnUserCallback = VkmDebgCallback;
    vkDebugUtilsMessengerCreateInfoEXT.pUserData = nullptr;
    vkResult = pfnCreateDebugUtilsMessengerEXT(vkm.vkInstance, &vkDebugUtilsMessengerCreateInfoEXT, nullptr, &vkm.vKDebugUtilsMessengerExt);

    if (vkResult != VK_SUCCESS)
    {
        OutputDebugString("Could not create debug util messenger\n");
        return false;
    }
    OutputDebugString("Created debug util messenger\n");

    u32 vkPhysicalDeviceCount;
    vkEnumeratePhysicalDevices(vkm.vkInstance, &vkPhysicalDeviceCount, nullptr);
    
    if (vkPhysicalDeviceCount == 0)
    {
        OutputDebugString("Could not find any physical devices\n");
        return false;
    }
    
    VkPhysicalDevice* vkPhysicalDevices = new VkPhysicalDevice[vkPhysicalDeviceCount];
    vkResult = vkEnumeratePhysicalDevices(vkm.vkInstance, &vkPhysicalDeviceCount, vkPhysicalDevices);

    if (vkResult != VK_SUCCESS)
    {
        OutputDebugString("Could not enumerate physical devices\n");
        return false;
    }

    const char* deviceExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    for (u32 i = 0; i < vkPhysicalDeviceCount; i++)
    {
        VkPhysicalDevice vkPhysicalDevice = vkPhysicalDevices[i];
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceProperties(vkPhysicalDevice, &deviceProperties);
        vkGetPhysicalDeviceFeatures(vkPhysicalDevice, &deviceFeatures);

        if (deviceProperties.apiVersion < VK_API_VERSION_1_3)
            continue;
        
        u32 queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyCount, NULL);

        if (queueFamilyCount < 1)
            continue;

        VkQueueFamilyProperties* queueFamilies = new VkQueueFamilyProperties[queueFamilyCount];
        vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyCount, queueFamilies);

        bool hasGraphics = false;
        for (u32 i = 0; i < queueFamilyCount; i++)
        {
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) 
            {
                hasGraphics = true;
                break;
            }
        }

        delete[] queueFamilies;
        if (!hasGraphics)
            continue;

        uint32_t extCount = 0;
        vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, NULL, &extCount, NULL);
        VkExtensionProperties* extensions = new VkExtensionProperties[extCount];
        vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, NULL, &extCount, extensions);

        bool foundAllExtensions = true;
        for (u32 i = 0; i < ARRAY_COUNT(deviceExtensions); i++)
        {
            bool found = false;
            for (u32 j = 0; j < extCount; j++)
            {
                if (strcmp(deviceExtensions[i], extensions[j].extensionName) == 0) 
                {
                    found = true;
                    break;
                }
            }

            if (!found) 
            {
                foundAllExtensions = false;
                break;
            }
        }

        delete[] extensions;

        if (!foundAllExtensions)
            continue;

        vkm.physicalDevice = vkPhysicalDevice;
        break;
    }

    delete[] vkPhysicalDevices;

    if (vkm.physicalDevice == nullptr)
    {
        OutputDebugString("Could not find sutiable physical devices\n");
        return false;
    }

    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vkm.physicalDevice, &queueFamilyCount, NULL);    
    VkQueueFamilyProperties* queueFamilies = new VkQueueFamilyProperties[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(vkm.physicalDevice, &queueFamilyCount, queueFamilies);

    for (s32 i = 0; i < queueFamilyCount; i++)
    {
        bool hasGraphics = (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(vkm.physicalDevice, i, vkm.surface, &presentSupport);

        if (hasGraphics && vkm.graphicsQueueIndex == -1)
            vkm.graphicsQueueIndex = i;
        if (presentSupport == VK_TRUE && vkm.presentQueueIndex == -1)
            vkm.presentQueueIndex = i;
        if (hasGraphics && presentSupport == VK_TRUE)
        {
            vkm.graphicsQueueIndex = i;
            vkm.presentQueueIndex = i;
            break;
        }
    }
    delete[] queueFamilies;

    if (vkm.graphicsQueueIndex == -1)
    {
        OutputDebugString("Could not find graphics queue index\n");
        return false;
    }
    if (vkm.presentQueueIndex == -1)
    {
        OutputDebugString("Could not find present queue index\n");
        return false;
    }

    float queuePriority = 0.5;
    VkDeviceQueueCreateInfo graphicsQueueCreateInfo;
    graphicsQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    graphicsQueueCreateInfo.pNext = nullptr;
    graphicsQueueCreateInfo.flags = 0;
    graphicsQueueCreateInfo.queueFamilyIndex = vkm.graphicsQueueIndex;
    graphicsQueueCreateInfo.queueCount = 1;
    graphicsQueueCreateInfo.pQueuePriorities = &queuePriority;

    VkDeviceQueueCreateInfo queueCreateInfos[2];
    uint32_t queueCreateInfoCount = 0;
    if (vkm.graphicsQueueIndex == vkm.presentQueueIndex) 
    {
        queueCreateInfos[0] = graphicsQueueCreateInfo;
        queueCreateInfoCount = 1;
    } 
    else 
    {
        queueCreateInfos[0] = graphicsQueueCreateInfo;
        VkDeviceQueueCreateInfo presentQueueCreateInfo = {};
        presentQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        presentQueueCreateInfo.pNext = nullptr;
        presentQueueCreateInfo.flags = 0;
        presentQueueCreateInfo.queueFamilyIndex = vkm.presentQueueIndex;
        presentQueueCreateInfo.queueCount = 1;
        presentQueueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos[1] = presentQueueCreateInfo;
        queueCreateInfoCount = 2;
    }

    VkPhysicalDeviceFeatures2 vkPhysicalDeviceFeatures2;
    vkPhysicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    vkPhysicalDeviceFeatures2.pNext = nullptr;
    vkPhysicalDeviceFeatures2.features = {};

    VkPhysicalDeviceVulkan11Features vkPhysicalDeviceVulkan11Features = {};
    vkPhysicalDeviceVulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    vkPhysicalDeviceVulkan11Features.pNext = nullptr;
    vkPhysicalDeviceVulkan11Features.storageBuffer16BitAccess = false;
    vkPhysicalDeviceVulkan11Features.uniformAndStorageBuffer16BitAccess = false;
    vkPhysicalDeviceVulkan11Features.storagePushConstant16 = false;
    vkPhysicalDeviceVulkan11Features.storageInputOutput16 = false;
    vkPhysicalDeviceVulkan11Features.multiview = false;
    vkPhysicalDeviceVulkan11Features.multiviewGeometryShader = false;
    vkPhysicalDeviceVulkan11Features.multiviewTessellationShader = false;
    vkPhysicalDeviceVulkan11Features.variablePointersStorageBuffer = false;
    vkPhysicalDeviceVulkan11Features.variablePointers = false;
    vkPhysicalDeviceVulkan11Features.protectedMemory = false;
    vkPhysicalDeviceVulkan11Features.samplerYcbcrConversion = false;
    vkPhysicalDeviceVulkan11Features.shaderDrawParameters = true;

    VkPhysicalDeviceVulkan13Features vkPhysicalDeviceVulkan13Features = {};
    vkPhysicalDeviceVulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    vkPhysicalDeviceVulkan13Features.pNext = nullptr;
    vkPhysicalDeviceVulkan13Features.robustImageAccess = false;
    vkPhysicalDeviceVulkan13Features.inlineUniformBlock = false;
    vkPhysicalDeviceVulkan13Features.descriptorBindingInlineUniformBlockUpdateAfterBind = false;
    vkPhysicalDeviceVulkan13Features.pipelineCreationCacheControl = false;
    vkPhysicalDeviceVulkan13Features.privateData = false;
    vkPhysicalDeviceVulkan13Features.shaderDemoteToHelperInvocation = false;
    vkPhysicalDeviceVulkan13Features.shaderTerminateInvocation = false;
    vkPhysicalDeviceVulkan13Features.subgroupSizeControl = false;
    vkPhysicalDeviceVulkan13Features.computeFullSubgroups = false;
    vkPhysicalDeviceVulkan13Features.synchronization2 = true;
    vkPhysicalDeviceVulkan13Features.textureCompressionASTC_HDR = false;
    vkPhysicalDeviceVulkan13Features.shaderZeroInitializeWorkgroupMemory = false;
    vkPhysicalDeviceVulkan13Features.dynamicRendering = true;
    vkPhysicalDeviceVulkan13Features.shaderIntegerDotProduct = false;
    vkPhysicalDeviceVulkan13Features.maintenance4 = false;

    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT vkPhysicalDeviceExtendedDynamicStateFeaturesEXT;
    vkPhysicalDeviceExtendedDynamicStateFeaturesEXT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
    vkPhysicalDeviceExtendedDynamicStateFeaturesEXT.pNext = nullptr;
    vkPhysicalDeviceExtendedDynamicStateFeaturesEXT.extendedDynamicState = true;

    vkPhysicalDeviceFeatures2.pNext = &vkPhysicalDeviceVulkan11Features;
    vkPhysicalDeviceVulkan11Features.pNext = &vkPhysicalDeviceVulkan13Features;
    vkPhysicalDeviceVulkan13Features.pNext = &vkPhysicalDeviceExtendedDynamicStateFeaturesEXT;

    VkDeviceCreateInfo vkDeviceCreateInfo;
    vkDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    vkDeviceCreateInfo.pNext = &vkPhysicalDeviceFeatures2;
    vkDeviceCreateInfo.flags = 0;
    vkDeviceCreateInfo.queueCreateInfoCount = queueCreateInfoCount;
    vkDeviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
    vkDeviceCreateInfo.enabledLayerCount = 0;
    vkDeviceCreateInfo.ppEnabledLayerNames = nullptr;
    vkDeviceCreateInfo.enabledExtensionCount = ARRAY_COUNT(deviceExtensions);
    vkDeviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;
    vkDeviceCreateInfo.pEnabledFeatures = nullptr;
    vkResult = vkCreateDevice(vkm.physicalDevice, &vkDeviceCreateInfo, nullptr, &vkm.vkDevice);

    if (vkResult != VK_SUCCESS)
    {
        OutputDebugString("Could not create logical device\n");
        return false;
    }

    vkGetDeviceQueue(vkm.vkDevice, vkm.graphicsQueueIndex, 0, &vkm.graphicsQueue);
    vkGetDeviceQueue(vkm.vkDevice, vkm.presentQueueIndex, 0, &vkm.presentQueue);

    if (!vkm.graphicsQueue)
    {
        OutputDebugString("Could not get graphics queue\n");
        return false;
    }
    if (!vkm.presentQueue)
    {
        OutputDebugString("Could not get present queue\n");
        return false;
    }

    u32 surfaceFormatCount = 0;
    vkResult = vkGetPhysicalDeviceSurfaceFormatsKHR(vkm.physicalDevice, vkm.surface, &surfaceFormatCount, nullptr);
    if (vkResult != VK_SUCCESS || surfaceFormatCount == 0)
    {
        OutputDebugString("Could not get surface formats\n");
        return false;
    }
    VkSurfaceFormatKHR* surfaceFormats = new VkSurfaceFormatKHR[surfaceFormatCount];
    vkResult = vkGetPhysicalDeviceSurfaceFormatsKHR(vkm.physicalDevice, vkm.surface, &surfaceFormatCount, surfaceFormats);

    bool foundGoodSurfaceFormat = false;
    for (u32 i = 0; i < surfaceFormatCount; i++)
    {
        VkSurfaceFormatKHR* currentFormat = &surfaceFormats[i];
        if (currentFormat->format ==VK_FORMAT_R8G8B8A8_SRGB && currentFormat->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            foundGoodSurfaceFormat = true;
            vkm.surfaceFormat = *currentFormat;
            break;
        }
    }

    if (!foundGoodSurfaceFormat)
        vkm.surfaceFormat = surfaceFormats[0];

    
    if (!VkmRecreateSwapChain()) 
    {
        OutputDebugString("Failed to create swap chain\n");
        return false;
    }

    st shaderFileSize = 0;
    bool nVkResult = ReadEntireFile("slang.spv", &vkm.shaderFileBytes, &shaderFileSize);
    if (!nVkResult)
    {
        OutputDebugString("Failed to read shader file\n");
        return false;
    }

    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.pNext = nullptr;
    shaderModuleCreateInfo.flags = 0;
    shaderModuleCreateInfo.codeSize = shaderFileSize;
    shaderModuleCreateInfo.pCode = (u32*)vkm.shaderFileBytes;

    vkResult = vkCreateShaderModule(vkm.vkDevice, &shaderModuleCreateInfo, nullptr, &vkm.shaderModule);
    if (vkResult != VK_SUCCESS)
    {
        OutputDebugString("Failed to create shader module\n");
        return false;
    }

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.pNext = nullptr;
    vertShaderStageInfo.flags = 0;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vkm.shaderModule;
    vertShaderStageInfo.pName = "vertMain";
    vertShaderStageInfo.pSpecializationInfo = nullptr;

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.pNext = nullptr;
    fragShaderStageInfo.flags = 0;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = vkm.shaderModule;
    fragShaderStageInfo.pName = "fragMain";
    fragShaderStageInfo.pSpecializationInfo = nullptr;

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        vertShaderStageInfo, fragShaderStageInfo
    };

    VkVertexInputBindingDescription bindingDescription = VertexGetBindingDescription();
    VkVertexInputAttributeDescription* attributeDescriptions = VertexGetInputAttributeDescription();

    VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = {};
    pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipelineVertexInputStateCreateInfo.pNext = nullptr;
    pipelineVertexInputStateCreateInfo.flags = 0;
    pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
    pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = &bindingDescription;
    pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = 2;
    pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = attributeDescriptions;

    VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo = {};
    pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pipelineInputAssemblyStateCreateInfo.pNext = nullptr;
    pipelineInputAssemblyStateCreateInfo.flags = 0;
    pipelineInputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = false;

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo = {};
    pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    pipelineDynamicStateCreateInfo.pNext = nullptr;
    pipelineDynamicStateCreateInfo.flags = 0;
    pipelineDynamicStateCreateInfo.dynamicStateCount = ARRAY_COUNT(dynamicStates);
    pipelineDynamicStateCreateInfo.pDynamicStates = dynamicStates;

    VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo = {};
    pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pipelineViewportStateCreateInfo.pNext = nullptr;
    pipelineViewportStateCreateInfo.flags = 0;
    pipelineViewportStateCreateInfo.viewportCount = 1;
    pipelineViewportStateCreateInfo.pViewports = nullptr;
    pipelineViewportStateCreateInfo.scissorCount = 1;
    pipelineViewportStateCreateInfo.pScissors = nullptr;

    VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo = {};
    pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pipelineRasterizationStateCreateInfo.pNext = nullptr;
    pipelineRasterizationStateCreateInfo.flags = 0;
    pipelineRasterizationStateCreateInfo.depthClampEnable = false;
    pipelineRasterizationStateCreateInfo.rasterizerDiscardEnable = false;
    pipelineRasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    pipelineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    pipelineRasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    pipelineRasterizationStateCreateInfo.depthBiasEnable = false;
    pipelineRasterizationStateCreateInfo.depthBiasConstantFactor = 0;
    pipelineRasterizationStateCreateInfo.depthBiasClamp = 0;
    pipelineRasterizationStateCreateInfo.depthBiasSlopeFactor = 1.0f;
    pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;
    
    VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo = {};
    pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipelineMultisampleStateCreateInfo.pNext = nullptr;
    pipelineMultisampleStateCreateInfo.flags = 0;
    pipelineMultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pipelineMultisampleStateCreateInfo.sampleShadingEnable = false;
    pipelineMultisampleStateCreateInfo.minSampleShading = 0;
    pipelineMultisampleStateCreateInfo.pSampleMask = nullptr;
    pipelineMultisampleStateCreateInfo.alphaToCoverageEnable = false;
    pipelineMultisampleStateCreateInfo.alphaToOneEnable = false;

    VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState = {};
    pipelineColorBlendAttachmentState.blendEnable = false;
    pipelineColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    pipelineColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    pipelineColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    pipelineColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    pipelineColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    pipelineColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
    pipelineColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |VK_COLOR_COMPONENT_A_BIT;
    
    VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo = {};
    pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pipelineColorBlendStateCreateInfo.pNext = nullptr;
    pipelineColorBlendStateCreateInfo.flags = 0;
    pipelineColorBlendStateCreateInfo.logicOpEnable = false;
    pipelineColorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    pipelineColorBlendStateCreateInfo.attachmentCount = 1;
    pipelineColorBlendStateCreateInfo.pAttachments = &pipelineColorBlendAttachmentState;
    pipelineColorBlendStateCreateInfo.blendConstants[0] = 0;
    pipelineColorBlendStateCreateInfo.blendConstants[1] = 0;
    pipelineColorBlendStateCreateInfo.blendConstants[2] = 0;
    pipelineColorBlendStateCreateInfo.blendConstants[3] = 0;

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.pNext = nullptr;
    pipelineLayoutCreateInfo.flags = 0;
    pipelineLayoutCreateInfo.setLayoutCount = 0;
    pipelineLayoutCreateInfo.pSetLayouts = nullptr;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
    vkResult = vkCreatePipelineLayout(vkm.vkDevice, &pipelineLayoutCreateInfo, nullptr, &vkm.pipelineLayout);
    if (vkResult != VK_SUCCESS)
    {
        OutputDebugString("Could not create pipeline layout");
        return false;
    }

    VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo = {};
    pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingCreateInfo.pNext = nullptr;
    pipelineRenderingCreateInfo.viewMask = 0;
    pipelineRenderingCreateInfo.colorAttachmentCount = 1;
    pipelineRenderingCreateInfo.pColorAttachmentFormats = &vkm.swapChainImageFormat;
    pipelineRenderingCreateInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
    pipelineRenderingCreateInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
    graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineCreateInfo.pNext = &pipelineRenderingCreateInfo;
    graphicsPipelineCreateInfo.flags = 0;
    graphicsPipelineCreateInfo.stageCount = 2;
    graphicsPipelineCreateInfo.pStages = shaderStages;
    graphicsPipelineCreateInfo.pVertexInputState = &pipelineVertexInputStateCreateInfo;
    graphicsPipelineCreateInfo.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo;
    graphicsPipelineCreateInfo.pTessellationState = nullptr;
    graphicsPipelineCreateInfo.pViewportState = &pipelineViewportStateCreateInfo;
    graphicsPipelineCreateInfo.pRasterizationState = &pipelineRasterizationStateCreateInfo;
    graphicsPipelineCreateInfo.pMultisampleState = &pipelineMultisampleStateCreateInfo;
    graphicsPipelineCreateInfo.pDepthStencilState = nullptr;
    graphicsPipelineCreateInfo.pColorBlendState = &pipelineColorBlendStateCreateInfo;
    graphicsPipelineCreateInfo.pDynamicState = &pipelineDynamicStateCreateInfo;
    graphicsPipelineCreateInfo.layout = vkm.pipelineLayout;
    graphicsPipelineCreateInfo.renderPass = VK_NULL_HANDLE;
    graphicsPipelineCreateInfo.subpass = 0;
    graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    graphicsPipelineCreateInfo.basePipelineIndex = -1;

    vkResult = vkCreateGraphicsPipelines(vkm.vkDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &vkm.graphicsPipeline);
    if (vkResult != VK_SUCCESS)
    {
        OutputDebugString("Could not create graphics pipeline");
        return false;
    }

    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.pNext = nullptr;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCreateInfo.queueFamilyIndex = vkm.graphicsQueueIndex;
    vkResult = vkCreateCommandPool(vkm.vkDevice, &commandPoolCreateInfo, nullptr, &vkm.commandPool);
    if (vkResult != VK_SUCCESS)
    {
        OutputDebugString("Could not create command pool");
        return false;
    }

    VkDeviceSize vertexBufferSize = sizeof(vertices);
    VkBuffer stagingBufferForVertices = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferForVerticesMemory = VK_NULL_HANDLE;
    if (!VkmCreateAndFillBuffer(vertexBufferSize, (void*)vertices, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBufferForVertices, &stagingBufferForVerticesMemory))
    {
        OutputDebugString("Could not create vertex staging buffer\n.");
        return false;
    }
    
    VkDeviceMemory vertexBufferForVerticesMemory = VK_NULL_HANDLE;
    if (!VkmCreateBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vkm.vertexBuffer, &vertexBufferForVerticesMemory))
    {
        OutputDebugString("Could not create vertex staging buffer\n.");
        return false;
    }
    VkmCopyBuffer(stagingBufferForVertices, vkm.vertexBuffer, vertexBufferSize);
    
    VkDeviceSize indexBufferSize = sizeof(indices);
    VkBuffer stagingBufferForIndices = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferForIndicesMemory = VK_NULL_HANDLE;
    if (!VkmCreateAndFillBuffer(indexBufferSize, (void*)indices, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBufferForIndices, &stagingBufferForIndicesMemory))
    {
        OutputDebugString("Could not create index staging buffer\n.");
        return false;
    }
    
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
    if (!VkmCreateBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vkm.indexBuffer, &indexBufferMemory))
    {
        OutputDebugString("Could not create index staging buffer\n.");
        return false;
    }
    VkmCopyBuffer(stagingBufferForIndices, vkm.indexBuffer, indexBufferSize);
        
    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.pNext = nullptr;
    commandBufferAllocateInfo.commandPool = vkm.commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
    
    vkResult = vkAllocateCommandBuffers(vkm.vkDevice, &commandBufferAllocateInfo, vkm.commandBuffers);
    if (vkResult != VK_SUCCESS)
    {
        OutputDebugString("Failed to allocate command buffer.");
        return false;
    }

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (   
            vkCreateSemaphore(vkm.vkDevice, &semaphoreInfo, nullptr, &vkm.acquireSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(vkm.vkDevice, &fenceInfo, nullptr, &vkm.frameFences[i]) != VK_SUCCESS) 
        {
            OutputDebugString("failed to create semaphores!");
            return false;
        }
    }
    
    vkm.submitSemaphores = new VkSemaphore[vkm.swapChainImagesCount];
    for (u32 i = 0; i < vkm.swapChainImagesCount; i++)
    {
        if (vkCreateSemaphore(vkm.vkDevice, &semaphoreInfo, nullptr, &vkm.submitSemaphores[i]))
        {
            OutputDebugString("failed to create semaphores!");
            return false;
        }
    }
    return true;
}

void VkmCleanUp()
{
    if (vkm.vkDevice != VK_NULL_HANDLE)
        vkDeviceWaitIdle(vkm.vkDevice);

    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkm.acquireSemaphores[i] != VK_NULL_HANDLE)
            vkDestroySemaphore(vkm.vkDevice, vkm.acquireSemaphores[i], nullptr);
        if (vkm.frameFences[i] != VK_NULL_HANDLE)
            vkDestroyFence(vkm.vkDevice, vkm.frameFences[i], nullptr);
    }
    for (u32 i = 0; i < vkm.swapChainImagesCount; i++)
    {
        if (vkm.submitSemaphores[i] != VK_NULL_HANDLE)
            vkDestroySemaphore(vkm.vkDevice, vkm.submitSemaphores[i], nullptr);
    }
    vkDestroyPipeline(vkm.vkDevice, vkm.graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(vkm.vkDevice, vkm.pipelineLayout, nullptr);
    vkDestroyCommandPool(vkm.vkDevice, vkm.commandPool, nullptr);
    vkDestroyShaderModule(vkm.vkDevice, vkm.shaderModule, nullptr);
    if (vkm.swapChainImageViews)
    {
        for (u32 i = 0; i < vkm.swapChainImagesCount; i++)
        {
            if (vkm.swapChainImageViews[i] != VK_NULL_HANDLE)
                vkDestroyImageView(vkm.vkDevice, vkm.swapChainImageViews[i], nullptr);
        }
    }
    if (vkm.swapChain != VK_NULL_HANDLE)
         vkDestroySwapchainKHR(vkm.vkDevice, vkm.swapChain, nullptr);
    vkDestroyDevice(vkm.vkDevice, NULL);
    vkDestroySurfaceKHR(vkm.vkInstance, vkm.surface, NULL);
    // pfnDestroyDebugUtilsMessengerEXT(vkm.vkInstance, vKDebugUtilsMessengerExt, NULL);
    vkDestroyInstance(vkm.vkInstance, NULL);

    delete[] vkm.swapChainImageViews;
    delete[] vkm.swapChainImages;
    delete[] vkm.shaderFileBytes;
}

void VkmCopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.pNext = nullptr;
    commandBufferAllocateInfo.commandPool = vkm.commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;

    VkCommandBuffer commandCopyBuffer;
    VkResult result = vkAllocateCommandBuffers(vkm.vkDevice, &commandBufferAllocateInfo, &commandCopyBuffer);
    if (result != VK_SUCCESS) 
    {
        OutputDebugString("Could not allocate temp command buffer");
        return;
    }

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; 
    beginInfo.pInheritanceInfo = nullptr; 

    result = vkBeginCommandBuffer(commandCopyBuffer, &beginInfo);
    if (result != VK_SUCCESS)
    {
        OutputDebugString("Could not begin command buffer");
        return;
    }

    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;

     vkCmdCopyBuffer(commandCopyBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    result = vkEndCommandBuffer(commandCopyBuffer);
    if (result != VK_SUCCESS)
    {
        OutputDebugString("Could not end command buffer");
        return;
    }

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandCopyBuffer;

    vkQueueSubmit(vkm.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vkm.graphicsQueue);

    vkFreeCommandBuffers(vkm.vkDevice, vkm.commandPool, 1, &commandCopyBuffer);
}

s32 VkmFindMemoryTypeIndex(uint32_t typeFilter, VkMemoryPropertyFlags memoryPropertyFlags)
{
    VkPhysicalDeviceMemoryProperties memProperties = {}; 
    vkGetPhysicalDeviceMemoryProperties(vkm.physicalDevice, &memProperties);

    // u32 typeFilter = memRequirements.memoryTypeBits;
    // VkMemoryPropertyFlags memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    s32 memoryTypeIndex = -1;

    for (u32 i = 0; i < memProperties.memoryTypeCount; i++) 
    {
        if ((typeFilter & (1 << i))  && (memProperties.memoryTypes[i].propertyFlags & memoryPropertyFlags) == memoryPropertyFlags) 
        {
            memoryTypeIndex = i;
            break;
        }
    }

    return memoryTypeIndex;
}

bool VkmCreateAndFillBuffer(VkDeviceSize size, void* data, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkBuffer* buffer, VkDeviceMemory* bufferMemory)
{
    if (!VkmCreateBuffer(size, usageFlags, memoryPropertyFlags, buffer, bufferMemory))
    {
        OutputDebugString("Could not create buffer\n.");
        return false;
    }

    vkBindBufferMemory(vkm.vkDevice, *buffer, *bufferMemory, 0);
    void* mappedMemory = nullptr;
    VkResult vkResult = vkMapMemory(vkm.vkDevice, *bufferMemory, 0, size, 0, &mappedMemory);
    if (vkResult != VK_SUCCESS)
    {
        OutputDebugString("Could not map staging memory.");
        return false;
    }
    memcpy(mappedMemory, data, size);
    vkUnmapMemory(vkm.vkDevice, *bufferMemory);
    return true;
}

bool VkmCreateBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkBuffer* buffer, VkDeviceMemory* bufferMemory)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext = nullptr;
    bufferInfo.flags = 0;
    bufferInfo.size = size;
    bufferInfo.usage = usageFlags;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.queueFamilyIndexCount = 0;
    bufferInfo.pQueueFamilyIndices = nullptr;
    VkResult vkResult = vkCreateBuffer(vkm.vkDevice, &bufferInfo, nullptr, buffer);
    if (vkResult != VK_SUCCESS)
    {
        OutputDebugString("Could not create buffer\n");
        return false;
    }

    VkMemoryRequirements memRequirements = {};
    vkGetBufferMemoryRequirements(vkm.vkDevice, *buffer, &memRequirements);

    VkMemoryAllocateInfo memoryAllocateInfo = {};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.pNext = nullptr;
    memoryAllocateInfo.allocationSize = memRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = VkmFindMemoryTypeIndex(memRequirements.memoryTypeBits, memoryPropertyFlags);
    if (memoryAllocateInfo.memoryTypeIndex == -1)
    {
        OutputDebugString("Could not find suitable memory type in physical device\n");
        return false;
    }
    vkResult = vkAllocateMemory(vkm.vkDevice, &memoryAllocateInfo, nullptr, bufferMemory);

    if (vkResult != VK_SUCCESS)
    {
        OutputDebugString("Could not allocate memory\n");
        return false;
    }

    vkBindBufferMemory(vkm.vkDevice, *buffer, *bufferMemory, 0);
    return true;
}

bool VkmRecreateSwapChain()
{
    OutputDebugString("Creating a new swap chain.\n");
    VkSwapchainKHR oldSwapChain = vkm.swapChain;
    if (vkm.swapChainImageViews)
    {
        for (u32 i = 0; i < vkm.swapChainImagesCount; i++)
        {
            if (vkm.swapChainImageViews[i] != VK_NULL_HANDLE)
            vkDestroyImageView(vkm.vkDevice, vkm.swapChainImageViews[i], nullptr);
        }
        delete[] vkm.swapChainImageViews;
    }
    vkm.swapChainImagesCount = 0;

    // Wait for non-minimized window
    RECT rect;
    int width = 0, height = 0;
    while (width == 0 || height == 0)
    {
        if (!GetClientRect(_hwnd, &rect))
        {
            Sleep(1);
            continue;
        }

        width = rect.right - rect.left;
        height = rect.bottom - rect.top;

        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        Sleep(1);
    }
    
    vkDeviceWaitIdle(vkm.vkDevice);
    VkResult vkResult = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkm.physicalDevice, vkm.surface, &vkm.surfaceCapabilities);
    if (vkResult != VK_SUCCESS)
    {
        OutputDebugString("Could not get physical device surface capabilities\n");
        return false;
    }

    if (vkm.surfaceCapabilities.currentExtent.width != UINT32_MAX)
    {
        vkm.extent = vkm.surfaceCapabilities.currentExtent;
    }
    else
    {
        RECT rect;
        GetClientRect(_hwnd, &rect);
        u32 width  = u32(rect.right  - rect.left);
        u32 height = u32(rect.bottom - rect.top);

        vkm.extent.width = Clamp(width, vkm.surfaceCapabilities.minImageExtent.width, vkm.surfaceCapabilities.maxImageExtent.width);
        vkm.extent.height = Clamp(height, vkm.surfaceCapabilities.minImageExtent.height, vkm.surfaceCapabilities.maxImageExtent.height);
    }

    vkm.imageCount = vkm.surfaceCapabilities.minImageCount + 1;
    // imageCount = Clamp(imageCount, surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount);
    if (vkm.surfaceCapabilities.maxImageCount > 0 && vkm.imageCount > vkm.surfaceCapabilities.maxImageCount)
        vkm.imageCount = vkm.surfaceCapabilities.maxImageCount;


    VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.pNext = nullptr;
    swapChainCreateInfo.flags = 0;
    swapChainCreateInfo.surface = vkm.surface;
    swapChainCreateInfo.minImageCount = vkm.imageCount;
    swapChainCreateInfo.imageFormat = vkm.surfaceFormat.format;
    swapChainCreateInfo.imageColorSpace = vkm.surfaceFormat.colorSpace;
    swapChainCreateInfo.imageExtent = vkm.extent;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapChainCreateInfo.queueFamilyIndexCount = 0;
    swapChainCreateInfo.pQueueFamilyIndices = nullptr;
    swapChainCreateInfo.preTransform = vkm.surfaceCapabilities.currentTransform;
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainCreateInfo.presentMode = vkm.presentMode;
    swapChainCreateInfo.clipped = true;
    swapChainCreateInfo.oldSwapchain = oldSwapChain;

    u32 queueFamilyIndices[] = {(u32)vkm.graphicsQueueIndex, (u32)vkm.presentQueueIndex};
    if (vkm.graphicsQueueIndex != vkm.presentQueueIndex) 
    {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapChainCreateInfo.queueFamilyIndexCount = 2;
        swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    }

    vkResult = vkCreateSwapchainKHR(vkm.vkDevice, &swapChainCreateInfo, nullptr, &vkm.swapChain);
    if (vkResult != VK_SUCCESS)
    {
        OutputDebugString("Could not create swap chain.\n");
        return false;
    }

    if (oldSwapChain != VK_NULL_HANDLE && oldSwapChain != vkm.swapChain)
    {
        vkDestroySwapchainKHR(vkm.vkDevice, oldSwapChain, nullptr);
    }

    vkResult = vkGetSwapchainImagesKHR(vkm.vkDevice, vkm.swapChain, &vkm.swapChainImagesCount, nullptr);
    if (vkResult != VK_SUCCESS || vkm.swapChainImagesCount == 0)
    {
        OutputDebugString("Could not determine amount of swap chain images.\n");
        return false;
    }
    vkm.swapChainImages = new VkImage[vkm.swapChainImagesCount];
    vkResult = vkGetSwapchainImagesKHR(vkm.vkDevice, vkm.swapChain, &vkm.swapChainImagesCount, vkm.swapChainImages);
    vkm.swapChainImageFormat = vkm.surfaceFormat.format;
    vkm.swapChainExtent = vkm.extent;

    VkImageSubresourceRange imageSubresourceRanger = {};
    imageSubresourceRanger.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageSubresourceRanger.baseMipLevel = 0;
    imageSubresourceRanger.levelCount = 1;
    imageSubresourceRanger.baseArrayLayer = 0;
    imageSubresourceRanger.layerCount = 1;

    VkImageViewCreateInfo imageViewCreateInfo = {};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.pNext = nullptr;
    imageViewCreateInfo.flags = 0;
    imageViewCreateInfo.image = VK_NULL_HANDLE;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = vkm.swapChainImageFormat;
    imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.subresourceRange = imageSubresourceRanger;

    vkm.swapChainImageViews = new VkImageView[vkm.swapChainImagesCount];
    for (u32 i = 0; i < vkm.swapChainImagesCount; i++)
    {
        VkImage currentImage = vkm.swapChainImages[i];
        imageViewCreateInfo.image = currentImage;

         vkResult = vkCreateImageView(vkm.vkDevice, &imageViewCreateInfo, nullptr, &vkm.swapChainImageViews[i]);
         if (vkResult != VK_SUCCESS)
         {
            OutputDebugString("Failed to create an image view for swap buffer image");
            return false;
         }
    }

    return true;
}

