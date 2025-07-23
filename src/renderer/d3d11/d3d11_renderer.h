#pragma once

#include "grapple_math.h"
#include "types.h"
#include "renderer/texture.h"

#include <d3d11.h>

typedef struct TextRenderer TextRenderer;

typedef struct
{
    v2 pos; // Screen-space position
    v2 tex_coord;
} Vertex;

typedef struct
{
    m4 proj;
} CBProj;

typedef struct Renderer
{
    IDXGISwapChain* swap_chain;
    ID3D11Device* device;
    ID3D11DeviceContext* ctx;
    ID3D11RenderTargetView* render_target_view;
    ID3D11PixelShader* pixel_shader;
    ID3D11VertexShader* vertex_shader;
    ID3D11InputLayout* input_layout;
    ID3D11SamplerState* sampler_state;
    ID3D11BlendState* blend_state;

    TextRenderer* text_renderer;

    m4 proj;
    ID3D11Buffer* proj_buffer;

    Vertex* cpu_vb;
    u16* cpu_ib;

    ID3D11Buffer* vb;
    ID3D11Buffer* ib;

    i16 quads_per_batch;
    i16 quads_in_batch;
    i16 batch_count;
    i32 total_quads;
    i32 vb_size;
    i32 ib_size;

    Texture* current_texture;
} Renderer;
