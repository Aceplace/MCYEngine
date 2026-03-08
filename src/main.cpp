
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "mcy_helpers.h"
#include "vulkan_layer.cpp"
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
    const char* CLASS_NAME  = "Sample Window Class";
    int x = 10;
    
    WNDCLASSA wc = { };

    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClassA(&wc);

    _hwnd = CreateWindowExA(
        0,                             
        CLASS_NAME,                    
        "Learn to Program Windows",    
        WS_OVERLAPPEDWINDOW,           

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

        NULL,      
        NULL,      
        hInstance, 
        NULL       
        );

    if (_hwnd == NULL)
    {
        return 0;
    }

    if (!VkmInitialize())
    {
        OutputDebugString("Could not intiialize Vulkan");
        return -1;
    }

    // const Vertex vertices[] = {
    //     {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    //     {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    //     {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    //     {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
    // };
    
    const Vertex vertices[] = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
    };

    const u16 indices[] = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4
    };
    
    VkBuffer stagingBufferForVertices = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferForVerticesMemory = VK_NULL_HANDLE;
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferForVerticesMemory = VK_NULL_HANDLE;
    VkDeviceSize vertexBufferSize = sizeof(vertices);
    if (!VkmCreateAndFillBuffer(vertexBufferSize, (void*)vertices, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBufferForVertices, &stagingBufferForVerticesMemory))
    {
        OutputDebugString("Could not create vertex staging buffer\n.");
        return false;
    }
    
    if (!VkmCreateBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertexBuffer, &vertexBufferForVerticesMemory))
    {
        OutputDebugString("Could not create vertex staging buffer\n.");
        return false;
    }
    VkmCopyBuffer(stagingBufferForVertices, vertexBuffer, vertexBufferSize);

    VkBuffer stagingBufferForIndices = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferForIndicesMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
    VkDeviceSize indexBufferSize = sizeof(indices);
    if (!VkmCreateAndFillBuffer(indexBufferSize, (void*)indices, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBufferForIndices, &stagingBufferForIndicesMemory))
    {
        OutputDebugString("Could not create index staging buffer\n.");
        return false;
    }
    
    if (!VkmCreateBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &indexBuffer, &indexBufferMemory))
    {
        OutputDebugString("Could not create index staging buffer\n.");
        return false;
    }
    VkmCopyBuffer(stagingBufferForIndices, indexBuffer, indexBufferSize);

    ShowWindow(_hwnd, nCmdShow);        
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

        if (!running) break;

        static std::chrono::steady_clock::time_point startTime = std::chrono::high_resolution_clock::now();

        std::chrono::steady_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        UniformBufferObject ubo = {};
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), static_cast<float>(vkm.swapChainExtent.width) / static_cast<float>(vkm.swapChainExtent.height), 0.1f, 10.0f);
        ubo.proj[1][1] *= -1;
        memcpy(vkm.uniformBuffersMapped[vkm.currentFrameInFlightIndex], &ubo, sizeof(ubo));
        
        if (!VkmSetupForFrameRendering(vkm.commandBuffers[vkm.currentFrameInFlightIndex]))
        {
            OutputDebugString("Could not set up frame for rendering.");
            break;
        }
        
        VkCommandBuffer commandBuffer = VkmGetInFlightCommandBuffer();

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkm.pipelineLayout, 0, 1, &vkm.descriptorSets[vkm.currentFrameInFlightIndex], 0, nullptr); 

        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(commandBuffer, ARRAY_COUNT(indices), 1, 0, 0, 0);

        if (!VkmEndRenderingAndSetupForPresent(vkm.commandBuffers[vkm.currentFrameInFlightIndex]))
        {
            OutputDebugString("Could not set up frame for rendering.");
            break;
        }
    }   

    if (vkm.vkDevice != VK_NULL_HANDLE)
        vkDeviceWaitIdle(vkm.vkDevice);
    vkDestroyBuffer(vkm.vkDevice, vertexBuffer, nullptr);
    vkFreeMemory(vkm.vkDevice, vertexBufferForVerticesMemory, nullptr);
    vkDestroyBuffer(vkm.vkDevice, stagingBufferForVertices, nullptr);
    vkFreeMemory(vkm.vkDevice, stagingBufferForVerticesMemory, nullptr);
    
    vkDestroyBuffer(vkm.vkDevice, indexBuffer, nullptr);
    vkFreeMemory(vkm.vkDevice, indexBufferMemory, nullptr);
    vkDestroyBuffer(vkm.vkDevice, stagingBufferForIndices, nullptr);
    vkFreeMemory(vkm.vkDevice, stagingBufferForIndicesMemory, nullptr);
    VkmCleanUp();

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_SIZE:
        vkm.framebufferResized = true;
        break;

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