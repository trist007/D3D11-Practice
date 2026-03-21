#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <math.h>

struct Timer
{
    LARGE_INTEGER start;
    LARGE_INTEGER frequency;
};

#pragma comment(lib, "d3d11.lib") // Tells Compiler to link to this library

// Main DirectX 11 Globals
ID3D11Device *pDevice = 0;
IDXGISwapChain *pSwap = 0;
ID3D11DeviceContext *pContext = 0;
ID3D11RenderTargetView *pTarget = 0;

void
InitD3D(HWND hwnd)
{
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 0;
    sd.BufferDesc.RefreshRate.Denominator = 0;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.SampleDesc.Count = 1; // Anti-aliasing
    sd.SampleDesc.Quality = 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 1; // Double buffering
    sd.OutputWindow = hwnd;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; // Vanilla
    sd.Flags = 0;
    
    D3D11CreateDeviceAndSwapChain(
                                  0,
                                  D3D_DRIVER_TYPE_HARDWARE,
                                  0,
                                  0,
                                  0,
                                  0,
                                  D3D11_SDK_VERSION,
                                  &sd,
                                  &pSwap,
                                  &pDevice,
                                  0,
                                  &pContext
                                  );
    
    // Gain access to texture subresource in swap chain (back buffer)
    ID3D11Resource *pBackBuffer = 0;
    pSwap->GetBuffer(0, __uuidof(ID3D11Resource), (void**)&pBackBuffer);
    pDevice->CreateRenderTargetView(pBackBuffer, 0, &pTarget);
    
    // No longer required only needed to create the RenderTargetView
    pBackBuffer->Release();
    
}

void
EndFrame()
{
    pSwap->Present(1u, 0u);
}

void
ClearBuffer(float red, float green, float blue)
{
    float color[] = { red, green, blue, 1.0f };
    pContext->ClearRenderTargetView(pTarget, color);
}

void
TimerInit(Timer *t)
{
    QueryPerformanceFrequency(&t->frequency);
    QueryPerformanceCounter(&t->start);
}

float
TimerPeek(Timer *t)
{
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return((float)(now.QuadPart - t->start.QuadPart) / (float)t->frequency.QuadPart);
}

LRESULT CALLBACK
WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_CLOSE:
        {
            PostQuitMessage(69);
        } break;
        
        case WM_KEYDOWN:
        {
            if(wParam == 'F')
            {
                SetWindowText(hwnd, "Sick");
            }
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


int
CALLBACK WinMain(
                 HINSTANCE hInstance,
                 HINSTANCE hPrevInstance,
                 LPSTR     lpCmdLine,
                 int       nCmdShow)
{
    const auto pClassName = "directx";
    // Register window class
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = 0;
    wc.hCursor = 0;
    wc.hbrBackground = 0;
    wc.lpszMenuName = 0;
    wc.lpszClassName = pClassName;
    wc.hIconSm = 0;
    
    RegisterClassEx(&wc);
    
    // Create window
    HWND hwnd = CreateWindowEx(
                               0, pClassName,
                               "DirectX Tutorial",
                               WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,
                               200, 200, 640, 480,
                               0, 0, hInstance, 0
                               );
    
    ShowWindow(hwnd, SW_SHOW);
    InitD3D(hwnd);
    Timer timer;
    TimerInit(&timer);
    
    // Message pump return value for GetMessage is 0 if WM_QUIT is received
    MSG msg;
    
    while(1)
    {
        while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            if(msg.message == WM_QUIT)
                return(msg.wParam);
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        float c = sin(TimerPeek(&timer)) / 2.0f + 0.5f;
        ClearBuffer(c, c, 1.0f);
        EndFrame();
    }
}