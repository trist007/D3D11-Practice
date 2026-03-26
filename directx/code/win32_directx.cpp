#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "graphics.cpp"

// Ignore Warnings
#pragma warning(disable:4700)

// globals
bool gPaused = false;

// ======================================================================================
// WIN32
// ======================================================================================

LRESULT CALLBACK
WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_CLOSE:
        {
            PostQuitMessage(17);
        } break;
        
        case WM_KEYDOWN:
        {
            if(wParam == 'F')
            {
                SetWindowText(hwnd, "Sick");
            }
            if(wParam == VK_SPACE)
                gPaused = !gPaused;
        } break;
        case WM_KEYUP:
        {
            if(wParam == 'F')
            {
                SetWindowText(hwnd, "DirectX Tutorial");
            }
        } break;
        /*
        case WM_CHAR:
        {
            char buf[32];
            wsprintfA(buf, "char %c was typed\n", (char)wParam);
            OutputDebugStringA(buf);
        } break;
*/
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// ======================================================================================
// ENTRY POINT
// ======================================================================================

int
CALLBACK WinMain(
                 HINSTANCE hInstance,
                 HINSTANCE hPrevInstance,
                 LPSTR     lpCmdLine,
                 int       nCmdShow)
{
    UINT WIDTH = 640;
    UINT HEIGHT = 480;
    
    char *pClassName = "directx";
    // Register window class
    WNDCLASSEX wc    = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_OWNDC;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = 0;
    wc.hCursor       = 0;
    wc.hbrBackground = 0;
    wc.lpszMenuName  = 0;
    wc.lpszClassName = pClassName;
    wc.hIconSm       = 0;
    
    RegisterClassEx(&wc);
    
    // Create window
    HWND hwnd = CreateWindowEx(
                               0, pClassName,
                               "DirectX Tutorial",
                               WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,
                               200, 200, WIDTH, HEIGHT,
                               0, 0, hInstance, 0
                               );
    
    ShowWindow(hwnd, SW_SHOW);
    
#ifdef _DEBUG
    DxgiInfoManagerInit(&gInfoManager);
#endif
    
    // One-time init
    Renderer r = {};
    RendererInit(&r, hwnd, WIDTH, HEIGHT);
    
    // Cube mesh
    Vertex vertices[] =
    {
        {-1.0f, -1.0f, -1.0f,    0.0f, 0.0f},
        { 1.0f, -1.0f, -1.0f,    1.0f, 0.0f},
        {-1.0f,  1.0f, -1.0f,    0.0f, 1.0f},
        { 1.0f,  1.0f, -1.0f,    1.0f, 1.0f},
        {-1.0f, -1.0f,  1.0f,    0.0f, 0.0f},
        { 1.0f, -1.0f,  1.0f,    1.0f, 0.0f},
        {-1.0f,  1.0f,  1.0f,    0.0f, 1.0f},
        { 1.0f,  1.0f,  1.0f,    1.0f, 1.0f},
    };
    unsigned short indices[] =
    {
        0,2,1, 2,3,1,
        1,3,5, 3,7,5,
        2,6,3, 3,6,7,
        4,5,7, 4,7,6,
        0,4,2, 2,4,6,
        0,1,4, 1,5,4,
    };
    Mesh cube = {};
    MeshInit(&r, &cube, vertices, ArrayCount(vertices), indices, ArrayCount(indices));
    
    ShaderPipeline pipeline = {};
    ShaderPipelineInit(&r, &pipeline,
                       L"../directx/code/shaders/vertex.cso",
                       L"../directx/code/shaders/pixel.cso");
    
    ConstantBuffers cb = {};
    ConstantBuffersInit(&r, &cb);
    
    DirectX::XMMATRIX projection = MakeProjection((float)WIDTH, (float)HEIGHT, 0.5f, 10.0f);
    
    Texture tex = {};
    TextureInit(&r, &tex, L"../directx/code/textures/kappa50.png", 0u);
    
    Sampler sampler = {};
    SamplerInit(&r, &sampler, SAMPLER_BILINEAR, false, 0u);
    
    Timer timer;
    TimerInit(&timer);
    
    // Frame loop
    MSG msg;
    while(1)
    {
        while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            if(msg.message == WM_QUIT)
            {
                // don't need to release em as OS will
                MeshRelease(&cube);
                ShaderPipelineRelease(&pipeline);
                ConstantBuffersRelease(&cb);
                TextureRelease(&tex);
                SamplerRelease(&sampler);
                
                return((int)msg.wParam);
            }
            
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        float t = TimerPeek(&timer);
        float c = sinf(t) / 2.0f + 0.5f;
        
        float draw_t = gPaused ? 0.0f : t;
        
        RendererClear(&r, c, c, 1.0f);
        
        POINT mouse;
        GetCursorPos(&mouse);
        ScreenToClient(hwnd, &mouse);
        
        TextureBind(&r, &tex);
        SamplerBind(&r, &sampler);
        
        //DrawCube(&r, &cube, &pipeline, &cb, -t, 0.0f, 0.0f, projection);
        DrawCube(&r, &cube, &pipeline, &cb,  draw_t,
                 (float)mouse.x / 320.0f - 1.0f,
                 ((float)mouse.y / 240.0f - 1.0f) * -1.0f,
                 projection);
        
        RendererPresent(&r);
    }
}