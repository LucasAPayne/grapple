#include "grapple_math.h"
#include "renderer/font.h"
#include "renderer/renderer.h"

#include "d3d11_renderer.h"
#include "platform/windows/win32_base.h"

#include "shaders/compiled/d3d11_pshader.h"
#include "shaders/compiled/d3d11_vshader.h"

#include <crtdbg.h>
#include <d3d11.h>
#include <windows.h>

internal Renderer* renderer_create(Window* window, Arena* arena)
{
#ifdef GRAPPLE_DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    Renderer* renderer = push_struct(arena, Renderer);

    UINT flags = D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef GRAPPLE_DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL feature_levels[] = {D3D_FEATURE_LEVEL_11_1};
    D3D_FEATURE_LEVEL feature_level;
    HR(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, feature_levels, countof(feature_levels),
                         D3D11_SDK_VERSION, &renderer->device, &feature_level, &renderer->ctx));
    if (feature_level != D3D_FEATURE_LEVEL_11_1)
    {
        MessageBoxA(0, TEXT("Only Direct3D Feature Level 11.1 is currently supported."),
            "Direct3D Feature Level Unsupported", MB_ICONERROR);
        ExitProcess(1);
    }

    DXGI_SWAP_CHAIN_DESC sd = {0};
    sd.BufferDesc.Width = window->width;
    sd.BufferDesc.Height = window->height;
    sd.BufferDesc.RefreshRate.Numerator = 0;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;

    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2;
    sd.OutputWindow = (HWND)window->ptr;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    // Get the IDXGI_Factory used to create the device before creating swap chain
    IDXGIDevice* dxgi_device = NULL;
    IDXGIAdapter* dxgi_adapter = NULL;
    IDXGIFactory* dxgi_factory = NULL;
    HR(renderer->device->lpVtbl->QueryInterface(renderer->device, &IID_IDXGIDevice, (void**)(&dxgi_device)));
    HR(dxgi_device->lpVtbl->GetParent(dxgi_device, &IID_IDXGIAdapter, (void**)(&dxgi_adapter)));
    HR(dxgi_adapter->lpVtbl->GetParent(dxgi_adapter, &IID_IDXGIFactory, (void**)(&dxgi_factory)));
    HR(dxgi_factory->lpVtbl->CreateSwapChain(dxgi_factory, (IUnknown*)renderer->device, &sd, &renderer->swap_chain));
    com_release(dxgi_device);
    com_release(dxgi_adapter);
    com_release(dxgi_factory);

    ID3D11Texture2D* back_buffer = 0;
    HR(renderer->swap_chain->lpVtbl->GetBuffer(renderer->swap_chain, 0, &IID_ID3D11Texture2D, (void**)(&back_buffer)));
    HR(renderer->device->lpVtbl->CreateRenderTargetView(renderer->device, (ID3D11Resource*)back_buffer, NULL,
       &renderer->render_target_view));
    com_release(back_buffer);
    renderer->ctx->lpVtbl->OMSetRenderTargets(renderer->ctx, 1, &renderer->render_target_view, NULL);

    D3D11_VIEWPORT vp = {0};
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.Width = (f32)window->width;
    vp.Height = (f32)window->height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    renderer->ctx->lpVtbl->RSSetViewports(renderer->ctx, 1, &vp);

    HR(renderer->device->lpVtbl->CreateVertexShader(renderer->device, d3d11_vshader, sizeof(d3d11_vshader), NULL,
                                                    &renderer->vertex_shader));
    HR(renderer->device->lpVtbl->CreatePixelShader(renderer->device, d3d11_pshader, sizeof(d3d11_pshader), NULL,
                                                   &renderer->pixel_shader));

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, pos),        D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, tex_coord), D3D11_INPUT_PER_VERTEX_DATA, 0}
    };
    HR(renderer->device->lpVtbl->CreateInputLayout(renderer->device, layout, countof(layout), d3d11_vshader,
       sizeof(d3d11_vshader), &renderer->input_layout));

    D3D11_BUFFER_DESC vb_desc = {0};
    vb_desc.ByteWidth = 4*sizeof(Vertex);
    vb_desc.Usage = D3D11_USAGE_DYNAMIC;
    vb_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    HR(renderer->device->lpVtbl->CreateBuffer(renderer->device, &vb_desc, NULL, &renderer->vertex_buffer));

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
    HR(renderer->device->lpVtbl->CreateBuffer(renderer->device, &ib_desc, &iinit_data, &renderer->index_buffer));

    D3D11_SAMPLER_DESC sampler_desc = {0};
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
    HR(renderer->device->lpVtbl->CreateSamplerState(renderer->device, &sampler_desc, &renderer->sampler_state));

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
    HR(renderer->device->lpVtbl->CreateBlendState(renderer->device, &transparent_desc, &renderer->blend_state));

    D3D11_BUFFER_DESC proj_desc = {0};
    proj_desc.ByteWidth = sizeof(m4);
    proj_desc.Usage = D3D11_USAGE_DYNAMIC;
    proj_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    proj_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    HR(renderer->device->lpVtbl->CreateBuffer(renderer->device, &proj_desc, NULL, &renderer->proj_buffer));
    renderer->proj = ortho_top_left((f32)window->width, (f32)window->height);
    renderer_set_projection(renderer, renderer->proj);

    renderer->text_renderer = text_renderer_create(window->ptr, renderer->swap_chain, arena);

    ASSERT(renderer->ctx, "D3D immediate context is null");
    ASSERT(renderer->swap_chain, "D3D swap chain is null");

    return renderer;
}

internal void renderer_destroy(Renderer* renderer)
{
    com_release(renderer->swap_chain);
    com_release(renderer->device);
    com_release(renderer->ctx);
    com_release(renderer->render_target_view);
    com_release(renderer->pixel_shader);
    com_release(renderer->vertex_shader);
    com_release(renderer->input_layout);
    com_release(renderer->vertex_buffer);
    com_release(renderer->index_buffer);
    com_release(renderer->sampler_state);
    com_release(renderer->blend_state);
    com_release(renderer->proj_buffer);
    text_renderer_destroy(renderer->text_renderer);
}

internal void renderer_set_projection(Renderer* renderer, m4 proj)
{
    renderer->proj = proj;
    D3D11_MAPPED_SUBRESOURCE mapped = {0};
    HR(renderer->ctx->lpVtbl->Map(renderer->ctx, (ID3D11Resource*)renderer->proj_buffer, 0, D3D11_MAP_WRITE_DISCARD,
                                  0, &mapped));
    CBProj* cb = (CBProj*)mapped.pData;
    cb->proj = m4_transpose(proj);
    renderer->ctx->lpVtbl->Unmap(renderer->ctx, (ID3D11Resource*)renderer->proj_buffer, 0);
    renderer->ctx->lpVtbl->VSSetConstantBuffers(renderer->ctx, 0, 1, &renderer->proj_buffer);
}

internal void renderer_upload_texture(Renderer* renderer, Texture* texture)
{
    D3D11_TEXTURE2D_DESC tex_desc = {0};
    tex_desc.Width = texture->width;
    tex_desc.Height = texture->height;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.SampleDesc.Quality = 0;
    tex_desc.Usage = D3D11_USAGE_DEFAULT;
    tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA tex_init_data = {0};
    tex_init_data.pSysMem = texture->data;
    tex_init_data.SysMemPitch = texture->width*texture->channels;
    tex_init_data.SysMemSlicePitch = texture->width*texture->height*texture->channels;

    ID3D11Texture2D* d3d_tex = NULL;
    HR(renderer->device->lpVtbl->CreateTexture2D(renderer->device, &tex_desc, &tex_init_data, &d3d_tex));

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {0};
    srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MipLevels = 1;

    ID3D11ShaderResourceView* srv;
    HR(renderer->device->lpVtbl->CreateShaderResourceView(renderer->device, (ID3D11Resource*)d3d_tex, &srv_desc, &srv));
    texture->api_handle = (void*)srv;

    com_release(d3d_tex);
}

internal void renderer_draw_texture(Renderer* renderer, Texture* texture, v2 pos, v2 dim)
{
    D3D11_MAPPED_SUBRESOURCE mapped;
    renderer->ctx->lpVtbl->Map(renderer->ctx, (ID3D11Resource*)renderer->vertex_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0,
                               &mapped);
    Vertex* verts = (Vertex*)mapped.pData;

    f32 x = pos.x;
    f32 y = pos.y;
    f32 w = dim.x;
    f32 h = dim.y;
    verts[0] = (Vertex){ pos,              v2(0.0f, 1.0f) };
    verts[1] = (Vertex){ v2(x + w, y),     v2(1.0f, 1.0f) };
    verts[2] = (Vertex){ v2(x,     y + h), v2(0.0f, 0.0f) };
    verts[3] = (Vertex){ v2(x + w, y + h), v2(1.0f, 0.0f) };

    renderer->ctx->lpVtbl->Unmap(renderer->ctx, (ID3D11Resource*)renderer->vertex_buffer, 0);

    renderer->ctx->lpVtbl->PSSetSamplers(renderer->ctx, 0, 1, &renderer->sampler_state);
    renderer->ctx->lpVtbl->PSSetShaderResources(renderer->ctx, 0, 1, (ID3D11ShaderResourceView**)&texture->api_handle);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    renderer->ctx->lpVtbl->IASetInputLayout(renderer->ctx, renderer->input_layout);
    renderer->ctx->lpVtbl->IASetVertexBuffers(renderer->ctx, 0, 1, &renderer->vertex_buffer, &stride, &offset);
    renderer->ctx->lpVtbl->IASetIndexBuffer(renderer->ctx, renderer->index_buffer, DXGI_FORMAT_R16_UINT, 0);
    renderer->ctx->lpVtbl->IASetPrimitiveTopology(renderer->ctx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    renderer->ctx->lpVtbl->VSSetShader(renderer->ctx, renderer->vertex_shader, NULL, 0);
    renderer->ctx->lpVtbl->PSSetShader(renderer->ctx, renderer->pixel_shader, NULL, 0);
    renderer->ctx->lpVtbl->OMSetBlendState(renderer->ctx, renderer->blend_state, NULL, 0xffffffff);

    renderer->ctx->lpVtbl->DrawIndexed(renderer->ctx, 6, 0, 0);
}

internal void renderer_clear(Renderer* renderer, v4 clear_color)
{
    renderer->ctx->lpVtbl->OMSetRenderTargets(renderer->ctx, 1, &renderer->render_target_view, NULL);
    renderer->ctx->lpVtbl->ClearRenderTargetView(renderer->ctx, renderer->render_target_view, clear_color.e);
}

internal void renderer_begin_frame(Renderer* renderer, Window* window)
{
    (void)window;
    renderer_set_projection(renderer, renderer->proj);
}

internal void renderer_end_frame(Renderer* renderer)
{
    HR(renderer->swap_chain->lpVtbl->Present(renderer->swap_chain, 1, 0));
}
