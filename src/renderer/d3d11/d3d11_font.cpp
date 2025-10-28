#include "renderer/font.h"
#include "platform/windows/win32_base.h"

#include "d3d11_renderer.h"

#include "grapple_memory.c"

#include <d3d11.h>
#include <d2d1_1.h>
#include <d2d1_1helper.h>
#include <dwrite.h>

struct TextRenderer
{
    ID2D1RenderTarget* render_target;
    ID2D1SolidColorBrush* brush;
    IDWriteTextFormat* text_format;
    IDWriteFactory* dwrite_factory;

    f32 font_size;

    Arena scratch_arena;
};

extern "C" TextRenderer* text_renderer_create(void* window_ptr, IDXGISwapChain* swap_chain, Arena* arena)
{
    TextRenderer* tr = push_struct(arena, TextRenderer);
    tr->scratch_arena = arena_alloc(KILOBYTES(1));

    f32 dpi = (f32)GetDpiForWindow((HWND)window_ptr);

    ID2D1Factory1* d2d_factory = NULL;
    D2D1_FACTORY_OPTIONS opts = {};
    HR(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1) ,&opts,
    (void**)&d2d_factory));

    IDXGISurface* dxgi_surface = NULL;
    HR(swap_chain->GetBuffer(0, __uuidof(IDXGISurface), (void**)&dxgi_surface));

    D2D1_RENDER_TARGET_PROPERTIES props = {};
    props.dpiX = dpi;
    props.dpiY = dpi;
    props.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    props.pixelFormat.format = DXGI_FORMAT_UNKNOWN;
    props.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;

    HR(d2d_factory->CreateDxgiSurfaceRenderTarget(dxgi_surface, &props, &tr->render_target));

    dxgi_surface->Release();
    d2d_factory->Release();

    HR(tr->render_target->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &tr->brush));

    tr->font_size = 22.0f;
    IDWriteFactory* dwrite_factory = NULL;
    HR(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&dwrite_factory));
    HR(dwrite_factory->CreateTextFormat(L"Segoe UI", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                                        DWRITE_FONT_STRETCH_NORMAL, tr->font_size, L"en-us", &tr->text_format));
    tr->text_format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    tr->dwrite_factory = dwrite_factory;

    return tr;
}

extern "C" void text_renderer_destroy(TextRenderer* tr)
{
    tr->render_target->Release();
    tr->brush->Release();
    tr->text_format->Release();
    tr->dwrite_factory->Release();
}

extern "C" f32 text_renderer_get_font_size(TextRenderer* tr)
{
    return tr->font_size;
}

extern "C" TextMetrics text_get_metrics(TextRenderer* tr, s8 text, rect bounds, size caret_idx)
{
    TextMetrics result = {0};

    if (text.len <= 0)
    {
        result.caret_pos = v2(bounds.x, bounds.y);
        return result;
    }

    // DirectWrite assumes strings are encoded in UTF-16, so need to convert the byte index
    size utf16_idx = utf8_to_utf16_offset(text, caret_idx);

    wchar_t* wide_buf = (wchar_t*)push_array(&tr->scratch_arena, text.len, u8);
    int wide_len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (const char*)text.data, (int)text.len,
                                       wide_buf, (int)text.len);
    ASSERT(wide_len > 0, "Conversion to UTF-16 failed!");
    if (wide_len <= 0)
    {
        result.caret_pos = v2(bounds.x, bounds.y);
        return result;
    }

    IDWriteTextLayout* layout = NULL;
    HR(tr->dwrite_factory->CreateTextLayout(wide_buf, wide_len, tr->text_format, bounds.w, bounds.h, &layout));

    FLOAT caret_x, caret_y;
    DWRITE_HIT_TEST_METRICS hit_test_metrics;
    HR(layout->HitTestTextPosition((u32)utf16_idx, FALSE, &caret_x, &caret_y, &hit_test_metrics));
    result.caret_pos = v2(bounds.x + caret_x, bounds.y + caret_y);

    DWRITE_TEXT_METRICS metrics;
    layout->GetMetrics(&metrics);
    result.text_width = metrics.widthIncludingTrailingWhitespace;

    layout->Release();

    zero_array(tr->scratch_arena.data, text.len, u8);
    arena_pop(&tr->scratch_arena, text.len*sizeof(u8));

    return result;
}

extern "C" void draw_text_rect(Renderer* renderer, s8 text, rect bounds, v4 color, f32 scroll)
{
    if (text.len <= 0) return;

    TextRenderer* tr = renderer->text_renderer;
    wchar_t* wide_buf = (wchar_t*)push_array(&tr->scratch_arena, text.len, u8);
    int wide_len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (const char*)text.data, (int)text.len,
                                       wide_buf, (int)text.len);
    ASSERT(wide_len > 0, "Conversion to UTF-16 failed!");
    if (wide_len <= 0) return;

    tr->render_target->BeginDraw();

    D2D_RECT_F clip_rect = D2D1::RectF(bounds.x, bounds.y, bounds.x + bounds.w, bounds.y + bounds.h);
    tr->render_target->PushAxisAlignedClip(clip_rect, D2D1_ANTIALIAS_MODE_ALIASED);

    tr->brush->SetColor(D2D1::ColorF(color.r, color.g, color.b, color.a));
    D2D_RECT_F text_rect = D2D1::RectF(bounds.x - scroll, bounds.y, bounds.x + bounds.w, bounds.y + bounds.h);

    tr->render_target->DrawText(wide_buf, wide_len, tr->text_format, text_rect, tr->brush);

    tr->render_target->PopAxisAlignedClip();

    HR(tr->render_target->EndDraw());

    zero_array(tr->scratch_arena.data, text.len, u8);
    arena_pop(&tr->scratch_arena, text.len*sizeof(u8));
}

extern "C" void draw_text(Renderer* renderer, s8 text, v2 pos, v2 dim, v4 color, f32 scroll)
{
    draw_text_rect(renderer, text, rect_min_dim(pos, dim), color, scroll);
}
