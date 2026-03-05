#define WIN32_MEAN_AND_LEAN
#define WIN32_MEAN_AND_LEAN
#include <windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include "mcy_helpers.h"
#include <glm/glm.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

struct Vertex
{
    glm::vec2 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
};

struct UniformBufferObject 
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
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
    VkVertexInputAttributeDescription* result = new VkVertexInputAttributeDescription[3];

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

    result[2] = {};
    result[2].location = 2;
    result[2].binding = 0;
    result[2].format = VK_FORMAT_R32G32_SFLOAT;
    result[2].offset = offsetof(Vertex, texCoord);
    
    return result;
}

#define VK_CALL(call, msg)                       \
    do {                                        \
        VkResult _vkResult = (call);           \
        if (_vkResult != VK_SUCCESS) {         \
            OutputDebugString(msg);            \
            OutputDebugString("\n");            \
            return true;                      \
        }                                      \
    } while (0)

const u32 MAX_FRAMES_IN_FLIGHT = 2;

HWND _hwnd = NULL;
struct VkMState
{
    VkInstance vkInstance;
    VkDebugUtilsMessengerEXT vKDebugUtilsMessengerExt;
    PFN_vkDestroyDebugUtilsMessengerEXT pfnDestroyDebugUtilsMessengerEXT;
    char* shaderFileBytes;
    VkPhysicalDevice physicalDevice;
    PFN_vkSetDebugUtilsObjectNameEXT pfnSetDebugUtilsObjectNameEXT;
    
    VkDevice vkDevice;
    VkSurfaceCapabilitiesKHR surfaceCapabilities; 
    VkSurfaceKHR surface;
    VkSurfaceFormatKHR surfaceFormat;
    VkExtent2D extent;
    
    VkSwapchainKHR swapChain;
    VkFormat swapChainImageFormat;
    u32 swapChainImagesCount;
    u32 imageCount;
    VkImage* swapChainImages;
    VkImageView* swapChainImageViews;
    VkExtent2D swapChainExtent;
    VkPresentModeKHR presentMode;
    bool framebufferResized;
    u32 imageIndex;

    VkPipeline graphicsPipeline;
    VkShaderModule shaderModule;
    
    VkBuffer uniformBuffers[MAX_FRAMES_IN_FLIGHT];
    VkDeviceMemory uniformBuffersMemory[MAX_FRAMES_IN_FLIGHT];
    void* uniformBuffersMapped[MAX_FRAMES_IN_FLIGHT];
    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout descriptorSetLayouts[MAX_FRAMES_IN_FLIGHT];
    VkDescriptorSet descriptorSets[MAX_FRAMES_IN_FLIGHT];
    
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

    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;
};
VkMState vkm = {};

bool VkmCopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
s32 VkmFindMemoryTypeIndex(uint32_t typeFilter, VkMemoryPropertyFlags memoryPropertyFlags);
bool VkmCreateAndFillBuffer(VkDeviceSize size, void* data, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkBuffer* buffer, VkDeviceMemory* bufferMemory);
bool VkmCreateBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkBuffer* buffer, VkDeviceMemory* bufferMemory);
bool VkmRecreateSwapChain();
bool VkmCreateTextureImage();
bool VkmCreateTextureImage();
bool VkmCreateImageView(VkImage* image, VkFormat format, VkImageView* imageView);

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

void VkmAttachNameToObject(u64 handle, VkObjectType objectType, char* name)
{
    VkDebugUtilsObjectNameInfoEXT nameInfo = {};
    nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    nameInfo.objectType = objectType;
    nameInfo.objectHandle = handle;
    nameInfo.pObjectName = name;
    vkm.pfnSetDebugUtilsObjectNameEXT(vkm.vkDevice, &nameInfo);
}

bool VkmInitialize()
{
    vkm = {};
    vkm.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    vkm.graphicsQueueIndex = -1;
    vkm.presentQueueIndex = -1;

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

    VK_CALL(vkCreateInstance(&vkInstanceCreateInfo, nullptr, &vkm.vkInstance), "Could not create Vulkan instance");

    VkWin32SurfaceCreateInfoKHR win32SurfaceCreateInfo{};
    win32SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    win32SurfaceCreateInfo.pNext = nullptr;
    win32SurfaceCreateInfo.flags = 0;
    win32SurfaceCreateInfo.hwnd = _hwnd;
    win32SurfaceCreateInfo.hinstance = GetModuleHandle(nullptr);

    VK_CALL(vkCreateWin32SurfaceKHR(vkm.vkInstance, &win32SurfaceCreateInfo, nullptr, &vkm.surface), "Could not create win32 surface");
    
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

    VK_CALL(pfnCreateDebugUtilsMessengerEXT(vkm.vkInstance, &vkDebugUtilsMessengerCreateInfoEXT, nullptr, &vkm.vKDebugUtilsMessengerExt), "Could not create debug util messenger");

    u32 vkPhysicalDeviceCount;
    vkEnumeratePhysicalDevices(vkm.vkInstance, &vkPhysicalDeviceCount, nullptr);
    
    if (vkPhysicalDeviceCount == 0)
    {
        OutputDebugString("Could not find any physical devices\n");
        return false;
    }
    
    VkPhysicalDevice* vkPhysicalDevices = new VkPhysicalDevice[vkPhysicalDeviceCount];
    VK_CALL(vkEnumeratePhysicalDevices(vkm.vkInstance, &vkPhysicalDeviceCount, vkPhysicalDevices), "Could not enumerate devices");

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

    VkPhysicalDeviceFeatures2 vkPhysicalDeviceFeatures2 = {};
    vkPhysicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    vkPhysicalDeviceFeatures2.pNext = nullptr;
    vkPhysicalDeviceFeatures2.features.samplerAnisotropy = true;

    VkPhysicalDeviceVulkan11Features vkPhysicalDeviceVulkan11Features = {};
    vkPhysicalDeviceVulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    vkPhysicalDeviceVulkan11Features.pNext = nullptr;
    vkPhysicalDeviceVulkan11Features.shaderDrawParameters = true;

    VkPhysicalDeviceVulkan13Features vkPhysicalDeviceVulkan13Features = {};
    vkPhysicalDeviceVulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    vkPhysicalDeviceVulkan13Features.pNext = nullptr;
    vkPhysicalDeviceVulkan13Features.synchronization2 = true;
    vkPhysicalDeviceVulkan13Features.dynamicRendering = true;

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
    VK_CALL(vkCreateDevice(vkm.physicalDevice, &vkDeviceCreateInfo, nullptr, &vkm.vkDevice), "Could not create logical device");

    vkm.pfnSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT) vkGetDeviceProcAddr(vkm.vkDevice, "vkSetDebugUtilsObjectNameEXT");

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
    VK_CALL(vkGetPhysicalDeviceSurfaceFormatsKHR(vkm.physicalDevice, vkm.surface, &surfaceFormatCount, nullptr), "Could not get surface formats");
    if (surfaceFormatCount == 0)
    {
        OutputDebugString("Could not get surface formats\n");
        return false;
    }
    VkSurfaceFormatKHR* surfaceFormats = new VkSurfaceFormatKHR[surfaceFormatCount];
    VK_CALL(vkGetPhysicalDeviceSurfaceFormatsKHR(vkm.physicalDevice, vkm.surface, &surfaceFormatCount, surfaceFormats), "Could not get surface formats");

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
    
    if (!ReadEntireFile("slang.spv", &vkm.shaderFileBytes, &shaderFileSize))
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

    VK_CALL(vkCreateShaderModule(vkm.vkDevice, &shaderModuleCreateInfo, nullptr, &vkm.shaderModule), "Failed to create shader module");

    VkDescriptorSetLayoutBinding descriptorSetBindings[2] = {};
    descriptorSetBindings[0].binding = 0;
    descriptorSetBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorSetBindings[0].descriptorCount = 1;
    descriptorSetBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    descriptorSetBindings[0].pImmutableSamplers = nullptr;
    descriptorSetBindings[1].binding = 1;
    descriptorSetBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorSetBindings[1].descriptorCount = 1;
    descriptorSetBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    descriptorSetBindings[1].pImmutableSamplers = nullptr;
    VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo;
    setLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutCreateInfo.pNext = nullptr;
    setLayoutCreateInfo.flags = 0;
    setLayoutCreateInfo.bindingCount = 2;
    setLayoutCreateInfo.pBindings = descriptorSetBindings;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE; 
    VK_CALL(vkCreateDescriptorSetLayout(vkm.vkDevice, &setLayoutCreateInfo, nullptr, &descriptorSetLayout), "Could not create descriptor set.");
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkm.descriptorSetLayouts[i] = descriptorSetLayout;
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
    pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = 3;
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
    pipelineRasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
    VK_CALL(vkCreatePipelineLayout(vkm.vkDevice, &pipelineLayoutCreateInfo, nullptr, &vkm.pipelineLayout), "Could not create pipeline layout");

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

    VK_CALL(vkCreateGraphicsPipelines(vkm.vkDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &vkm.graphicsPipeline), "Could not create graphics pipeline");

    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.pNext = nullptr;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCreateInfo.queueFamilyIndex = vkm.graphicsQueueIndex;
    VK_CALL(vkCreateCommandPool(vkm.vkDevice, &commandPoolCreateInfo, nullptr, &vkm.commandPool), "Could not create command pool");
        
    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.pNext = nullptr;
    commandBufferAllocateInfo.commandPool = vkm.commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
    
    VK_CALL(vkAllocateCommandBuffers(vkm.vkDevice, &commandBufferAllocateInfo, vkm.commandBuffers), "Failed to allocate command buffer");

    u32 uniformBufferSize = sizeof(UniformBufferObject);
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (!VkmCreateBuffer(uniformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            &vkm.uniformBuffers[i], &vkm.uniformBuffersMemory[i]))
        {
            OutputDebugString("Could not create uniform buffer");
            return false;   
        }

        VkmAttachNameToObject((u64)vkm.uniformBuffers[i], VK_OBJECT_TYPE_BUFFER, "uniform buffer");

        VkResult vkResult = vkMapMemory(vkm.vkDevice, vkm.uniformBuffersMemory[i], 0, uniformBufferSize, 0, &vkm.uniformBuffersMapped[i]);
        if (vkResult != VK_SUCCESS)
        {
            OutputDebugString("Could not map staging memory.");
            return false;
        }
    }

    if (!VkmCreateTextureImage())
    {
        OutputDebugString("Could not make texture image");
        return false;
    }

    if (!VkmCreateImageView(&vkm.textureImage, VK_FORMAT_R8G8B8A8_SRGB, &vkm.textureImageView))
    {
        OutputDebugString("Could not create texture image view");
        return false;
    }

    VkPhysicalDeviceProperties physicalDeviceProperties = {};
    vkGetPhysicalDeviceProperties(vkm.physicalDevice, &physicalDeviceProperties);

    VkSamplerCreateInfo samplerCreateInfo = {};
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.pNext = nullptr;
    samplerCreateInfo.flags = 0;
    samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
    samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.mipLodBias = 0;
    samplerCreateInfo.anisotropyEnable = true;
    samplerCreateInfo.maxAnisotropy = physicalDeviceProperties.limits.maxSamplerAnisotropy;
    samplerCreateInfo.compareEnable = false;
    samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerCreateInfo.minLod = 0;
    samplerCreateInfo.maxLod = 0;
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerCreateInfo.unnormalizedCoordinates = false;

    vkCreateSampler(vkm.vkDevice, &samplerCreateInfo, nullptr, &vkm.textureSampler);

    VkDescriptorPoolSize descriptorPoolSizes[2] = {};
    descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorPoolSizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT;
    descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorPoolSizes[1].descriptorCount = MAX_FRAMES_IN_FLIGHT;
    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.pNext = nullptr;
    descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    descriptorPoolCreateInfo.maxSets = MAX_FRAMES_IN_FLIGHT;
    descriptorPoolCreateInfo.poolSizeCount = 2;
    descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes;
    vkCreateDescriptorPool(vkm.vkDevice, &descriptorPoolCreateInfo, nullptr, &vkm.descriptorPool);
    
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.pNext = nullptr;
    descriptorSetAllocateInfo.descriptorPool = vkm.descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = ARRAY_COUNT(vkm.descriptorSetLayouts);
    descriptorSetAllocateInfo.pSetLayouts = vkm.descriptorSetLayouts;
    vkAllocateDescriptorSets(vkm.vkDevice, &descriptorSetAllocateInfo, vkm.descriptorSets);

    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
    {
        VkDescriptorBufferInfo descriptorBufferInfo = {};
        descriptorBufferInfo.buffer = vkm.uniformBuffers[i];
        descriptorBufferInfo.offset = 0;
        descriptorBufferInfo.range = uniformBufferSize;

        VkDescriptorImageInfo imageInfo = {};
        imageInfo.sampler = vkm.textureSampler;
        imageInfo.imageView = vkm.textureImageView;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet writeDescriptorSets[2] = {};
        writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[0].pNext = nullptr;
        writeDescriptorSets[0].dstSet = vkm.descriptorSets[i];
        writeDescriptorSets[0].dstBinding = 0;
        writeDescriptorSets[0].dstArrayElement = 0;
        writeDescriptorSets[0].descriptorCount = 1;
        writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSets[0].pImageInfo = nullptr;
        writeDescriptorSets[0].pBufferInfo = &descriptorBufferInfo;
        writeDescriptorSets[0].pTexelBufferView = nullptr;
        
        writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[1].pNext = nullptr;
        writeDescriptorSets[1].dstSet = vkm.descriptorSets[i];
        writeDescriptorSets[1].dstBinding = 1;
        writeDescriptorSets[1].dstArrayElement = 0;
        writeDescriptorSets[1].descriptorCount = 1;
        writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSets[1].pImageInfo = &imageInfo;
        writeDescriptorSets[1].pBufferInfo = nullptr;
        writeDescriptorSets[1].pTexelBufferView = nullptr;

        vkUpdateDescriptorSets(vkm.vkDevice, 2, writeDescriptorSets, 0, nullptr);
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

    // textureImageViewCreateInfo.pNext = nullptr;
    // textureImageViewCreateInfo.flags;
    // VkImage                    textureImageViewCreateInfo.image;
    // textureImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    // VkFormat                   textureImageViewCreateInfo.format;
    // VkComponentMapping         textureImageViewCreateInfo.components;
    // VkImageSubresourceRange    textureImageViewCreateInfo.subresourceRange;

    return true;
}

bool VkmCreateImageView(VkImage* image, VkFormat format, VkImageView* imageView)
{
    VkImageViewCreateInfo imageViewCreateInfo = {};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.pNext = nullptr;
    imageViewCreateInfo.flags = 0;
    imageViewCreateInfo.image = *image;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = format;
    imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;

    VK_CALL(vkCreateImageView(vkm.vkDevice, &imageViewCreateInfo, nullptr, imageView), "Failed to create image view");

    return true;
}

bool VkmBeginSingleTimeCommands(VkCommandBuffer* commandBuffer)
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.commandPool = vkm.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    *commandBuffer = VK_NULL_HANDLE;
    VK_CALL(vkAllocateCommandBuffers(vkm.vkDevice, &allocInfo, commandBuffer), "Could not allocate for command buffer");

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pNext = nullptr;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr;

    VK_CALL(vkBeginCommandBuffer(*commandBuffer, &beginInfo), "Could not begin command buffer");
    
    return true;
}

void VkmEndSingleTimeCommands(VkCommandBuffer* commandBuffer)
{
    vkEndCommandBuffer(*commandBuffer);
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.pWaitDstStageMask = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = commandBuffer;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = nullptr;

    vkQueueSubmit(vkm.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

    vkQueueWaitIdle(vkm.graphicsQueue);

    vkFreeCommandBuffers(vkm.vkDevice, vkm.commandPool, 1, commandBuffer);
}

bool VkmCopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBuffer commandCopyBuffer = VK_NULL_HANDLE;
    if (!VkmBeginSingleTimeCommands(&commandCopyBuffer))
    {
        return false;
    }

    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandCopyBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    VkmEndSingleTimeCommands(&commandCopyBuffer);
    return true;
}

bool VkmTransitionImageLayout(VkImage *image, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    if (!VkmBeginSingleTimeCommands(&commandBuffer))
    {
        return false;
    }

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.pNext = nullptr;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = 0;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = 0;
    barrier.dstQueueFamilyIndex = 0;
    barrier.image = *image;
    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;
    barrier.subresourceRange = subresourceRange;

    VkPipelineStageFlags sourceStage = 0;
    VkPipelineStageFlags destinationStage = 0;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        OutputDebugString("Unsupported layout transition.");
        return false;
    }

    vkCmdPipelineBarrier(commandBuffer,
        sourceStage,
        destinationStage,
        0,                  
        0, nullptr,            
        0, nullptr,            
        1, &barrier         
    );

    VkmEndSingleTimeCommands(&commandBuffer);
    return true;
}

bool VkmCopyBufferToImage(VkBuffer buffer, VkImage image, u32 width, u32 height)
{
    VkCommandBuffer commandCopyBuffer = VK_NULL_HANDLE;
    if (!VkmBeginSingleTimeCommands(&commandCopyBuffer))
    {
        return false;
    }

    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset.x = 0;
    region.imageOffset.y = 0;
    region.imageOffset.z = 0;
    region.imageExtent.width = width;
    region.imageExtent.height = height;
    region.imageExtent.depth = 1;

    vkCmdCopyBufferToImage(commandCopyBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    VkmEndSingleTimeCommands(&commandCopyBuffer);
    return true;
}

s32 VkmFindMemoryTypeIndex(uint32_t typeFilter, VkMemoryPropertyFlags memoryPropertyFlags)
{
    VkPhysicalDeviceMemoryProperties memProperties = {}; 
    vkGetPhysicalDeviceMemoryProperties(vkm.physicalDevice, &memProperties);

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

    // vkBindBufferMemory(vkm.vkDevice, *buffer, *bufferMemory, 0);
    void* mappedMemory = nullptr;
    VK_CALL(vkMapMemory(vkm.vkDevice, *bufferMemory, 0, size, 0, &mappedMemory), "Could not map staging memory");
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
    VK_CALL(vkCreateBuffer(vkm.vkDevice, &bufferInfo, nullptr, buffer), "Could not create buffer");

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
    VK_CALL(vkAllocateMemory(vkm.vkDevice, &memoryAllocateInfo, nullptr, bufferMemory), "Could not allocate memory");

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
    VK_CALL(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkm.physicalDevice, vkm.surface, &vkm.surfaceCapabilities), "Could not get physical device surface capabilities");

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

    VK_CALL(vkCreateSwapchainKHR(vkm.vkDevice, &swapChainCreateInfo, nullptr, &vkm.swapChain), "Could not create swap chain");

    if (oldSwapChain != VK_NULL_HANDLE && oldSwapChain != vkm.swapChain)
    {
        vkDestroySwapchainKHR(vkm.vkDevice, oldSwapChain, nullptr);
    }

    VK_CALL(vkGetSwapchainImagesKHR(vkm.vkDevice, vkm.swapChain, &vkm.swapChainImagesCount, nullptr), "Could not determine amount of swap chain images");
    vkm.swapChainImages = new VkImage[vkm.swapChainImagesCount];
    VK_CALL(vkGetSwapchainImagesKHR(vkm.vkDevice, vkm.swapChain, &vkm.swapChainImagesCount, vkm.swapChainImages), "Could not fill get swap chain images");
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
        VkmCreateImageView(&vkm.swapChainImages[i], vkm.swapChainImageFormat, &vkm.swapChainImageViews[i]);
    }

    return true;
}

VkCommandBuffer VkmGetInFlightCommandBuffer()
{
    return vkm.commandBuffers[vkm.currentFrameInFlightIndex];
}

bool VkmSetupForFrameRendering(VkCommandBuffer commandBuffer)
{
    VK_CALL(vkWaitForFences(vkm.vkDevice, 1, &vkm.frameFences[vkm.currentFrameInFlightIndex], VK_TRUE, UINT64_MAX), "Failed to wait for fence");
    
    VkResult vkResult = vkAcquireNextImageKHR(vkm.vkDevice, vkm.swapChain, UINT64_MAX, vkm.acquireSemaphores[vkm.currentFrameInFlightIndex], VK_NULL_HANDLE, &vkm.imageIndex);
    if (vkResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        VkmRecreateSwapChain();
        return true;
    }
    vkResetFences(vkm.vkDevice, 1, &vkm.frameFences[vkm.currentFrameInFlightIndex]);
    
    if (vkResult != VK_SUCCESS && vkResult != VK_SUBOPTIMAL_KHR)
    {
        if (vkResult != VK_TIMEOUT && vkResult != VK_NOT_READY)
            OutputDebugString("Unexpected return value from acquire image");
        OutputDebugString("Failed to acquire swap chain image!");
        return false;
    }
    vkResetCommandBuffer(vkm.commandBuffers[vkm.currentFrameInFlightIndex], 0);

    VkCommandBufferBeginInfo commandBufferBeginInfo = {};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.pNext = nullptr;
    commandBufferBeginInfo.flags = 0;
    commandBufferBeginInfo.pInheritanceInfo = nullptr;
    vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);

    VkImageMemoryBarrier2 barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.pNext = nullptr;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT_KHR;
    barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = vkm.swapChainImages[vkm.imageIndex];
    VkImageSubresourceRange imageSubresourceName = {};
    imageSubresourceName.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageSubresourceName.baseMipLevel = 0;
    imageSubresourceName.levelCount = 1;
    imageSubresourceName.baseArrayLayer = 0;
    imageSubresourceName.layerCount = 1;
    barrier.subresourceRange = imageSubresourceName;

    VkDependencyInfo dependencyInfo = {};
    dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencyInfo.pNext = nullptr;
    dependencyInfo.dependencyFlags = 0;
    dependencyInfo.memoryBarrierCount = 0;
    dependencyInfo.pMemoryBarriers = nullptr;
    dependencyInfo.bufferMemoryBarrierCount = 0;
    dependencyInfo.pBufferMemoryBarriers = nullptr;
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &barrier;
    vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

    VkClearValue clearColor = {};
    clearColor.color.float32[0] = 0.0f;
    clearColor.color.float32[1] = 0.0f;
    clearColor.color.float32[2] = 0.0f;
    clearColor.color.float32[3] = 1.0f;
    VkRenderingAttachmentInfo attachmentInfo = {};
    attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    attachmentInfo.pNext = nullptr;
    attachmentInfo.imageView = vkm.swapChainImageViews[vkm.imageIndex];
    attachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachmentInfo.resolveMode = VK_RESOLVE_MODE_NONE;
    attachmentInfo.resolveImageView = VK_NULL_HANDLE;
    attachmentInfo.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentInfo.clearValue = clearColor;    

    VkRenderingInfo renderingInfo = {};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.pNext = nullptr;
    renderingInfo.flags = 0;
    VkRect2D renderArea = {};
    renderArea.offset.x = 0;
    renderArea.offset.y = 0;
    renderArea.extent = vkm.swapChainExtent;
    renderingInfo.renderArea = renderArea;
    renderingInfo.layerCount = 1;
    renderingInfo.viewMask = 0;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &attachmentInfo;
    renderingInfo.pDepthAttachment = nullptr;
    renderingInfo.pStencilAttachment = nullptr;

    vkCmdBeginRendering(commandBuffer, &renderingInfo);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkm.graphicsPipeline);
    
    VkViewport viewport = {};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = vkm.swapChainExtent.width;
    viewport.height = vkm.swapChainExtent.height;
    viewport.minDepth = 0;
    viewport.maxDepth = 1;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    
    VkRect2D scissorRect = {};
    scissorRect.offset.x = 0;
    scissorRect.offset.y = 0;
    scissorRect.extent = vkm.swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);

    return true;
}

bool VkmEndRenderingAndSetupForPresent(VkCommandBuffer commandBuffer)
{
    vkCmdEndRendering(commandBuffer);
    VkImageMemoryBarrier2 barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.pNext = nullptr;
    barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT_KHR;
    barrier.dstAccessMask = 0;
    barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    barrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = vkm.swapChainImages[vkm.imageIndex];
    VkImageSubresourceRange imageSubresourceName = {};
    imageSubresourceName.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageSubresourceName.baseMipLevel = 0;
    imageSubresourceName.levelCount = 1;
    imageSubresourceName.baseArrayLayer = 0;
    imageSubresourceName.layerCount = 1;
    barrier.subresourceRange = imageSubresourceName;

    VkDependencyInfo dependencyInfo = {};
    dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencyInfo.pNext = nullptr;
    dependencyInfo.dependencyFlags = 0;
    dependencyInfo.memoryBarrierCount = 0;
    dependencyInfo.pMemoryBarriers = nullptr;
    dependencyInfo.bufferMemoryBarrierCount = 0;
    dependencyInfo.pBufferMemoryBarriers = nullptr;
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &barrier;
    vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
    vkEndCommandBuffer(commandBuffer);

    VkSemaphore submitSemaphore = vkm.submitSemaphores[vkm.imageIndex];

    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &vkm.acquireSemaphores[vkm.currentFrameInFlightIndex];
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vkm.commandBuffers[vkm.currentFrameInFlightIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &submitSemaphore;
    vkQueueSubmit(vkm.graphicsQueue, 1, &submitInfo, vkm.frameFences[vkm.currentFrameInFlightIndex]);

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &submitSemaphore;
    VkSwapchainKHR swapChains[] = {vkm.swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &vkm.imageIndex;
    presentInfo.pResults = nullptr;
    VkResult vkResult = vkQueuePresentKHR(vkm.presentQueue, &presentInfo);
    if (vkResult == VK_SUBOPTIMAL_KHR || vkResult == VK_ERROR_OUT_OF_DATE_KHR || vkm.framebufferResized)
    {
        vkm.framebufferResized = false;
        VkmRecreateSwapChain();
    }
    else
    {
        if (vkResult != VK_SUCCESS)
        {
            OutputDebugString("Unexpected return value from queue present");
            return false;
        }
    }

    vkm.currentFrameInFlightIndex = (vkm.currentFrameInFlightIndex + 1) % MAX_FRAMES_IN_FLIGHT;
    return true;
}

bool VkmCreateTextureImage()
{
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) 
    {
        OutputDebugString("Could not load image");
        return false;
    }

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
    VkmCreateAndFillBuffer(imageSize, pixels, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                    &stagingBuffer, &stagingBufferMemory);

    stbi_image_free(pixels);

    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.pNext = nullptr;
    imageCreateInfo.flags = 0;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageCreateInfo.extent.width = texWidth;
    imageCreateInfo.extent.height = texHeight;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.queueFamilyIndexCount = 1;
    imageCreateInfo.pQueueFamilyIndices = (u32*)&vkm.graphicsQueueIndex;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VK_CALL(vkCreateImage(vkm.vkDevice, &imageCreateInfo, nullptr, &vkm.textureImage), "Could not create image");

    VkMemoryRequirements memRequirements = {};
    vkGetImageMemoryRequirements(vkm.vkDevice, vkm.textureImage, &memRequirements);
    VkMemoryAllocateInfo memoryAllocateInfo = {};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.pNext = nullptr;
    memoryAllocateInfo.allocationSize = memRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = VkmFindMemoryTypeIndex(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (memoryAllocateInfo.memoryTypeIndex == -1)
    {
        OutputDebugString("Could not find suitable memory type in physical device\n");
        return false;
    }
    VK_CALL(vkAllocateMemory(vkm.vkDevice, &memoryAllocateInfo, nullptr, &vkm.textureImageMemory), "Could not allocate memory");

    vkBindImageMemory(vkm.vkDevice, vkm.textureImage, vkm.textureImageMemory, 0);

    VkmTransitionImageLayout(&vkm.textureImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    VkmCopyBufferToImage(stagingBuffer, vkm.textureImage, texWidth, texHeight);
    VkmTransitionImageLayout(&vkm.textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(vkm.vkDevice, stagingBuffer, nullptr);
    vkFreeMemory(vkm.vkDevice, stagingBufferMemory, nullptr);
    
    return true;
}

void VkmCleanUp()
{
    if (vkm.vkDevice != VK_NULL_HANDLE)
        vkDeviceWaitIdle(vkm.vkDevice);

    // --- Semaphores & Fences ---
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkm.acquireSemaphores[i] != VK_NULL_HANDLE)
            vkDestroySemaphore(vkm.vkDevice, vkm.acquireSemaphores[i], nullptr);
        if (vkm.frameFences[i] != VK_NULL_HANDLE)
            vkDestroyFence(vkm.vkDevice, vkm.frameFences[i], nullptr);
    }

    if (vkm.submitSemaphores)
    {
        for (u32 i = 0; i < vkm.swapChainImagesCount; i++)
        {
            if (vkm.submitSemaphores[i] != VK_NULL_HANDLE)
                vkDestroySemaphore(vkm.vkDevice, vkm.submitSemaphores[i], nullptr);
        }
    }

    // --- Pipelines & Shader ---
    if (vkm.graphicsPipeline != VK_NULL_HANDLE)
        vkDestroyPipeline(vkm.vkDevice, vkm.graphicsPipeline, nullptr);
    if (vkm.pipelineLayout != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(vkm.vkDevice, vkm.pipelineLayout, nullptr);
    if (vkm.shaderModule != VK_NULL_HANDLE)
        vkDestroyShaderModule(vkm.vkDevice, vkm.shaderModule, nullptr);

    // --- Command Pool ---
    if (vkm.commandPool != VK_NULL_HANDLE)
        vkDestroyCommandPool(vkm.vkDevice, vkm.commandPool, nullptr);

    // --- Uniform Buffers ---
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkm.uniformBuffers[i] != VK_NULL_HANDLE)
            vkDestroyBuffer(vkm.vkDevice, vkm.uniformBuffers[i], nullptr);
        if (vkm.uniformBuffersMemory[i] != VK_NULL_HANDLE)
            vkFreeMemory(vkm.vkDevice, vkm.uniformBuffersMemory[i], nullptr);
    }

    // --- Descriptor Sets & Pool ---
    for (u32 i = 0; i < 1; i++)
    {
        if (vkm.descriptorSetLayouts[i] != VK_NULL_HANDLE)
            vkDestroyDescriptorSetLayout(vkm.vkDevice, vkm.descriptorSetLayouts[i], nullptr);
    }
    if (vkm.descriptorPool != VK_NULL_HANDLE)
        vkDestroyDescriptorPool(vkm.vkDevice, vkm.descriptorPool, nullptr);

    // --- Texture ---
    if (vkm.textureSampler != VK_NULL_HANDLE)
        vkDestroySampler(vkm.vkDevice, vkm.textureSampler, nullptr);
    if (vkm.textureImageView != VK_NULL_HANDLE)
        vkDestroyImageView(vkm.vkDevice, vkm.textureImageView, nullptr);
    if (vkm.textureImage != VK_NULL_HANDLE)
        vkDestroyImage(vkm.vkDevice, vkm.textureImage, nullptr);
    if (vkm.textureImageMemory != VK_NULL_HANDLE)
        vkFreeMemory(vkm.vkDevice, vkm.textureImageMemory, nullptr);

    // --- Swapchain Images & Views ---
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

    // --- Device, Surface, Debug Messenger, Instance ---
    if (vkm.vkDevice != VK_NULL_HANDLE)
        vkDestroyDevice(vkm.vkDevice, nullptr);

    if (vkm.surface != VK_NULL_HANDLE)
        vkDestroySurfaceKHR(vkm.vkInstance, vkm.surface, nullptr);

    if (vkm.vKDebugUtilsMessengerExt != VK_NULL_HANDLE && vkm.pfnDestroyDebugUtilsMessengerEXT)
        vkm.pfnDestroyDebugUtilsMessengerEXT(vkm.vkInstance, vkm.vKDebugUtilsMessengerExt, nullptr);

    if (vkm.vkInstance != VK_NULL_HANDLE)
        vkDestroyInstance(vkm.vkInstance, nullptr);

    // --- Free heap memory ---
    delete[] vkm.swapChainImageViews;
    delete[] vkm.swapChainImages;
    delete[] vkm.shaderFileBytes;
}
