#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgidebug.h>
#include <math.h>
#include "dxerr.cpp"
#include <d3dcompiler.h>

// NOTE(trist007): This include uses namespaces of DirectX
// I don't want to do using namespace DirectX so I will use
// my own namespace
#include <DirectXMath.h>
namespace dx = DirectX;

// Ignore Warnings
#pragma warning(disable:4700)

// MISC
struct Timer
{
    LARGE_INTEGER start;
    LARGE_INTEGER frequency;
};

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

// DirectX11
#pragma comment(lib, "d3d11.lib") // Tells Compiler to link to this library
#pragma comment(lib, "dxguid.lib") // Tells Compiler to link to this library
#pragma comment(lib, "D3DCompiler.lib") // for loading shaders

struct HrException
{
    int line;
    const char *file;
    HRESULT hr;
    char info[4096];
};

struct DxgiInfoManager
{
    unsigned long long next;
    IDXGIInfoQueue *pDxgiInfoQueue;
};

// Main DirectX 11 Globals
ID3D11Device *pDevice = 0;
IDXGISwapChain *pSwap = 0;
ID3D11DeviceContext *pContext = 0;
ID3D11RenderTargetView *pTarget = 0;

// Resources
ID3D11Buffer *pVertexBuffer = 0;
ID3D11Buffer *pIndexBuffer = 0;
ID3D11Buffer *pConstantBuffer = 0;
ID3D11VertexShader *pVertexShader = 0;
ID3D11PixelShader *pPixelShader = 0;
ID3D11InputLayout *pInputLayout = 0;

#ifdef _DEBUG
DxgiInfoManager gInfoManager;
#endif

void
HRException(HrException *error, int line, const char *file,
            HRESULT hr, const char **infoMsgs, int msgCount)
{
    error->line = line;
    error->file = file;
    error->hr = hr;
    error->info[0] = '\0';
    
    // Join messages
    for(int i = 0;
        i < msgCount;
        i++)
    {
        strncat(error->info, infoMsgs[i], sizeof(error->info) - strlen(error->info) - 1);
        strncat(error->info, "\n", sizeof(error->info) - strlen(error->info) - 1);
    }
    
    // Remove newline
    size_t len = strlen(error->info);
    if(len > 0)
    {
        error->info[len - 1] = '\0';
    }
}


void
DxgiInfoManagerSet(DxgiInfoManager *m)
{
    // DXGI_DEBUG_ALL   - gives all Direct3D and DXGI objects and private apps
    // DXGI_DEBUG_DX    - gives all Direct3D and DXGI objects
    // DXGI_DEBUG_DXGI  - gives DXGI
    // DXGI_DEBUG_APP   - gives private apps
    // DXGI_DEBUG_D3D11 - gives Direct3D
    
    m->next = m->pDxgiInfoQueue->GetNumStoredMessages(DXGI_DEBUG_ALL);
}

// returns number of messages written into outMessages
// caller must free each string with HeapFree
#ifdef _DEBUG

#define GFX_THROW_INFO_ONLY(call)                                                  \
DxgiInfoManagerSet(&gInfoManager);                                             \
(call);                                                                        \
{                                                                              \
char *msgs[64];                                                            \
int msgCount = DxgiInfoManagerGetMessages(&gInfoManager, msgs, 64);        \
if(msgCount > 0)                                                           \
{                                                                          \
char fullMsg[4096] = {};                                               \
for(int _i = 0; _i < msgCount; _i++)                                   \
{                                                                      \
strncat(fullMsg, msgs[_i], sizeof(fullMsg) - strlen(fullMsg) - 1); \
strncat(fullMsg, "\n", sizeof(fullMsg) - strlen(fullMsg) - 1);     \
}                                                                      \
DxgiInfoManagerFreeMessages(msgs, msgCount);                           \
DXTraceA(__FILE__, __LINE__, S_OK, fullMsg, true);                     \
__debugbreak();                                                        \
}                                                                          \
}


#define GFX_THROW_FAILED(hrcall)                                            \
DxgiInfoManagerSet(&gInfoManager);                                      \
if(FAILED(hr = (hrcall)))                                               \
{                                                                       \
char *msgs[64];                                                     \
int msgCount = 0;                                                   \
char fullMsg[4096] = {};                                            \
_snprintf_s(fullMsg, sizeof(fullMsg), _TRUNCATE, "%s", #hrcall);   \
strncat(fullMsg, "\n", sizeof(fullMsg) - strlen(fullMsg) - 1);     \
msgCount = DxgiInfoManagerGetMessages(&gInfoManager, msgs, 64);    \
for(int _i = 0; _i < msgCount; _i++)                               \
{                                                                   \
strncat(fullMsg, msgs[_i], sizeof(fullMsg) - strlen(fullMsg) - 1); \
strncat(fullMsg, "\n", sizeof(fullMsg) - strlen(fullMsg) - 1); \
}                                                                   \
DxgiInfoManagerFreeMessages(msgs, msgCount);                       \
DXTraceA(__FILE__, __LINE__, hr, fullMsg, true);                   \
__debugbreak();                                                     \
}

#else

#define GFX_THROW_INFO_ONLY(call) (call)
#define GFX_THROW_FAILED(hrcall)                             \
if(FAILED(hr = (hrcall)))                                \
{                                                        \
DXTraceA(__FILE__,__LINE__, hr, #hrcall, true);      \
__debugbreak();                                      \
}                                                        

#endif

#define GFX_DEVICE_REMOVED_EXCEPT(hr) DeviceRemovedException(hr)


void
DxgiInfoManagerFreeMessages(char **messages, int count)
{
    for(int i = 0; i < count; i++)
        HeapFree(GetProcessHeap(), 0, messages[i]);
}

int
DxgiInfoManagerGetMessages(DxgiInfoManager *m, char **outMessages, int maxMessages)
{
    int count = 0;
    UINT64 end = m->pDxgiInfoQueue->GetNumStoredMessages(DXGI_DEBUG_ALL);
    
    for(UINT64 i = m->next; i < end && count < maxMessages; i++)
    {
        HRESULT hr;
        SIZE_T messageLength = 0;
        if(FAILED(hr = m->pDxgiInfoQueue->GetMessage(DXGI_DEBUG_ALL, i, 0, &messageLength)))
        {
            DXTraceA(__FILE__, __LINE__, hr, "GetMessage failed", true);
            __debugbreak();
            
            
        }
        
        DXGI_INFO_QUEUE_MESSAGE *pMessage = (DXGI_INFO_QUEUE_MESSAGE*)
            HeapAlloc(GetProcessHeap(), 0, messageLength);
        
        if(FAILED(hr = m->pDxgiInfoQueue->GetMessage(DXGI_DEBUG_ALL, i, pMessage, &messageLength)))
        {
            
            DXTraceA(__FILE__, __LINE__, hr, "GetMessage failed", true);
            __debugbreak();
        }
        
        // copy description into a separate allocation
        SIZE_T descLen = strlen(pMessage->pDescription) + 1;
        outMessages[count] = (char*)HeapAlloc(GetProcessHeap(), 0, descLen);
        memcpy(outMessages[count], pMessage->pDescription, descLen);
        count++;
        
        HeapFree(GetProcessHeap(), 0, pMessage);
    }
    return count;
}

void
DeviceRemovedException(HRESULT hr)
{
    DXTraceA(__FILE__, __LINE__, hr, "Device Removed (DXGI_ERROR_DEVICE_REMOVED)", true);
    __debugbreak();
}


void
DxgiInfoManagerInit(DxgiInfoManager *m)
{
    m->next = 0u;
    m->pDxgiInfoQueue = 0;
    
    typedef HRESULT (WINAPI* DXGIGetDebugInterface)(REFIID, void**);
    
    HMODULE hModDxgiDebug = LoadLibraryExA("dxgidebug.dll", 0, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if(hModDxgiDebug == 0)
    {
        OutputDebugStringA("Failed to load dxgidebug.dll\n");
        __debugbreak();
    }
    
    DXGIGetDebugInterface DxgiGetDebugInterface = (DXGIGetDebugInterface)(void*)
        GetProcAddress(hModDxgiDebug, "DXGIGetDebugInterface");
    if(DxgiGetDebugInterface == 0)
    {
        OutputDebugStringA("Failed to get DXGIGetDebugInterface\n");
        __debugbreak();
    }
    
    HRESULT hr;
    if(FAILED(hr = DxgiGetDebugInterface(__uuidof(IDXGIInfoQueue), (void**)&m->pDxgiInfoQueue)))
    {
        DXTraceA(__FILE__, __LINE__, hr, "GetMessage failed", true);
        __debugbreak();
    }
}

void
InitD3D(HWND hwnd)
{
    // For error checking of D3D functions
    HRESULT hr;
    
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
    
    UINT swapCreateFlags = 0u;
    
#ifdef _DEBUG
    swapCreateFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    GFX_THROW_FAILED(D3D11CreateDeviceAndSwapChain(
                                                   0,
                                                   D3D_DRIVER_TYPE_HARDWARE,
                                                   0,
                                                   swapCreateFlags,
                                                   0,
                                                   0,
                                                   D3D11_SDK_VERSION,
                                                   &sd,
                                                   &pSwap,
                                                   &pDevice,
                                                   0,
                                                   &pContext
                                                   ));
    
    // Gain access to texture subresource in swap chain (back buffer)
    ID3D11Resource *pBackBuffer = 0;
    GFX_THROW_FAILED(pSwap->GetBuffer(0, __uuidof(ID3D11Resource), (void**)&pBackBuffer));
    GFX_THROW_FAILED(pDevice->CreateRenderTargetView(pBackBuffer, 0, &pTarget));
    
    // No longer required only needed to create the RenderTargetView
    pBackBuffer->Release();
    
}


void
EndFrame()
{
    HRESULT hr;
    if(FAILED(hr = pSwap->Present(1u, 0u)))
    {
        if(hr == DXGI_ERROR_DEVICE_REMOVED)
            GFX_DEVICE_REMOVED_EXCEPT(pDevice->GetDeviceRemovedReason());
        else
            GFX_THROW_FAILED(hr);
    }
}

void
ClearBuffer(float red, float green, float blue)
{
    float color[] = { red, green, blue, 1.0f };
    pContext->ClearRenderTargetView(pTarget, color);
}

void
DrawTestTriangle(float angle)
{
    HRESULT hr;
    
    /*
    struct Vertex
    {
        float x;
        float y;
        unsigned char r;
        unsigned char g;
        unsigned char b;
        unsigned char a;
    };
    */
    
    struct Vertex
    {
        struct
        {
            float x;
            float y;
        } pos;
        
        struct
        {
            unsigned char r;
            unsigned char g;
            unsigned char b;
            unsigned char a;
        } color;
    };
    
    Vertex vertices[] = 
    {
        { 0.0f,  0.5f, 255, 0, 0, 0 },
        { 0.5f, -0.5f, 0, 255, 0, 0 },
        {-0.5f, -0.5f, 0, 0, 255, 0 },
        {-0.3f,  0.3f, 0, 255, 0, 0 },
        { 0.3f,  0.3f, 0, 0, 255, 0 },
        { 0.0f, -1.0f, 255, 0, 0, 0 },
    };
    
    UINT vertexCount = sizeof(vertices) / sizeof(vertices[0]);
    
    vertices[0].color.g = 255;
    
    D3D11_BUFFER_DESC bd = {};
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.CPUAccessFlags = 0u;
    bd.MiscFlags = 0u;
    bd.ByteWidth = sizeof(vertices);
    bd.StructureByteStride = sizeof(Vertex);
    
    D3D11_SUBRESOURCE_DATA sd = {};
    sd.pSysMem = vertices;
    
    GFX_THROW_FAILED(hr = (pDevice->CreateBuffer(&bd, &sd, &pVertexBuffer)));
    
    
    const UINT stride = sizeof(Vertex);
    const UINT offset = 0u;
    pContext->IASetVertexBuffers(0u, 1u, &pVertexBuffer, &stride, &offset);
    
    // Create vertex shader
    ID3DBlob *pBlob = 0;
    GFX_THROW_FAILED(D3DReadFileToBlob(L"../directx/code/shaders/vertex.cso", &pBlob));
    
    GFX_THROW_FAILED(pDevice->CreateVertexShader(
                                                 pBlob->GetBufferPointer(),
                                                 pBlob->GetBufferSize(),
                                                 0,
                                                 &pVertexShader));
    // Bind vertex shader
    pContext->VSSetShader(pVertexShader, 0, 0u);
    
    // Create input layout
    D3D11_INPUT_ELEMENT_DESC ied[] = {
        {"Position", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        
        // NOTE(trist007): 8u offset for Color element cause we are 8 bytes into ied
        // since Position has 2 floats which is 8 bytes
        // you can also just use D3D11_APPEND_ALIGNED_ELEMENT which auto calculates offset
        
        // NOTE(trist007): changing from UINT to UNORM, UINT goes 0-255 UNORM will normalize to float
        // 0 will be 0.0f 128 will be 0.5f and 255 will be 1.0f
        {"Color", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    
    UINT numElements = sizeof(ied) / sizeof(ied[0]);
    
    GFX_THROW_FAILED(pDevice->CreateInputLayout(
                                                ied, numElements,
                                                pBlob->GetBufferPointer(),
                                                pBlob->GetBufferSize(),
                                                &pInputLayout));
    pContext->IASetInputLayout(pInputLayout);
    
    pBlob->Release();
    pBlob = 0;
    
    // Create index buffer
    const unsigned short indices[] =
    {
        0, 1, 2,
        0, 2, 3,
        0, 4, 1,
        2, 1, 5,
    };
    
    D3D11_BUFFER_DESC ibd = {};
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.CPUAccessFlags = 0u;
    ibd.MiscFlags = 0u;
    ibd.ByteWidth = sizeof(indices);
    ibd.StructureByteStride = sizeof(unsigned short);
    D3D11_SUBRESOURCE_DATA isd = {};
    isd.pSysMem = indices;
    
    GFX_THROW_FAILED(hr = (pDevice->CreateBuffer(&ibd, &isd, &pIndexBuffer)));
    
    
    // Bind index buffer
    pContext->IASetIndexBuffer(pIndexBuffer, DXGI_FORMAT_R16_UINT, 0u);
    
    // Create constant buffer
    struct ConstantBuffer
    {
        // NOTE(trist007):  XMMATRIX is a 4x4 float matrix
        dx::XMMATRIX transform;
    };
    
    const ConstantBuffer cb =
    {
        {
            // NOTE(trist007): there is Operator Overloading, you can use "*" instead
            // of 
            // dx::XMMatrixMultiply(
            // dx::XMMatrixRotationZ(angle),               
            // dx::XMMatrixScaling(3.0f/4.0f, 1.0f, 1.0f)  
            // )
            // NOTE(trist007): since I removed row_major in the VertexShader.hlsl I will have to
            // Transpose
            dx::XMMatrixTranspose(
                                  dx::XMMatrixRotationZ(angle) *                   // rotation
                                  dx::XMMatrixScaling(3.0f/4.0f, 1.0f, 1.0f)   // scaling
                                  )
        }
    };
    
    D3D11_BUFFER_DESC cbd;
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbd.MiscFlags = 0u;
    cbd.ByteWidth = sizeof(cb);
    cbd.StructureByteStride = 0u;
    D3D11_SUBRESOURCE_DATA csd = {};
    csd.pSysMem = &cb;
    GFX_THROW_FAILED(pDevice->CreateBuffer(&cbd, &csd, &pConstantBuffer));
    
    // Bind constant buffer to vertex shader
    pContext->VSSetConstantBuffers(0u, 1u, &pConstantBuffer);
    
    // Create pixel shader
    GFX_THROW_FAILED(D3DReadFileToBlob(L"../directx/code/shaders/pixel.cso", &pBlob));
    GFX_THROW_FAILED(pDevice->CreatePixelShader(
                                                pBlob->GetBufferPointer(),
                                                pBlob->GetBufferSize(),
                                                0,
                                                &pPixelShader));
    // Bind pixel shader
    pContext->PSSetShader(pPixelShader, 0, 0u);
    
    // Bind render target
    pContext->OMSetRenderTargets(1u, &pTarget, 0);
    
    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    // Configure viewport
    D3D11_VIEWPORT vp;
    vp.Width = 640;
    vp.Height = 480;
    vp.MinDepth = 0;
    vp.MaxDepth = 1;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    pContext->RSSetViewports(1u, &vp);
    
    // NOTE(trist007): need to use DrawIndexed instead of Draw if using indexes
    //pContext->Draw(vertexCount, 0u);
    pContext->DrawIndexed(ArrayCount(indices), 0u, 0u);
    
    pBlob->Release();
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
            PostQuitMessage(17);
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
#ifdef _DEBUG
    DxgiInfoManagerInit(&gInfoManager);
#endif
    
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
        
        float t = TimerPeek(&timer);
        float c = sin(t) / 2.0f + 0.5f;
        ClearBuffer(c, c, 1.0f);
        DrawTestTriangle(t);
        EndFrame();
    }
    
    // Not worth releasing as OS will Release automatically upon process termination
    // Free D3D11 resources
    /*
        pVertexBuffer->Release();
        pVertexShader->Release();
        pInputLayout->Release();
        pPixelShader->Release();
        
        pTarget->Release();
        pContext->Release();
        pSwap->Release();
        pDevice->Release();
        */
}