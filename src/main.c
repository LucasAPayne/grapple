#include "math.h"
#include "texture.c"
#include "types.h"
#include "window.c"

#include "shaders/compiled/d3d11_pshader.h"
#include "shaders/compiled/d3d11_vshader.h"

#include <windows.h>
#include <crtdbg.h>
#include <d3d11.h>

#define com_release(obj) do                      \
{                                                \
    if(obj){obj->lpVtbl->Release(obj); obj = 0;} \
} while(0)

#ifdef GRAPPLE_DEBUG
	#define HR(x) do                                                                                             \
	{                                                                                                            \
		HRESULT hr_hr = (x);                                                                                     \
		if(FAILED(hr_hr))                                                                                        \
		{                                                                                                        \
            char hr_msg[512] = "Unknown error";                                                                  \
            char hr_buf[1024];                                                                                   \
            FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, hr_hr, 0, hr_msg,   \
                           sizeof(hr_msg), NULL);                                                                \
            StringCchPrintfA(hr_buf, sizeof(hr_buf), "D3D call failed at %s:%d, HRESULT: 0x%08X\n\nMessage: %s", \
                             __FILE__, __LINE__, hr_hr, hr_msg);                                                 \
			MessageBoxA(NULL, hr_buf, "Direct3D Error", MB_OK | MB_ICONERROR);                                   \
            ExitProcess(1);                                                                                      \
		}                                                                                                        \
	} while(0)
#else
	#define HR(x) (x)
#endif

typedef struct
{
    v2 pos; // Screen-space position
    v2 tex_coord;
} Vertex;

typedef struct
{
    m4 proj;
} CBProj;

global IDXGISwapChain* swap_chain;
global ID3D11Device* device;
global ID3D11DeviceContext* immediate_context;
global ID3D11RenderTargetView* render_target_view;
global ID3D11PixelShader* pixel_shader;
global ID3D11VertexShader* vertex_shader;
global ID3D11InputLayout* input_layout;
global ID3D11Buffer* vertex_buffer;
global ID3D11Buffer* index_buffer;
global ID3D11SamplerState* sampler_state;
global ID3D11ShaderResourceView* texture_view;
global ID3D11BlendState* blend_state;

internal inline v4 color_red(void)    {return (v4){1.0f, 0.0f, 0.0f, 1.0f};}
internal inline v4 color_green(void)  {return (v4){0.0f, 1.0f, 0.0f, 1.0f};}
internal inline v4 color_blue(void)   {return (v4){0.0f, 0.0f, 1.0f, 1.0f};}
internal inline v4 color_black(void)  {return (v4){0.0f, 0.0f, 0.0f, 1.0f};}
internal inline v4 color_white(void)  {return (v4){1.0f, 1.0f, 1.0f, 1.0f};}

internal void init_d3d11(Window* window)
{
    D3D_FEATURE_LEVEL feature_level;

    UINT flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
#ifdef GRAPPLE_DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    HR(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, NULL, 0, D3D11_SDK_VERSION,
                         &device, &feature_level, &immediate_context));
    if (feature_level != D3D_FEATURE_LEVEL_11_0)
    {
        MessageBoxA(0, TEXT("Only Direct3D Feature Level 11.0 is currently supported."),
            "Direct3D Feature Level Unsupported", MB_ICONERROR);
        ExitProcess(1);
    }

    // TODO(lucas): V-Sync
    // TODO(lucas): SRGB gamma correction
    DXGI_SWAP_CHAIN_DESC sd = {0};
    sd.BufferDesc.Width = window->width;
    sd.BufferDesc.Height = window->height;
    sd.BufferDesc.RefreshRate.Numerator = 0;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

    // TODO(lucas): MSAA
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;

    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2;
    sd.OutputWindow = (HWND)window->ptr;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    // Get the IDXGI_Factory used to create the device before creating swap chain
    IDXGIDevice* dxgi_device = 0;
    IDXGIAdapter* dxgi_adapter = 0;
    IDXGIFactory* dxgi_factory = 0;
    HR(device->lpVtbl->QueryInterface(device, &IID_IDXGIDevice, (void**)(&dxgi_device)));
    HR(dxgi_device->lpVtbl->GetParent(dxgi_device, &IID_IDXGIAdapter, (void**)(&dxgi_adapter)));
    HR(dxgi_adapter->lpVtbl->GetParent(dxgi_adapter, &IID_IDXGIFactory, (void**)(&dxgi_factory)));
    HR(dxgi_factory->lpVtbl->CreateSwapChain(dxgi_factory, (IUnknown*)device, &sd, &swap_chain));
    com_release(dxgi_device);
    com_release(dxgi_adapter);
    com_release(dxgi_factory);

    ID3D11Texture2D* back_buffer = 0;
    HR(swap_chain->lpVtbl->GetBuffer(swap_chain, 0, &IID_ID3D11Texture2D, (void**)(&back_buffer)));
    HR(device->lpVtbl->CreateRenderTargetView(device, (ID3D11Resource*)back_buffer, NULL, &render_target_view));
    com_release(back_buffer);
    immediate_context->lpVtbl->OMSetRenderTargets(immediate_context, 1, &render_target_view, NULL);

    D3D11_VIEWPORT vp = {0};
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.Width = (f32)window->width;
    vp.Height = (f32)window->height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    immediate_context->lpVtbl->RSSetViewports(immediate_context, 1, &vp);

    HR(device->lpVtbl->CreateVertexShader(device, d3d11_vshader, sizeof(d3d11_vshader), NULL, &vertex_shader));
    HR(device->lpVtbl->CreatePixelShader(device, d3d11_pshader, sizeof(d3d11_pshader), NULL, &pixel_shader));

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, pos),       D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, tex_coord), D3D11_INPUT_PER_VERTEX_DATA, 0}
    };
    HR(device->lpVtbl->CreateInputLayout(device, layout, countof(layout), d3d11_vshader, sizeof(d3d11_vshader),
       &input_layout));

    D3D11_BUFFER_DESC vb_desc = {0};
    vb_desc.ByteWidth = 4*sizeof(Vertex);
    vb_desc.Usage = D3D11_USAGE_DYNAMIC;
    vb_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    HR(device->lpVtbl->CreateBuffer(device, &vb_desc, NULL, &vertex_buffer));

    u16 indices[] =
    {
        0, 1, 2,
        2, 1, 3
    };
    D3D11_BUFFER_DESC ib_desc = {0};
    ib_desc.Usage = D3D11_USAGE_IMMUTABLE;
    ib_desc.ByteWidth = sizeof(indices);
    ib_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA iinit_data = {0};
    iinit_data.pSysMem = indices;
    HR(device->lpVtbl->CreateBuffer(device, &ib_desc, &iinit_data, &index_buffer));
    immediate_context->lpVtbl->IASetIndexBuffer(immediate_context, index_buffer, DXGI_FORMAT_R16_UINT, 0);

    D3D11_SAMPLER_DESC sampler_desc = {0};
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
    HR(device->lpVtbl->CreateSamplerState(device, &sampler_desc, &sampler_state));

    D3D11_BLEND_DESC transparent_desc = {0};
    transparent_desc.AlphaToCoverageEnable = FALSE;
    transparent_desc.IndependentBlendEnable = FALSE;
    transparent_desc.RenderTarget[0].BlendEnable = TRUE;
    transparent_desc.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
    transparent_desc.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
    transparent_desc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
    transparent_desc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_SRC_ALPHA;
    transparent_desc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_INV_SRC_ALPHA;
    transparent_desc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
    transparent_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    HR(device->lpVtbl->CreateBlendState(device, &transparent_desc, &blend_state));
}

void release_d3d11()
{
    com_release(swap_chain);
    com_release(device);
    com_release(immediate_context);
    com_release(render_target_view);
    com_release(pixel_shader);
    com_release(vertex_shader);
    com_release(input_layout);
    com_release(vertex_buffer);
    com_release(index_buffer);
    com_release(sampler_state);
    com_release(texture_view);
    com_release(blend_state);
}

void draw_texture(v2 pos, v2 dim)
{
    D3D11_MAPPED_SUBRESOURCE mapped;
    immediate_context->lpVtbl->Map(immediate_context, (ID3D11Resource*)vertex_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    Vertex* verts = (Vertex*)mapped.pData;

    f32 x = pos.x;
    f32 y = pos.y;
    f32 w = dim.x;
    f32 h = dim.y;
    verts[0] = (Vertex){ pos,              v2(0.0f, 1.0f) };
    verts[1] = (Vertex){ v2(x + w, y),     v2(1.0f, 1.0f) };
    verts[2] = (Vertex){ v2(x,     y + h), v2(0.0f, 0.0f) };
    verts[3] = (Vertex){ v2(x + w, y + h), v2(1.0f, 0.0f) };

    immediate_context->lpVtbl->Unmap(immediate_context, (ID3D11Resource*)vertex_buffer, 0);
}

int main(void)
{
#ifdef GRAPPLE_DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    int window_width = 800;
    int window_height = 600;
    Window* window = window_create("Grapple", window_width, window_height);
    init_d3d11(window);
    ASSERT(immediate_context, "D3D immediate context is null");
    ASSERT(swap_chain, "D3D swap chain is null");

    m4 proj = ortho_top_left((f32)window_width, (f32)window_height);
    ID3D11Buffer* proj_buffer = {0};
    D3D11_BUFFER_DESC proj_desc = {0};
    proj_desc.ByteWidth = sizeof(proj);
    proj_desc.Usage = D3D11_USAGE_DYNAMIC;
    proj_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    proj_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    HR(device->lpVtbl->CreateBuffer(device, &proj_desc, NULL, &proj_buffer));

    Arena arena = arena_alloc(MEGABYTES(10));
    Texture texture = load_bmp_from_file("res/icons/magnifying_glass.bmp", &arena);

    D3D11_TEXTURE2D_DESC tex_desc = {0};
    tex_desc.Width = texture.width;
    tex_desc.Height = texture.height;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.SampleDesc.Quality = 0;
    tex_desc.Usage = D3D11_USAGE_DEFAULT;
    tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA tex_init_data = {0};
    tex_init_data.pSysMem = texture.data;
    tex_init_data.SysMemPitch = texture.width*texture.channels;
    tex_init_data.SysMemSlicePitch = texture.width*texture.height*texture.channels;

    ID3D11Texture2D* d3d_tex = 0;
    HR(device->lpVtbl->CreateTexture2D(device, &tex_desc, &tex_init_data, &d3d_tex));

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {0};
    srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MipLevels = 1;

    HR(device->lpVtbl->CreateShaderResourceView(device, (ID3D11Resource*)d3d_tex, &srv_desc, &texture_view));

    MSG msg = {0};
    while (window->open)
    {
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                window->open = false;
                break;
            }

            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        if (!window->open)
            break;

        D3D11_MAPPED_SUBRESOURCE mapped = {0};
        HR(immediate_context->lpVtbl->Map(immediate_context, (ID3D11Resource*)proj_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
        CBProj* cb = (CBProj*)mapped.pData;
        cb->proj = m4_transpose(ortho_top_left((f32)window_width, (f32)window_height));
        immediate_context->lpVtbl->Unmap(immediate_context, (ID3D11Resource*)proj_buffer, 0);
        immediate_context->lpVtbl->VSSetConstantBuffers(immediate_context, 0, 1, &proj_buffer);

        v4 clear_color = v4(0.2f, 0.2f, 0.2f, 1.0f);
        immediate_context->lpVtbl->OMSetRenderTargets(immediate_context, 1, &render_target_view, NULL);
        immediate_context->lpVtbl->ClearRenderTargetView(immediate_context, render_target_view, clear_color.e);

        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        immediate_context->lpVtbl->IASetInputLayout(immediate_context, input_layout);
        immediate_context->lpVtbl->IASetVertexBuffers(immediate_context, 0, 1, &vertex_buffer, &stride, &offset);
        immediate_context->lpVtbl->IASetPrimitiveTopology(immediate_context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        immediate_context->lpVtbl->PSSetSamplers(immediate_context, 0, 1, &sampler_state);
        immediate_context->lpVtbl->PSSetShaderResources(immediate_context, 0, 1, &texture_view);
        immediate_context->lpVtbl->VSSetShader(immediate_context, vertex_shader, NULL, 0);
        immediate_context->lpVtbl->PSSetShader(immediate_context, pixel_shader, NULL, 0);
        immediate_context->lpVtbl->OMSetBlendState(immediate_context, blend_state, NULL, 0xffffffff);

        draw_texture(v2(50.0f, 50.0f), v2(32.0f, 32.0f));

        immediate_context->lpVtbl->DrawIndexed(immediate_context, 6, 0, 0);

        HR(swap_chain->lpVtbl->Present(swap_chain, 1, 0));
    }

    com_release(proj_buffer);
    release_d3d11();
    return 0;
}
