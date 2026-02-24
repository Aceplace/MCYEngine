#define WIN32_MEAN_AND_LEAN
#include <windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include "mcy_helpers.h"

HWND _hwnd = NULL;
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
bool framebufferResized = false;

bool RecreateSwapChain()
{
    OutputDebugString("Creating a new swap chain.");
    VkSwapchainKHR oldSwapChain = swapChain;
    if (swapChainImageViews)
    {
        for (u32 i = 0; i < swapChainImagesCount; i++)
        {
            if (swapChainImageViews[i] != VK_NULL_HANDLE)
            vkDestroyImageView(vkDevice, swapChainImageViews[i], nullptr);
        }
        delete[] swapChainImageViews;
    }
    swapChainImagesCount = 0;

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
    swapChainCreateInfo.oldSwapchain = oldSwapChain;

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

    if (oldSwapChain != VK_NULL_HANDLE && oldSwapChain != swapChain)
    {
        vkDestroySwapchainKHR(vkDevice, oldSwapChain, nullptr);
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