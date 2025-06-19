#include "types.h"
#include "window.c"

#include <windows.h>
#include <crtdbg.h>
#include <d3d11.h>

#define com_release(obj) if(obj){obj->lpVtbl->Release(obj); obj = 0;}

#ifdef GRAPPLE_DEBUG
	#ifndef HR
	#define HR(x) do                                                                                                 \
	{                                                                                                                \
		HRESULT hr__ = (x);                                                                                          \
		if(FAILED(hr__))                                                                                             \
		{                                                                                                            \
            char hr_msg__[512];                                                                                      \
            char hr_buf__[1024];                                                                                     \
            FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, hr__, 0, hr_msg__,      \
                           sizeof(hr_msg__), NULL);                                                                  \
            StringCchPrintfA(hr_buf__, sizeof(hr_buf__), "D3D call failed at %s:%d, HRESULT: 0x%08X\n\nMessage: %s", \
                             __FILE__, __LINE__, hr__, hr_msg__);                                                    \
			MessageBoxA(NULL, hr_buf__, "Direct3D Error", MB_OK | MB_ICONERROR);                                     \
            ExitProcess(1);                                                                                          \
		}                                                                                                            \
	} while(0)
	#endif
#else
	#ifndef HR
	#define HR(x) (x)
	#endif
#endif

global IDXGISwapChain* swap_chain;
global ID3D11Device* device;
global ID3D11DeviceContext* immediate_context;
global ID3D11RenderTargetView* render_target_view;

void init_d3d11(Window* window)
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

    DXGI_SWAP_CHAIN_DESC sd = {0};
    sd.BufferDesc.Width = window->width;
    sd.BufferDesc.Height = window->height;
    sd.BufferDesc.RefreshRate.Numerator = 60;
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
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

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
    HR(device->lpVtbl->CreateRenderTargetView(device, (ID3D11Resource*)back_buffer, 0, &render_target_view));
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
}

void release_d3d11()
{
    com_release(device);
    com_release(immediate_context);
    com_release(swap_chain);
    com_release(render_target_view);
}

int main()
{
#ifdef GRAPPLE_DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    Window* window = window_create("Grapple", 800, 600);
    init_d3d11(window);

    ASSERT(immediate_context, "D3D immediate context is null");
    ASSERT(swap_chain, "D3D swap chain is null");

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

        immediate_context->lpVtbl->ClearRenderTargetView(immediate_context, render_target_view,
                                                         (float[4]){0.0f, 0.0f, 0.0f, 0.0f});
        swap_chain->lpVtbl->Present(swap_chain, 0, 0);
    }

    release_d3d11();
    return 0;
}
