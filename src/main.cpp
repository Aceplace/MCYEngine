#include <iostream>
#include <vector>
#include <string>
#include <fstream>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#define s8  int8_t
#define s16 int16_t
#define s32 int32_t
#define s64 int64_t
#define u8  uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t
#define st size_t

uint32_t pupp = 0;
#define ARRAY_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))
#include <cmath>

template<typename T>
T Clamp(T value, T min, T max)
{
    if (value < min) 
        return min;
    if (value > max)
        return max;
    return value;
}

template<typename T>
T Max(T value1, T value2)
{
    if (value1 > value2) 
        return value1;
    return value2;
}

template<typename T>
T Min(T value1, T value2)
{
    if (value1 < value2) 
        return value1;
    return value2;
}

bool ReadEntireFile(const char* fileName, char** bufferOut, st* sizeOut)
{
    char errorBuffer[512];

    *bufferOut = nullptr;
    *sizeOut = 0;

    FILE* file = fopen(fileName, "rb");
    if (!file)
    {
        sprintf_s(errorBuffer, "Could not load file: %s\n", fileName);
        OutputDebugString(errorBuffer);
        return false;
    }
    
    std::fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    if (fileSize < 0)
    {
        sprintf_s(errorBuffer, "Invalid file size: %s\n", fileName);
        OutputDebugString(errorBuffer);
        std::fclose(file);
        return false;
    }
    *sizeOut = (st)fileSize;

    fseek(file, 0, SEEK_SET);
    *bufferOut = new char[*sizeOut];

    size_t bytesRead = std::fread(*bufferOut, 1, *sizeOut, file);
    std::fclose(file);

    if (bytesRead != *sizeOut)
    {
        delete[] *bufferOut;
        *bufferOut = nullptr;
        sizeOut = 0;
        sprintf_s(errorBuffer, "Could not read all bytes: %s\n", fileName);
        OutputDebugString(errorBuffer);
        return false; 
    }

    return true;
}

VkDevice vkDevice = VK_NULL_HANDLE;
VkSwapchainKHR swapChain = VK_NULL_HANDLE;
const u32 MAX_FRAMES_IN_FLIGHT = 2;
VkCommandBuffer commandBuffers[MAX_FRAMES_IN_FLIGHT] = {};
u32 imageCount = 0;
VkImage* swapChainImages = nullptr;
VkImageView* swapChainImageViews = nullptr;
VkExtent2D swapChainExtent = {};
VkPipeline graphicsPipeline = VK_NULL_HANDLE;
VkSurfaceKHR surface = VK_NULL_HANDLE;
VkSurfaceFormatKHR surfaceFormat = {};
VkExtent2D extent = {};
VkSurfaceCapabilitiesKHR surfaceCapabilities{}; 
VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
s32 graphicsQueueIndex = -1;
s32 presentQueueIndex = -1;
VkFormat swapChainImageFormat = VK_FORMAT_UNDEFINED;
u32 swapChainImagesCount = 0;

void CleanUpSwapChain()
{
    delete []swapChainImageViews;
    swapChain = VK_NULL_HANDLE;

}

bool RecreateSwapChain()
{
    // CleanUpSwapChain();
    
    vkDeviceWaitIdle(vkDevice);
    VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.pNext = nullptr;
    swapChainCreateInfo.flags = 0;
    swapChainCreateInfo.surface = surface;
    swapChainCreateInfo.minImageCount = imageCount;
    swapChainCreateInfo.imageFormat = surfaceFormat.format;
    swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapChainCreateInfo.imageExtent = extent;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapChainCreateInfo.queueFamilyIndexCount = 0;
    swapChainCreateInfo.pQueueFamilyIndices = nullptr;
    swapChainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainCreateInfo.presentMode = presentMode;
    swapChainCreateInfo.clipped = true;
    swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    u32 queueFamilyIndices[] = {(u32)graphicsQueueIndex, (u32)presentQueueIndex};
    if (graphicsQueueIndex != presentQueueIndex) 
    {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapChainCreateInfo.queueFamilyIndexCount = 2;
        swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    }

    VkResult vkResult = vkCreateSwapchainKHR(vkDevice, &swapChainCreateInfo, nullptr, &swapChain);
    if (vkResult != VK_SUCCESS)
    {
        OutputDebugString("Could not create swap chain.\n");
        return false;
    }

    vkResult = vkGetSwapchainImagesKHR(vkDevice, swapChain, &swapChainImagesCount, nullptr);
    if (vkResult != VK_SUCCESS || swapChainImagesCount == 0)
    {
        OutputDebugString("Could not determine amount of swap chain images.\n");
        return false;
    }
    swapChainImages = new VkImage[swapChainImagesCount];
    vkResult = vkGetSwapchainImagesKHR(vkDevice, swapChain, &swapChainImagesCount, swapChainImages);
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;

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
    imageViewCreateInfo.format = swapChainImageFormat;
    imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.subresourceRange = imageSubresourceRanger;

    swapChainImageViews = new VkImageView[swapChainImagesCount];
    for (u32 i = 0; i < swapChainImagesCount; i++)
    {
        VkImage currentImage = swapChainImages[i];
        imageViewCreateInfo.image = currentImage;

         vkResult = vkCreateImageView(vkDevice, &imageViewCreateInfo, nullptr, &swapChainImageViews[i]);
         if (vkResult != VK_SUCCESS)
         {
            OutputDebugString("Failed to create an image view for swap buffer image");
            return false;
         }
    }

    return true;
}

void RecordCommandBuffer(VkCommandBuffer commandBuffer, u32 imageIndex)
{
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
    barrier.image = swapChainImages[imageIndex];
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
    attachmentInfo.imageView = swapChainImageViews[imageIndex];
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
    renderArea.extent = swapChainExtent;
    renderingInfo.renderArea = renderArea;
    renderingInfo.layerCount = 1;
    renderingInfo.viewMask = 0;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &attachmentInfo;
    renderingInfo.pDepthAttachment = nullptr;
    renderingInfo.pStencilAttachment = nullptr;

    vkCmdBeginRendering(commandBuffer, &renderingInfo);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    
    VkViewport viewport = {};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = swapChainExtent.width;
    viewport.height = swapChainExtent.height;
    viewport.minDepth = 0;
    viewport.maxDepth = 1;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    
    VkRect2D scissorRect = {};
    scissorRect.offset.x = 0;
    scissorRect.offset.y = 0;
    scissorRect.extent = swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRendering(commandBuffer);
    barrier = {};
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
    barrier.image = swapChainImages[imageIndex];
    imageSubresourceName = {};
    imageSubresourceName.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageSubresourceName.baseMipLevel = 0;
    imageSubresourceName.levelCount = 1;
    imageSubresourceName.baseArrayLayer = 0;
    imageSubresourceName.layerCount = 1;
    barrier.subresourceRange = imageSubresourceName;

    dependencyInfo = {};
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
}


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebgCallback(
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
    const char* CLASS_NAME  = "Sample Window Class";
    int x = 10;
    
    WNDCLASSA wc = { };

    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(
        0,                              // Optional window styles.
        CLASS_NAME,                     // Window class
        "Learn to Program Windows",    // Window text
        WS_OVERLAPPEDWINDOW,            // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

        NULL,       // Parent window    
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
        );

    if (hwnd == NULL)
    {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

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
        return -1;
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

    VkInstance vkInstance;
    vkResult = vkCreateInstance(&vkInstanceCreateInfo, nullptr, &vkInstance);

    if (vkResult != VK_SUCCESS)
    {
        OutputDebugString("Could not create Vulkan Instance\n");
        return -1;
    }
    OutputDebugString("Created Vulkan Instance\n");

    VkWin32SurfaceCreateInfoKHR win32SurfaceCreateInfo{};
    win32SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    win32SurfaceCreateInfo.pNext = nullptr;
    win32SurfaceCreateInfo.flags = 0;
    win32SurfaceCreateInfo.hwnd = hwnd;
    win32SurfaceCreateInfo.hinstance = GetModuleHandle(nullptr);

    vkResult = vkCreateWin32SurfaceKHR(vkInstance, &win32SurfaceCreateInfo, nullptr, &surface);
    if (vkResult != VK_SUCCESS)
    {
        OutputDebugString("Could not create win32 surface\n");
        return -1;
    }
    
    PFN_vkCreateDebugUtilsMessengerEXT pfnCreateDebugUtilsMessengerEXT = nullptr;
    pfnCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vkInstance, "vkCreateDebugUtilsMessengerEXT");    
    if (!pfnCreateDebugUtilsMessengerEXT)
    {
        OutputDebugString("Could not get function vkCreateDebugUtilsMessengerEXT\n");
        return -1;
    }
    PFN_vkDestroyDebugUtilsMessengerEXT pfnDestroyDebugUtilsMessengerEXT = nullptr;
    pfnDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vkInstance, "vkDestroyDebugUtilsMessengerEXT");
    if (!pfnDestroyDebugUtilsMessengerEXT)
    {
        OutputDebugString("Could not get function vkDestroyDebugUtilsMessengerEXT\n");
        return -1;
    }

    VkDebugUtilsMessengerCreateInfoEXT vkDebugUtilsMessengerCreateInfoEXT;
    vkDebugUtilsMessengerCreateInfoEXT.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    vkDebugUtilsMessengerCreateInfoEXT.pNext = nullptr;
    vkDebugUtilsMessengerCreateInfoEXT.flags = 0;
    vkDebugUtilsMessengerCreateInfoEXT.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | 
                                                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    vkDebugUtilsMessengerCreateInfoEXT.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
                                                     VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    vkDebugUtilsMessengerCreateInfoEXT.pfnUserCallback = VulkanDebgCallback;
    vkDebugUtilsMessengerCreateInfoEXT.pUserData = nullptr;
    VkDebugUtilsMessengerEXT vKDebugUtilsMessengerExt;
    vkResult = pfnCreateDebugUtilsMessengerEXT(vkInstance, &vkDebugUtilsMessengerCreateInfoEXT, nullptr, &vKDebugUtilsMessengerExt);

    if (vkResult != VK_SUCCESS)
    {
        OutputDebugString("Could not create debug util messenger\n");
        return -1;
    }
    OutputDebugString("Created debug util messenger\n");

    u32 vkPhysicalDeviceCount;
    vkEnumeratePhysicalDevices(vkInstance, &vkPhysicalDeviceCount, nullptr);
    
    if (vkPhysicalDeviceCount == 0)
    {
        OutputDebugString("Could not find any physical devices\n");
        return -1;
    }
    
    VkPhysicalDevice* vkPhysicalDevices = new VkPhysicalDevice[vkPhysicalDeviceCount];
    vkResult = vkEnumeratePhysicalDevices(vkInstance, &vkPhysicalDeviceCount, vkPhysicalDevices);

    if (vkResult != VK_SUCCESS)
    {
        OutputDebugString("Could not enumerate physical devices\n");
        return -1;
    }

    const char* deviceExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkPhysicalDevice physicalDevice = nullptr;
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

        physicalDevice = vkPhysicalDevice;
        break;
    }

    delete[] vkPhysicalDevices;

    if (physicalDevice == nullptr)
    {
        OutputDebugString("Could not find sutiable physical devices\n");
        return -1;
    }

    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);    
    VkQueueFamilyProperties* queueFamilies = new VkQueueFamilyProperties[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies);

    for (s32 i = 0; i < queueFamilyCount; i++)
    {
        bool hasGraphics = (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

        if (hasGraphics && graphicsQueueIndex == -1)
            graphicsQueueIndex = i;
        if (presentSupport == VK_TRUE && presentQueueIndex == -1)
            presentQueueIndex = i;
        if (hasGraphics && presentSupport == VK_TRUE)
        {
            graphicsQueueIndex = i;
            presentQueueIndex = i;
        }
    }
    delete[] queueFamilies;

    if (graphicsQueueIndex == -1)
    {
        OutputDebugString("Could not find graphics queue index\n");
        return -1;
    }
    if (presentQueueIndex == -1)
    {
        OutputDebugString("Could not find present queue index\n");
        return -1;
    }

    float queuePriority = 0.5;
    VkDeviceQueueCreateInfo graphicsQueueCreateInfo;
    graphicsQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    graphicsQueueCreateInfo.pNext = nullptr;
    graphicsQueueCreateInfo.flags = 0;
    graphicsQueueCreateInfo.queueFamilyIndex = graphicsQueueIndex;
    graphicsQueueCreateInfo.queueCount = 1;
    graphicsQueueCreateInfo.pQueuePriorities = &queuePriority;

    VkDeviceQueueCreateInfo queueCreateInfos[2];
    uint32_t queueCreateInfoCount = 0;
    if (graphicsQueueIndex == presentQueueIndex) 
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
        presentQueueCreateInfo.queueFamilyIndex = presentQueueIndex;
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
    vkResult = vkCreateDevice(physicalDevice, &vkDeviceCreateInfo, nullptr, &vkDevice);

    if (vkResult != VK_SUCCESS)
    {
        OutputDebugString("Could not create logical device\n");
        return -1;
    }

    VkQueue graphicsQueue = nullptr;
    VkQueue presentQueue = nullptr;
    vkGetDeviceQueue(vkDevice, graphicsQueueIndex, 0, &graphicsQueue);
    vkGetDeviceQueue(vkDevice, presentQueueIndex, 0, &presentQueue);

    if (!graphicsQueue)
    {
        OutputDebugString("Could not get graphics queue\n");
        return -1;
    }
    if (!presentQueue)
    {
        OutputDebugString("Could not get present queue\n");
        return -1;
    }

    u32 surfaceFormatCount = 0;
    vkResult = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, nullptr);
    if (vkResult != VK_SUCCESS || surfaceFormatCount == 0)
    {
        OutputDebugString("Could not get surface formats\n");
        return -1;
    }
    VkSurfaceFormatKHR* surfaceFormats = new VkSurfaceFormatKHR[surfaceFormatCount];
    vkResult = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, surfaceFormats);

    bool foundGoodSurfaceFormat = false;
    for (u32 i = 0; i < surfaceFormatCount; i++)
    {
        VkSurfaceFormatKHR* currentFormat = &surfaceFormats[i];
        if (currentFormat->format ==VK_FORMAT_R8G8B8A8_SRGB && currentFormat->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            foundGoodSurfaceFormat = true;
            surfaceFormat = *currentFormat;
            break;
        }
    }

    if (!foundGoodSurfaceFormat)
        surfaceFormat = surfaceFormats[0];

    vkResult = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);
    if (vkResult != VK_SUCCESS)
    {
        OutputDebugString("Could not get physical device surface capabilities\n");
        return -1;
    }

    if (surfaceCapabilities.currentExtent.width != UINT32_MAX)
    {
        extent = surfaceCapabilities.currentExtent;
    }
    else
    {
        RECT rect;
        GetClientRect(hwnd, &rect);
        u32 width  = u32(rect.right  - rect.left);
        u32 height = u32(rect.bottom - rect.top);

        extent.width = Clamp(width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
        extent.height = Clamp(height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
    }

    imageCount = surfaceCapabilities.minImageCount + 1;
    imageCount = Clamp(imageCount, surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount);
    if (!RecreateSwapChain()) 
    {
        OutputDebugString("Failed to create swap chain\n");
        return -1;
    }

    char* shaderFileBytes = nullptr;
    st shaderFileSize = 0;
    bool result = ReadEntireFile("slang.spv", &shaderFileBytes, &shaderFileSize);
    if (!result)
    {
        OutputDebugString("Failed to read shader file\n");
        return -1;
    }

    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.pNext = nullptr;
    shaderModuleCreateInfo.flags = 0;
    shaderModuleCreateInfo.codeSize = shaderFileSize;
    shaderModuleCreateInfo.pCode = (u32*)shaderFileBytes;

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    vkResult = vkCreateShaderModule(vkDevice, &shaderModuleCreateInfo, nullptr, &shaderModule);
    if (vkResult != VK_SUCCESS)
    {
        OutputDebugString("Failed to create shader module\n");
        return -1;
    }

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.pNext = nullptr;
    vertShaderStageInfo.flags = 0;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = shaderModule;
    vertShaderStageInfo.pName = "vertMain";
    vertShaderStageInfo.pSpecializationInfo = nullptr;

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.pNext = nullptr;
    fragShaderStageInfo.flags = 0;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = shaderModule;
    fragShaderStageInfo.pName = "fragMain";
    fragShaderStageInfo.pSpecializationInfo = nullptr;

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        vertShaderStageInfo, fragShaderStageInfo
    };

    VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = {};
    pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipelineVertexInputStateCreateInfo.pNext = nullptr;
    pipelineVertexInputStateCreateInfo.flags = 0;
    pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;
    pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = nullptr;
    pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
    pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = nullptr;

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

    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.pNext = nullptr;
    pipelineLayoutCreateInfo.flags = 0;
    pipelineLayoutCreateInfo.setLayoutCount = 0;
    pipelineLayoutCreateInfo.pSetLayouts = nullptr;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
    vkResult = vkCreatePipelineLayout(vkDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);
    if (vkResult != VK_SUCCESS)
    {
        OutputDebugString("Could not create pipeline layout");
        return -1;
    }

    VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo = {};
    pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingCreateInfo.pNext = nullptr;
    pipelineRenderingCreateInfo.viewMask = 0;
    pipelineRenderingCreateInfo.colorAttachmentCount = 1;
    pipelineRenderingCreateInfo.pColorAttachmentFormats = &swapChainImageFormat;
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
    graphicsPipelineCreateInfo.layout = pipelineLayout;
    graphicsPipelineCreateInfo.renderPass = VK_NULL_HANDLE;
    graphicsPipelineCreateInfo.subpass = 0;
    graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    graphicsPipelineCreateInfo.basePipelineIndex = -1;

    vkResult = vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &graphicsPipeline);
    if (vkResult != VK_SUCCESS)
    {
        OutputDebugString("Could not create graphics pipeline");
        return -1;
    }

    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.pNext = nullptr;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCreateInfo.queueFamilyIndex = graphicsQueueIndex;
    vkResult = vkCreateCommandPool(vkDevice, &commandPoolCreateInfo, nullptr, &commandPool);
    if (vkResult != VK_SUCCESS)
    {
        OutputDebugString("Could not create command pool");
        return -1;
    }

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.pNext = nullptr;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
    
    vkResult = vkAllocateCommandBuffers(vkDevice, &commandBufferAllocateInfo, commandBuffers);
    if (vkResult != VK_SUCCESS)
    {
        OutputDebugString("Failed to allocate command buffer.");
        return -1;
    }

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphore acquireSemaphores[MAX_FRAMES_IN_FLIGHT];
    VkFence frameFences[MAX_FRAMES_IN_FLIGHT];
    
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (   
            vkCreateSemaphore(vkDevice, &semaphoreInfo, nullptr, &acquireSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(vkDevice, &fenceInfo, nullptr, &frameFences[i]) != VK_SUCCESS) 
        {
            OutputDebugString("failed to create semaphores!");
            return -1;
        }
    }
    
    VkSemaphore* submitSemaphores = new VkSemaphore[swapChainImagesCount];
    for (u32 i = 0; i < swapChainImagesCount; i++)
    {
        if (vkCreateSemaphore(vkDevice, &semaphoreInfo, nullptr, &submitSemaphores[i]))
        {
            OutputDebugString("failed to create semaphores!");
            return -1;
        }
    }

    u32 currentFrameInFlightIndex = 0;
        
    MSG msg = { };
    bool running = true;
    while (running)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            vkWaitForFences(vkDevice, 1, &frameFences[currentFrameInFlightIndex], VK_TRUE, UINT64_MAX);
            vkResetFences(vkDevice, 1, &frameFences[currentFrameInFlightIndex]);
            
            u32 imageIndex;
            vkAcquireNextImageKHR(vkDevice, swapChain, UINT64_MAX, acquireSemaphores[currentFrameInFlightIndex], VK_NULL_HANDLE, &imageIndex);
            vkResetCommandBuffer(commandBuffers[currentFrameInFlightIndex], 0);
            RecordCommandBuffer(commandBuffers[currentFrameInFlightIndex], imageIndex);

            VkSemaphore submitSemaphore = submitSemaphores[imageIndex];

            VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
            VkSubmitInfo submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.pNext = nullptr;
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = &acquireSemaphores[currentFrameInFlightIndex];
            submitInfo.pWaitDstStageMask = waitStages;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffers[currentFrameInFlightIndex];
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = &submitSemaphore;
            vkQueueSubmit(graphicsQueue, 1, &submitInfo, frameFences[currentFrameInFlightIndex]);

            VkPresentInfoKHR presentInfo = {};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.pNext = nullptr;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = &submitSemaphore;
            VkSwapchainKHR swapChains[] = {swapChain};
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = swapChains;
            presentInfo.pImageIndices = &imageIndex;
            presentInfo.pResults = nullptr;
            vkResult = vkQueuePresentKHR(presentQueue, &presentInfo);

            currentFrameInFlightIndex = (currentFrameInFlightIndex + 1) % MAX_FRAMES_IN_FLIGHT;
        }
    }   

    vkDeviceWaitIdle(vkDevice);
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (acquireSemaphores[i] != VK_NULL_HANDLE)
            vkDestroySemaphore(vkDevice, acquireSemaphores[i], nullptr);
        if (frameFences[i] != VK_NULL_HANDLE)
            vkDestroyFence(vkDevice, frameFences[i], nullptr);
    }
    for (u32 i = 0; i < swapChainImagesCount; i++)
    {
        if (submitSemaphores[i] != VK_NULL_HANDLE)
            vkDestroySemaphore(vkDevice, submitSemaphores[i], nullptr);
    }
    vkDestroyPipeline(vkDevice, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(vkDevice, pipelineLayout, nullptr);
    vkDestroyCommandPool(vkDevice, commandPool, nullptr);
    vkDestroyShaderModule(vkDevice, shaderModule, nullptr);
    for (u32 i = 0; i < swapChainImagesCount; i++)
    {
        if (swapChainImageViews[i] != VK_NULL_HANDLE)
            vkDestroyImageView(vkDevice, swapChainImageViews[i], nullptr);
    }
    vkDestroySwapchainKHR(vkDevice, swapChain, nullptr);
    vkDestroyDevice(vkDevice, NULL);
    vkDestroySurfaceKHR(vkInstance, surface, NULL);
    pfnDestroyDebugUtilsMessengerEXT(vkInstance, vKDebugUtilsMessengerExt, NULL);
    vkDestroyInstance(vkInstance, NULL);

    delete[] swapChainImageViews;
    delete[] swapChainImages;
    delete[] shaderFileBytes;

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            // All painting occurs here, between BeginPaint and EndPaint.

            FillRect(hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));

            EndPaint(hwnd, &ps);
        }
        return 0;

    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}