#include "font.h"
#include "win32_base.h"

#include <d3d11.h>
#include <d2d1_1.h>
#include <d2d1_1helper.h>
#include <dwrite.h>

struct TextRenderer
{
    ID2D1RenderTarget* render_target;
    ID2D1SolidColorBrush* brush;
    IDWriteTextFormat* text_format;

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

    IDWriteFactory* dwrite_factory = NULL;
    HR(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&dwrite_factory));
    HR(dwrite_factory->CreateTextFormat(L"Segoe UI", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                                         DWRITE_FONT_STRETCH_NORMAL, 16.0f, L"en-us", &tr->text_format));
    dwrite_factory->Release();

    return tr;
}

extern "C" void text_renderer_destroy(TextRenderer* tr)
{
    tr->render_target->Release();
    tr->brush->Release();
    tr->text_format->Release();
}

extern "C" void text_draw(TextRenderer* tr, s8 text, rect bounds, v4 color)
{
    wchar_t* wide_buf = push_array(&tr->scratch_arena, text.len, wchar_t);
    int wide_len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (const char*)text.data, (int)text.len,
                                       wide_buf, (int)text.len);
    ASSERT(wide_len > 0, "Conversion to UTF-16 failed!");
    if (wide_len <= 0) return;

    tr->render_target->BeginDraw();
    tr->brush->SetColor(D2D1::ColorF(color.r, color.g, color.b, color.a));
    tr->render_target->DrawText(wide_buf, (UINT32)text.len, tr->text_format,
                                D2D1::RectF(bounds.min.x, bounds.min.y, bounds.max.x, bounds.max.y), tr->brush);
    HR(tr->render_target->EndDraw());

    arena_pop(&tr->scratch_arena, text.len*sizeof(wchar_t));
}
