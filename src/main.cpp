#include "mcy_helpers.h"
#include "vulkan_layer.cpp"

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

    if (_hwnd == NULL)
    {
        return 0;
    }

    if (!VkmInitialize())
    {
        OutputDebugString("Could not intiialize Vulkan");
        return -1;
    }

    const Vertex vertices[] = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
    };

    const u16 indices[] = {
        0, 1, 2, 2, 3, 0
    };

    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;

    VkDeviceSize vertexBufferSize = sizeof(vertices);
    VkBuffer stagingBufferForVertices = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferForVerticesMemory = VK_NULL_HANDLE;
    if (!VkmCreateAndFillBuffer(vertexBufferSize, (void*)vertices, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBufferForVertices, &stagingBufferForVerticesMemory))
    {
        OutputDebugString("Could not create vertex staging buffer\n.");
        return false;
    }
    
    VkDeviceMemory vertexBufferForVerticesMemory = VK_NULL_HANDLE;
    if (!VkmCreateBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertexBuffer, &vertexBufferForVerticesMemory))
    {
        OutputDebugString("Could not create vertex staging buffer\n.");
        return false;
    }
    VkmCopyBuffer(stagingBufferForVertices, vertexBuffer, vertexBufferSize);
    
    VkDeviceSize indexBufferSize = sizeof(indices);
    VkBuffer stagingBufferForIndices = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferForIndicesMemory = VK_NULL_HANDLE;
    if (!VkmCreateAndFillBuffer(indexBufferSize, (void*)indices, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBufferForIndices, &stagingBufferForIndicesMemory))
    {
        OutputDebugString("Could not create index staging buffer\n.");
        return false;
    }
    
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
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
        
        // u32 imageIndex;
        if (!VkmSetupForFrameRendering(vkm.commandBuffers[vkm.currentFrameInFlightIndex]))
        {
            OutputDebugString("Could not set up frame for rendering.");
            break;
        }
        VkCommandBuffer commandBuffer = VkmGetInFlightCommandBuffer();

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
    VkmCleanUp();
    OutputDebugString("Cleaned up Vulkan");

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