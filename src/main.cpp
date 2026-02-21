#include <iostream>
#include <vector>
#include <string>

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

uint32_t pupp = 0;
#define ARRAY_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebgCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT   messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT          messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void*                                    pUserData)
{
    char messageBuffer[256];
    sprintf_s(messageBuffer, 256, "Vulkan validation layer: %s\n", pCallbackData->pMessage);

    return VK_FALSE;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
    const char* CLASS_NAME  = "Sample Window Class";
    
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

    VkSurfaceKHR surface = nullptr;
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

    VkPhysicalDevice chosenDevice = nullptr;
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

        chosenDevice = vkPhysicalDevice;
        break;
    }

    delete[] vkPhysicalDevices;

    if (chosenDevice == nullptr)
    {
        OutputDebugString("Could not find sutiable physical devices\n");
        return -1;
    }

    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(chosenDevice, &queueFamilyCount, NULL);    
    VkQueueFamilyProperties* queueFamilies = new VkQueueFamilyProperties[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(chosenDevice, &queueFamilyCount, queueFamilies);
    s32 graphicsIndex = -1;
    s32 presentIndex = -1;

    for (s32 i = 0; i < queueFamilyCount; i++)
    {
        bool hasGraphics = (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(chosenDevice, i, surface, &presentSupport);

        if (hasGraphics && graphicsIndex == -1)
            graphicsIndex = i;
        if (presentSupport == VK_TRUE && presentIndex == -1)
            presentIndex = i;
        if (hasGraphics && presentSupport == VK_TRUE)
        {
            graphicsIndex = i;
            presentIndex = i;
        }
            
    }
    delete[] queueFamilies;

    if (graphicsIndex == -1)
    {
        OutputDebugString("Could not find graphics queue index\n");
        return -1;
    }
    if (presentIndex == -1)
    {
        OutputDebugString("Could not find present queue index\n");
        return -1;
    }


    float queuePriority = 0.5;
    
    VkDeviceQueueCreateInfo graphicsQueueCreateInfo;
    graphicsQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    graphicsQueueCreateInfo.pNext = nullptr;
    graphicsQueueCreateInfo.flags = 0;
    graphicsQueueCreateInfo.queueFamilyIndex = graphicsIndex;
    graphicsQueueCreateInfo.queueCount = 1;
    graphicsQueueCreateInfo.pQueuePriorities = &queuePriority;

    VkDeviceQueueCreateInfo queueCreateInfos[2];
    uint32_t queueCreateInfoCount = 0;
    if (graphicsIndex == presentIndex) 
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
        presentQueueCreateInfo.queueFamilyIndex = presentIndex;
        presentQueueCreateInfo.queueCount = 1;
        presentQueueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos[1] = presentQueueCreateInfo;
        queueCreateInfoCount = 2;
    }

    VkPhysicalDeviceFeatures2 vkPhysicalDeviceFeatures2;
    vkPhysicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    vkPhysicalDeviceFeatures2.pNext = nullptr;
    vkPhysicalDeviceFeatures2.features = {};

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
    vkPhysicalDeviceVulkan13Features.synchronization2 = false;
    vkPhysicalDeviceVulkan13Features.textureCompressionASTC_HDR = false;
    vkPhysicalDeviceVulkan13Features.shaderZeroInitializeWorkgroupMemory = false;
    vkPhysicalDeviceVulkan13Features.dynamicRendering = true;
    vkPhysicalDeviceVulkan13Features.shaderIntegerDotProduct = false;
    vkPhysicalDeviceVulkan13Features.maintenance4 = false;

    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT vkPhysicalDeviceExtendedDynamicStateFeaturesEXT;
    vkPhysicalDeviceExtendedDynamicStateFeaturesEXT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
    vkPhysicalDeviceExtendedDynamicStateFeaturesEXT.pNext = nullptr;
    vkPhysicalDeviceExtendedDynamicStateFeaturesEXT.extendedDynamicState = true;

    vkPhysicalDeviceFeatures2.pNext = &vkPhysicalDeviceVulkan13Features;
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

    VkDevice vkDevice;
    vkResult = vkCreateDevice(chosenDevice, &vkDeviceCreateInfo, nullptr, &vkDevice);

    if (vkResult != VK_SUCCESS)
    {
        OutputDebugString("Could not create logical device\n");
        return -1;
    }

    VkQueue graphicsQueue = nullptr;
    VkQueue presentQueue = nullptr;
    vkGetDeviceQueue(vkDevice, graphicsIndex, 0, &graphicsQueue);
    vkGetDeviceQueue(vkDevice, presentIndex, 0, &presentQueue);

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
        }
    }   

    vkDestroySurfaceKHR(vkInstance, surface, NULL);
    pfnDestroyDebugUtilsMessengerEXT(vkInstance, vKDebugUtilsMessengerExt, NULL);
    vkDestroyDevice(vkDevice, NULL);
    vkDestroyInstance(vkInstance, NULL);

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