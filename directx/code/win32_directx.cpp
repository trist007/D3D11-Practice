#define IMGUI_DEFINE_MATH_OPERATORS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "graphics.cpp"

#include "imgui/imgui.cpp"
#include "imgui/imgui_impl_win32.cpp"
#include "imgui/imgui_impl_dx11.cpp"
#include "imgui/imgui_demo.cpp"
#include "imgui/imgui_draw.cpp"
#include "imgui/imgui_tables.cpp"
#include "imgui/imgui_widgets.cpp"

#include "camera.cpp"
#include "sheet.cpp"

// Ignore Warnings
#pragma warning(disable:4700)

#define SHEET_COUNT 80

// globals
bool gPaused = false;
bool gImguiEnabled = true;

struct Parameters
{
    float speed_factor;
};


// ======================================================================================
// WIN32
// ======================================================================================

LRESULT CALLBACK
WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if(ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
    {
        return(true);
    }
    
    switch(msg)
    {
        case WM_CLOSE:
        {
            PostQuitMessage(17);
        } break;
        
        case WM_KEYDOWN:
        {
            if(ImGui::GetIO().WantCaptureKeyboard)
            {
                break;
            }
            if(wParam == 'P')
            {
                SetWindowText(hwnd, "Sick");
                gPaused = !gPaused;
            }
            if(wParam == VK_SPACE)
                gImguiEnabled = !gImguiEnabled;;
        } break;
        case WM_KEYUP:
        {
            if(ImGui::GetIO().WantCaptureKeyboard)
            {
                break;
            }
            if(wParam == 'F')
            {
                SetWindowText(hwnd, "DirectX Tutorial");
            }
        } break;
        
        /*
                case WM_CHAR:
                {
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
    UINT WIDTH  = 1024;
    UINT HEIGHT = 768;
    
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
    Parameters param = {};
    Camera camera = {};
    param.speed_factor = 1.0f;
    Renderer r = {};
    RendererInit(&r, hwnd, WIDTH, HEIGHT);
    CameraInit(&camera);
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(r.device, r.context);
    
    EnableImgui(&r);
    
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
    //Mesh cube = {};
    //MeshInit(&r, &cube, vertices, ArrayCount(vertices), indices, ArrayCount(indices));
    
    Vertex plane_verts[] =
    {
        {-1.0f, -1.0f, 0.0f,   0.0f, 0.0f},
        { 1.0f, -1.0f, 0.0f,   1.0f, 0.0f},
        {-1.0f,  1.0f, 0.0f,   0.0f, 1.0f},
        { 1.0f,  1.0f, 0.0f,   1.0f, 1.0f},
    };
    unsigned short plane_indices[] =
    {
        0,2,1,
        1,2,3,
    };
    Mesh plane = {};
    MeshInit(&r, &plane, plane_verts, ArrayCount(plane_verts),
             plane_indices, ArrayCount(plane_indices));
    
    //ShaderPipeline pipeline = {};
    //ShaderPipelineInit(&r, &pipeline,
    //L"../directx/code/shaders/vertex.cso",
    //L"../directx/code/shaders/pixel.cso");
    
    ShaderPipeline sheet_pipeline = {};
    ShaderPipelineInit(&r, &sheet_pipeline,
                       L"../directx/code/shaders/vertex.cso",
                       L"../directx/code/shaders/sheet_pixel.cso");
    
    ConstantBuffers cb = {};
    ConstantBuffersInit(&r, &cb);
    
    //DirectX::XMMATRIX projection = MakeProjection((float)WIDTH, (float)HEIGHT, 0.5f, 40.0f);
    SetProjection(&r, DirectX::XMMatrixPerspectiveLH(1.0f, 3.0f / 4.0f, 0.5f, 40.0f));
    SetCamera(&r, DirectX::XMMatrixTranslation(0.0f, 0.0f, 20.0f));
    
    Texture tex = {};
    TextureInit(&r, &tex, L"../directx/code/textures/kappa50.png", 0u);
    
    Sampler sampler = {};
    SamplerInit(&r, &sampler, SAMPLER_BILINEAR, false, 0u);
    
    Timer timer;
    TimerInit(&timer);
    
    Sheet sheets[SHEET_COUNT];
    
    for(int i = 0; i < SHEET_COUNT; i++)
    {
        SheetInit(&sheets[i],
                  /* r     */ 6.0f + rand_float() * 4.0f,
                  /* droll */ rand_float() * 0.5f,
                  /* dpitch*/ rand_float() * 0.5f,
                  /* dyaw  */ rand_float() * 0.5f,
                  /* dphi  */ rand_float() * 0.3f,
                  /* dtheta*/ rand_float() * 0.3f,
                  /* dchi  */ rand_float() * 0.3f,
                  /* chi   */ rand_float() * DirectX::XM_2PI,
                  /* theta */ rand_float() * DirectX::XM_2PI,
                  /* phi   */ rand_float() * DirectX::XM_2PI,
                  rand_float(), rand_float(), rand_float(), 1.0f);
    }
    
    float last_t = TimerPeek(&timer);
    
    // Frame loop
    MSG msg;
    while(1)
    {
        while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            if(msg.message == WM_QUIT)
            {
                // don't need to release em as OS will
                //MeshRelease(&cube);
                //ShaderPipelineRelease(&pipeline);
                ShaderPipelineRelease(&sheet_pipeline);
                ConstantBuffersRelease(&cb);
                TextureRelease(&tex);
                SamplerRelease(&sampler);
                ImGui_ImplWin32_Shutdown();
                
                return((int)msg.wParam);
            }
            
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        float t = TimerPeek(&timer);
        float dt = t - last_t;
        dt *= param.speed_factor;
        last_t = t;
        
        
        
        float c = sinf(t) / 2.0f + 0.5f;
        float draw_t = gPaused ? 0.0f : t;
        
        if(!gPaused)
            dt *= param.speed_factor;
        else
            dt = 0.0f;
        
        //RendererClear(&r, c, c, 1.0f);
        r.imgui_enabled = gImguiEnabled;
        BeginFrame(&r, c, c, 1.0f);
        SetCamera(&r, CameraGetMatrix(&camera));
        
        POINT mouse;
        GetCursorPos(&mouse);
        ScreenToClient(hwnd, &mouse);
        
        TextureBind(&r, &tex);
        SamplerBind(&r, &sampler);
        
        //DrawCube(&r, &cube, &pipeline, &cb, -t, 0.0f, 0.0f, projection);
        
        for(int i = 0;
            i < SHEET_COUNT;
            i++)
        {
            SheetUpdate(&sheets[i], dt);
            SheetDraw(&r, &sheets[i], &plane, &sheet_pipeline, &cb, WIDTH, HEIGHT);
        }
        
        //DrawCube(&r, &cube, &pipeline, &cb,  draw_t,
        //(float)mouse.x / 320.0f - 1.0f,
        //((float)mouse.y / 240.0f - 1.0f) * -1.0f,
        //projection, WIDTH, HEIGHT);
        
        if(r.imgui_enabled)
        {
            
            CameraSpawnControlWindow(&camera);
            static bool show_demo_window = false;
            ImGui::SetNextWindowPos(ImVec2(30, 30), ImGuiCond_Once);
            
            if(show_demo_window)
                ImGui::ShowDemoWindow(&show_demo_window);
            
            static char buffer[1024] = {};
            
            // ImGui windows
            if(ImGui::Begin("Simulation Speed"))
            {
                ImGui::SliderFloat("Speed Factor", &param.speed_factor, 0.0f, 4.0f);
                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                            ImGui::GetIO().Framerate);
                ImGui::InputText("Butts", buffer, sizeof(buffer));
            }
            
            
            ImGui::End();
        }
        
        EndFrame(&r);
        
    }
}