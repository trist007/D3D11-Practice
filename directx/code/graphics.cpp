#include <d3d11.h>
#include <dxgidebug.h>
#include "dxerr.cpp"
#include <d3dcompiler.h>

#include <DirectXMath.h>

#include "shared.h"

// DirectX11 Libraries
#pragma comment(lib, "d3d11.lib") // Tells Compiler to link to this library
#pragma comment(lib, "dxguid.lib") // Tells Compiler to link to this library
#pragma comment(lib, "D3DCompiler.lib") // for loading shaders

// ======================================================================================
// ERROR / DEBUG
// ======================================================================================

struct HrException
{
    int line;
    char *file;
    HRESULT hr;
    char info[4096];
};

struct DxgiInfoManager
{
    unsigned long long next;
    IDXGIInfoQueue *pDxgiInfoQueue;
};

#ifdef _DEBUG
DxgiInfoManager gInfoManager;
#endif

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
DxgiInfoManagerSet(DxgiInfoManager *m)
{
    // DXGI_DEBUG_ALL   - gives all Direct3D and DXGI objects and private apps
    // DXGI_DEBUG_DX    - gives all Direct3D and DXGI objects
    // DXGI_DEBUG_DXGI  - gives DXGI
    // DXGI_DEBUG_APP   - gives private apps
    // DXGI_DEBUG_D3D11 - gives Direct3D
    
    m->next = m->pDxgiInfoQueue->GetNumStoredMessages(DXGI_DEBUG_ALL);
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
DeviceRemovedException(HRESULT hr)
{
    DXTraceA(__FILE__, __LINE__, hr, "Device Removed (DXGI_ERROR_DEVICE_REMOVED)", true);
    __debugbreak();
}

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

// ======================================================================================
// RENDERER CORE
// ======================================================================================

struct Renderer
{
    ID3D11Device              *device;
    IDXGISwapChain              *swap;
    ID3D11DeviceContext      *context;
    ID3D11RenderTargetView    *target;
    ID3D11DepthStencilView       *dsv;
    ID3D11DepthStencilState *ds_state;
    ID3D11Texture2D    *depth_stencil;
};

void
RendererInitSwapChain(Renderer *r, HWND hwnd)
{
    // For error checking of D3D functions
    HRESULT hr;
    
    DXGI_SWAP_CHAIN_DESC sd               = {};
    sd.BufferDesc.Width                   = 0;
    sd.BufferDesc.Height                  = 0;
    sd.BufferDesc.Format                  = DXGI_FORMAT_B8G8R8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator   = 0;
    sd.BufferDesc.RefreshRate.Denominator = 0;
    sd.BufferDesc.Scaling                 = DXGI_MODE_SCALING_UNSPECIFIED;
    sd.BufferDesc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.SampleDesc.Count                   = 1; // Anti-aliasing
    sd.SampleDesc.Quality                 = 0;
    sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount                        = 1; // Double buffering
    sd.OutputWindow                       = hwnd;
    sd.Windowed                           = TRUE;
    sd.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD; // Vanilla
    sd.Flags                              = 0;
    
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
                                                   &r->swap,
                                                   &r->device,
                                                   0,
                                                   &r->context
                                                   ));
}

void
RendererInitRenderTarget(Renderer *r)
{
    HRESULT hr;
    
    // Gain access to texture subresource in swap chain (back buffer)
    ID3D11Resource *pBackBuffer = 0;
    
    GFX_THROW_FAILED(r->swap->GetBuffer(0, __uuidof(ID3D11Resource), (void**)&pBackBuffer));
    GFX_THROW_FAILED(r->device->CreateRenderTargetView(pBackBuffer, 0, &r->target));
    
    pBackBuffer->Release();
    
}

void
RendererInitDepthStencil(Renderer *r, UINT width, UINT height)
{
    HRESULT hr;
    
    // Create depth stencil state
    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable              = TRUE;
    dsDesc.DepthWriteMask           = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc                = D3D11_COMPARISON_LESS;
    
    GFX_THROW_FAILED(r->device->CreateDepthStencilState(&dsDesc, &r->ds_state));
    
    // Bind depth state
    r->context->OMSetDepthStencilState(r->ds_state, 1u);
    
    // Create depth stencil texture
    D3D11_TEXTURE2D_DESC descDepth = {};
    descDepth.Width                = 1024u;
    descDepth.Height               = 768u;
    descDepth.MipLevels            = 1u;
    descDepth.ArraySize            = 1u;
    descDepth.Format               = DXGI_FORMAT_D32_FLOAT; // special format for Depth hence D32
    descDepth.SampleDesc.Count     = 1u;
    descDepth.SampleDesc.Quality   = 0u;
    descDepth.Usage                = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags            = D3D11_BIND_DEPTH_STENCIL;
    
    GFX_THROW_FAILED(r->device->CreateTexture2D(&descDepth, 0, &r->depth_stencil));
    
    // Create view of depth stencil texture
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
    descDSV.Format                        = DXGI_FORMAT_D32_FLOAT;
    descDSV.ViewDimension                 = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice            = 0u;
    
    GFX_THROW_FAILED(r->device->CreateDepthStencilView(r->depth_stencil, &descDSV, &r->dsv));
    
    // Bind depth stencil view to OM pipeline
    r->context->OMSetRenderTargets(1u, &r->target, r->dsv);
}

void
RendererInit(Renderer *r, HWND hwnd, UINT width, UINT height)
{
    RendererInitSwapChain(r, hwnd);
    RendererInitRenderTarget(r);
    RendererInitDepthStencil(r, width, height);
}

void
RendererClear(Renderer *r, float red, float green, float blue)
{
    float color[] = { red, green, blue, 1.0f };
    r->context->ClearRenderTargetView(r->target, color);
    r->context->ClearDepthStencilView(r->dsv, D3D11_CLEAR_DEPTH, 1.0f, 0u);
}

void
RendererPresent(Renderer *r)
{
    HRESULT hr;
    
    if(FAILED(hr = r->swap->Present(1u, 0u)))
    {
        if(hr == DXGI_ERROR_DEVICE_REMOVED)
            GFX_DEVICE_REMOVED_EXCEPT(r->device->GetDeviceRemovedReason());
        else
            GFX_THROW_FAILED(hr);
    }
}

// ======================================================================================
// SURFACE
// ======================================================================================

struct Surface
{
    unsigned char *pixels;
    int            width;
    int            height;
    bool           has_alpha;
};

bool
SurfaceLoad(Surface *s, wchar_t *path)
{
    char narrow[256];
    WideCharToMultiByte(CP_UTF8, 0, path, -1, narrow, sizeof(narrow), 0, 0);
    
    int channels = 0;
    
    // force 4 channels (RGBA) on load
    s->pixels = stbi_load(narrow, &s->width, &s->height, &channels, 4);
    
    if(!s->pixels)
    {
        OutputDebugStringA("SurfaceLoad failed: ");
        OutputDebugStringA(stbi_failure_reason());
        OutputDebugStringA("\n");
        
        return(false);
    }
    
    s->has_alpha = (channels == 4);
    return(true);
}

void
SurfaceFree(Surface *s)
{
    if(s->pixels) { stbi_image_free(s->pixels); s->pixels = 0; }
}


UINT SurfaceWidth(Surface *s)  { return (UINT)s->width;  }
UINT SurfaceHeight(Surface *s) { return (UINT)s->height; }
UINT SurfaceRowPitch(Surface *s) { return (UINT)s->width * 4u; } // stb_image never pads rows

unsigned char *
SurfacePixels(Surface *s)
{
    return(s->pixels);
}

// ======================================================================================
// TEXTURE
// ======================================================================================

struct Texture
{
    ID3D11ShaderResourceView *srv;
    UINT                      slot;
    bool                      has_alpha;
};

void
TextureInit(Renderer *r, Texture *t, wchar_t *path, UINT slot)
{
    HRESULT hr;
    
    t->slot      = slot;
    t->has_alpha = false;
    
    Surface s = {};
    char err[256];
    if(!SurfaceLoad(&s, path))
    {
        OutputDebugStringA(err);
        __debugbreak();
    }
    
    t->has_alpha = s.has_alpha;
    
    D3D11_TEXTURE2D_DESC td  = {};
    td.Width                 = SurfaceWidth(&s);
    td.Height                = SurfaceHeight(&s);
    td.MipLevels             = 1; // no mipmapping
    td.ArraySize             = 1;
    td.Format                = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count      = 1;
    td.SampleDesc.Quality    = 0;
    td.Usage                 = D3D11_USAGE_DEFAULT;
    td.BindFlags             = D3D11_BIND_SHADER_RESOURCE;
    td.CPUAccessFlags        = 0;
    td.MiscFlags             = 0;
    
    D3D11_SUBRESOURCE_DATA sd = {};
    sd.pSysMem     = SurfacePixels(&s);
    sd.SysMemPitch = SurfaceRowPitch(&s);
    
    ID3D11Texture2D *pTex = 0;
    GFX_THROW_FAILED(r->device->CreateTexture2D(&td, &sd, &pTex));
    
    // Upload top mip — must use actual row pitch from the scratch image, 
    // not width*4, because DirectXTex may pad rows
    
    /*
r->context->UpdateSubresource(
                                  pTex, 0u, 0,
                                  SurfacePixels(&s),
                                  SurfaceRowPitch(&s),
                                  0u
                                  );
    */
    D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
    srvd.Format                          = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvd.ViewDimension                   = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvd.Texture2D.MostDetailedMip       = 0;
    srvd.Texture2D.MipLevels             = 1;
    GFX_THROW_FAILED(r->device->CreateShaderResourceView(pTex, &srvd, &t->srv));
    
    //r->context->GenerateMips(t->srv);
    
    pTex->Release();
    SurfaceFree(&s);
}

void
TextureBind(Renderer *r, Texture *t)
{
    GFX_THROW_INFO_ONLY(
                        r->context->PSSetShaderResources(t->slot, 1u, &t->srv)
                        );
}

void
TextureRelease(Texture *t)
{
    if(t->srv) { t->srv->Release(); t->srv = 0; }
}

// ======================================================================================
// SAMPLER
// ======================================================================================

typedef enum
{
    SAMPLER_ANISOTROPIC,
    SAMPLER_BILINEAR,
    SAMPLER_POINT,
} SamplerType;

struct Sampler
{
    ID3D11SamplerState *state;
    UINT                slot;
};

void
SamplerInit(Renderer *r, Sampler *s, SamplerType type, bool reflect, UINT slot)
{
    HRESULT hr;
    
    s->slot = slot;
    
    D3D11_SAMPLER_DESC sd       = {};
    sd.Filter                   = D3D11_FILTER_MIN_MAG_MIP_LINEAR; // great for smoothness if use point it gets pixellated when texture is close
    sd.AddressU                 = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.AddressV                 = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.AddressW                 = D3D11_TEXTURE_ADDRESS_WRAP;
    
    GFX_THROW_FAILED(r->device->CreateSamplerState(&sd, &s->state));
}

void
SamplerBind(Renderer *r, Sampler *s)
{
    GFX_THROW_INFO_ONLY(
                        r->context->PSSetSamplers(s->slot, 1u, &s->state)
                        );
}

void
SamplerRelease(Sampler *s)
{
    if(s->state) { s->state->Release(); s->state = 0; }
}

// ======================================================================================
// MESH
// ======================================================================================

struct Vertex
{
    struct
    {
        float x;
        float y;
        float z;
    } pos;
    
    struct
    {
        float u;
        float v;
    } tex;
};

struct Mesh
{
    ID3D11Buffer *vertex_buffer;
    ID3D11Buffer *index_buffer;
    UINT          index_count;
};

void
MeshInit(Renderer *r, Mesh *m, Vertex *vertices, UINT vertex_count,
         unsigned short *indices, UINT index_count)
{
    HRESULT hr;
    
    D3D11_BUFFER_DESC bd       = {};
    bd.BindFlags               = D3D11_BIND_VERTEX_BUFFER;
    bd.Usage                   = D3D11_USAGE_DEFAULT;
    bd.CPUAccessFlags          = 0u;
    bd.MiscFlags               = 0u;
    bd.ByteWidth               = sizeof(Vertex) * vertex_count;
    bd.StructureByteStride     = sizeof(Vertex);
    
    D3D11_SUBRESOURCE_DATA sd  = {};
    sd.pSysMem                 = vertices;
    
    GFX_THROW_FAILED(r->device->CreateBuffer(&bd, &sd, &m->vertex_buffer));
    
    D3D11_BUFFER_DESC ibd      = {};
    ibd.BindFlags              = D3D11_BIND_INDEX_BUFFER;
    ibd.Usage                  = D3D11_USAGE_DEFAULT;
    ibd.CPUAccessFlags         = 0u;
    ibd.MiscFlags              = 0u;
    ibd.ByteWidth              = sizeof(unsigned short) * index_count;
    ibd.StructureByteStride    = sizeof(unsigned short);
    D3D11_SUBRESOURCE_DATA isd = {};
    isd.pSysMem = indices;
    
    GFX_THROW_FAILED(r->device->CreateBuffer(&ibd, &isd, &m->index_buffer));
    
    m->index_count = index_count;
}

void
MeshRelease(Mesh *m)
{
    if(m->vertex_buffer) { m->vertex_buffer->Release(); m->vertex_buffer = 0; }
    if(m->index_buffer)  { m->index_buffer->Release();  m->index_buffer  = 0; }
}

// ======================================================================================
// SHADER PIPELINE
// ======================================================================================

struct ShaderPipeline
{
    ID3D11VertexShader *vs;
    ID3D11PixelShader  *ps;
    ID3D11InputLayout  *input_layout;
    
};

void
ShaderPipelineInit(Renderer *r, ShaderPipeline *sp,
                   wchar_t *vs_path, wchar_t *ps_path)
{
    HRESULT hr;
    
    // Create vertex shader
    ID3DBlob *pBlob = 0;
    GFX_THROW_FAILED(D3DReadFileToBlob(vs_path, &pBlob));
    GFX_THROW_FAILED(r->device->CreateVertexShader(
                                                   pBlob->GetBufferPointer(),
                                                   pBlob->GetBufferSize(),
                                                   0,
                                                   &sp->vs));
    
    // Create input layout
    D3D11_INPUT_ELEMENT_DESC ied[] = {
        {"Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TexCoord", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(float) * 3, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    
    GFX_THROW_FAILED(r->device->CreateInputLayout(
                                                  ied, ArrayCount(ied),
                                                  pBlob->GetBufferPointer(),
                                                  pBlob->GetBufferSize(),
                                                  &sp->input_layout));
    pBlob->Release();
    pBlob = 0;
    
    
    // Create pixel shader
    GFX_THROW_FAILED(D3DReadFileToBlob(ps_path, &pBlob));
    GFX_THROW_FAILED(r->device->CreatePixelShader(
                                                  pBlob->GetBufferPointer(),
                                                  pBlob->GetBufferSize(),
                                                  0,
                                                  &sp->ps));
    
    pBlob->Release();
}

void
ShaderPipelineRelease(ShaderPipeline *sp)
{
    if(sp->vs)           { sp->vs->Release();           sp->vs           = 0; }
    if(sp->ps)           { sp->ps->Release();           sp->ps           = 0; }
    if(sp->input_layout) { sp->input_layout->Release(); sp->input_layout = 0; }
}

// ======================================================================================
// CONSTANT BUFFERS
// ======================================================================================

struct CBTransform
{
    DirectX::XMMATRIX transform;
};

struct CBFaceColors
{
    struct { float r, g, b, a; } face_colors[6];
};

struct ConstantBuffers
{
    ID3D11Buffer *transform;     // bound to VS slot 0
    ID3D11Buffer *face_colors;   // bound to PS slot 0
};

void
ConstantBuffersInit(Renderer *r, ConstantBuffers *cb)
{
    HRESULT hr;
    
    // Transform CB  — DYNAMIC so we can Map/Unmap each frame
    D3D11_BUFFER_DESC cbd   = {};
    cbd.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
    cbd.Usage               = D3D11_USAGE_DYNAMIC;
    cbd.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
    cbd.MiscFlags           = 0u;
    cbd.ByteWidth           = sizeof(CBTransform);
    cbd.StructureByteStride = 0u;
    GFX_THROW_FAILED(r->device->CreateBuffer(&cbd, 0, &cb->transform));
    
    // Face color CB — DEFAULT, uploaded once
    CBFaceColors face_data =
    {
        {
            {1.0f, 0.0f, 1.0f, 1.0f},
            {1.0f, 0.0f, 0.0f, 1.0f},
            {0.0f, 1.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f, 1.0f},
            {1.0f, 1.0f, 0.0f, 1.0f},
            {0.0f, 1.0f, 1.0f, 1.0f},
        }
    };
    
    D3D11_BUFFER_DESC cbd2      = {};
    cbd2.BindFlags              = D3D11_BIND_CONSTANT_BUFFER;
    cbd2.Usage                  = D3D11_USAGE_DEFAULT;
    cbd2.CPUAccessFlags         = 0u;
    cbd2.MiscFlags              = 0u;
    cbd2.ByteWidth              = sizeof(CBFaceColors);
    cbd2.StructureByteStride    = 0u;
    D3D11_SUBRESOURCE_DATA csd2 = {};
    csd2.pSysMem                = &face_data;
    GFX_THROW_FAILED(r->device->CreateBuffer(&cbd2, &csd2, &cb->face_colors));
}

void
ConstantBuffersRelease(ConstantBuffers *cb)
{
    if(cb->transform)   { cb->transform->Release();   cb->transform   = 0; }
    if(cb->face_colors) { cb->face_colors->Release(); cb->face_colors = 0; }
}

void
ConstantBuffersUpdateTransform(Renderer *r, ConstantBuffers *cb, DirectX::XMMATRIX transform)
{
    D3D11_MAPPED_SUBRESOURCE msr;
    r->context->Map(cb->transform, 0u, D3D11_MAP_WRITE_DISCARD, 0u, &msr);
    CBTransform *data   = (CBTransform *)msr.pData;
    data->transform     = transform;
    r->context->Unmap(cb->transform, 0u);
}

void
ConstantBuffersUpdateColor(Renderer *r, ConstantBuffers *cb, float *color)
{
    CBFaceColors data = {};
    for(int i = 0;
        i < 6;
        i++)
    {
        data.face_colors[i].r = color[0];
        data.face_colors[i].g = color[1];
        data.face_colors[i].b = color[2];
        data.face_colors[i].a = color[3];
    }
    
    r->context->UpdateSubresource(cb->face_colors, 0u, 0, &data, 0u, 0);
}

// ============================================================
// DRAW
// ============================================================

// NOTE: projection matrix is computed once per camera setup, not per draw
DirectX::XMMATRIX
MakeProjection(float width, float height, float near_z, float far_z)
{
    float fov = DirectX::XM_PIDIV4;
    float aspect = width / height;
    return(DirectX::XMMatrixPerspectiveFovLH(fov, aspect, near_z, far_z));
}

void
DrawCube(Renderer *r, Mesh *m, ShaderPipeline *sp, ConstantBuffers *cb,
         float angle, float x, float z,
         DirectX::XMMATRIX projection,
         UINT width, UINT height)
{
    // Update transform constant buffer via Map/Unmap (no alloc)
    DirectX::XMMATRIX transform = DirectX::XMMatrixTranspose(
                                                             DirectX::XMMatrixRotationZ(angle) *
                                                             DirectX::XMMatrixRotationX(angle) *
                                                             DirectX::XMMatrixTranslation(x, 0.0f, z + 4.0f) *
                                                             projection
                                                             );
    ConstantBuffersUpdateTransform(r, cb, transform);
    
    // Bind shaders
    r->context->VSSetShader(sp->vs, 0, 0u);
    r->context->PSSetShader(sp->ps, 0, 0u);
    
    // Bind input layout
    r->context->IASetInputLayout(sp->input_layout);
    
    // Bind vertex buffer
    UINT stride = sizeof(Vertex);
    UINT offset = 0u;
    r->context->IASetVertexBuffers(0u, 1u, &m->vertex_buffer, &stride, &offset);
    
    // Bind index buffer
    r->context->IASetIndexBuffer(m->index_buffer, DXGI_FORMAT_R16_UINT, 0u);
    
    // Bind constant buffers
    r->context->VSSetConstantBuffers(0u, 1u, &cb->transform);
    r->context->PSSetConstantBuffers(0u, 1u, &cb->face_colors);
    
    // Topology + viewport
    r->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    D3D11_VIEWPORT vp;
    vp.Width    = (float)width;
    vp.Height   = (float)height;
    vp.MinDepth = 0;
    vp.MaxDepth = 1;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    r->context->RSSetViewports(1u, &vp);
    
    r->context->DrawIndexed(m->index_count, 0u, 0u);
}