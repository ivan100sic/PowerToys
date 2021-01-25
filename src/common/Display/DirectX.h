#pragma once

#define NOMINMAX
#include <winrt/base.h>
#include <d3d11_4.h>
#include <d2d1_3.h>
#include <dwrite_3.h>
#include <wincodec.h>

#include <algorithm>

namespace DX
{
    void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            throw hr;
        }
    }

#if defined(_DEBUG)
    // Check for SDK Layer support.
    inline bool SdkLayersAvailable()
    {
        HRESULT hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_NULL, // There is no need to create a real hardware device.
            0,
            D3D11_CREATE_DEVICE_DEBUG, // Check for the SDK layers.
            nullptr, // Any feature level will do.
            0,
            D3D11_SDK_VERSION, // Always set this to D3D11_SDK_VERSION for Windows Runtime apps.
            nullptr, // No need to keep the D3D device reference.
            nullptr, // No need to know the feature level.
            nullptr // No need to keep the D3D device context reference.
        );

        return SUCCEEDED(hr);
    }
#endif

    struct IDeviceNotify
    {
        virtual void OnDeviceLost() = 0;
        virtual void OnDeviceRestored() = 0;
    };

    class DeviceResources
    {
        DeviceResources()
        {
            CreateDeviceIndependentResources();
        }

    private:
        winrt::com_ptr<ID3D11Device5> m_d3dDevice;
        winrt::com_ptr<ID3D11DeviceContext4> m_d3dContext;
        winrt::com_ptr<ID2D1Factory7> m_d2dFactory;
        winrt::com_ptr<ID2D1Device5> m_d2dDevice;
        winrt::com_ptr<IDWriteFactory3> m_dwriteFactory;
        winrt::com_ptr<IWICImagingFactory2> m_wicFactory;
        winrt::com_ptr<ID2D1DeviceContext6> m_d2dContext;
        winrt::com_ptr<IDXGISwapChain4> m_swapChain;
        winrt::com_ptr<ID3D11RenderTargetView> m_d3dRenderTargetView;
        winrt::com_ptr<ID3D11DepthStencilView> m_d3dDepthStencilView;
        winrt::com_ptr<ID2D1Bitmap1> m_d2dTargetBitmap;

        HWND m_window{};
        D2D1_SIZE_U m_outputSize;
        D3D11_VIEWPORT m_screenViewport;
        D3D_FEATURE_LEVEL m_d3dFeatureLevel;

        // The IDeviceNotify can be held directly as it owns the DeviceResources.
        IDeviceNotify* m_deviceNotify;

        void CreateDeviceIndependentResources()
        {
            // Initialize Direct2D resources.
            D2D1_FACTORY_OPTIONS options;
            ZeroMemory(&options, sizeof(options));

#if defined(_DEBUG)
            // If the project is in a debug build, enable Direct2D debugging via SDK Layers.
            options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

            // Initialize the Direct2D Factory.
            DX::ThrowIfFailed(
                D2D1CreateFactory(
                    D2D1_FACTORY_TYPE_SINGLE_THREADED,
                    __uuidof(ID2D1Factory7),
                    &options,
                    m_d2dFactory.put_void()));

            // Initialize the DirectWrite Factory.
            DX::ThrowIfFailed(
                DWriteCreateFactory(
                    DWRITE_FACTORY_TYPE_SHARED,
                    __uuidof(IDWriteFactory3),
                    reinterpret_cast<IUnknown**>(m_dwriteFactory.put_void())));

            // Initialize the Windows Imaging Component (WIC) Factory.
            DX::ThrowIfFailed(
                CoCreateInstance(
                    CLSID_WICImagingFactory2,
                    nullptr,
                    CLSCTX_INPROC_SERVER,
                    __uuidof(IWICImagingFactory2),
                    m_wicFactory.put_void()));
        }
        
        void CreateDeviceResources()
        {
            // This flag adds support for surfaces with a different color channel ordering
            // than the API default. It is required for compatibility with Direct2D.
            UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
            if (DX::SdkLayersAvailable())
            {
                // If the project is in a debug build, enable debugging via SDK Layers with this flag.
                creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
            }
#endif
            // This array defines the set of DirectX hardware feature levels this app will support.
            // Note the ordering should be preserved.
            // Don't forget to declare your application's minimum required feature level in its
            // description.  All applications are assumed to support 9.1 unless otherwise stated.
            D3D_FEATURE_LEVEL featureLevels[] = {
                D3D_FEATURE_LEVEL_11_1,
                D3D_FEATURE_LEVEL_11_0,
                D3D_FEATURE_LEVEL_10_1,
                D3D_FEATURE_LEVEL_10_0,
                D3D_FEATURE_LEVEL_9_3,
                D3D_FEATURE_LEVEL_9_2,
                D3D_FEATURE_LEVEL_9_1
            };

            // Create the Direct3D 11 API device object and a corresponding context.
            winrt::com_ptr<ID3D11Device> device;
            winrt::com_ptr<ID3D11DeviceContext> context;

            HRESULT hr = D3D11CreateDevice(
                nullptr, // Specify nullptr to use the default adapter.
                D3D_DRIVER_TYPE_HARDWARE, // Create a device using the hardware graphics driver.
                0, // Should be 0 unless the driver is D3D_DRIVER_TYPE_SOFTWARE.
                creationFlags, // Set debug and Direct2D compatibility flags.
                featureLevels, // List of feature levels this app can support.
                ARRAYSIZE(featureLevels), // Size of the list above.
                D3D11_SDK_VERSION, // Always set this to D3D11_SDK_VERSION for Windows Runtime apps.
                device.put(), // Returns the Direct3D device created.
                &m_d3dFeatureLevel, // Returns feature level of device created.
                context.put() // Returns the device immediate context.
            );

            if (FAILED(hr))
            {
                // If the initialization fails, fall back to the WARP device.
                // For more information on WARP, see:
                // http://go.microsoft.com/fwlink/?LinkId=286690
                DX::ThrowIfFailed(
                    D3D11CreateDevice(
                        nullptr,
                        D3D_DRIVER_TYPE_WARP, // Create a WARP device instead of a hardware device.
                        0,
                        creationFlags,
                        featureLevels,
                        ARRAYSIZE(featureLevels),
                        D3D11_SDK_VERSION,
                        device.put(),
                        &m_d3dFeatureLevel,
                        context.put()));
            }

            // Store pointers to the Direct3D 11.1 API device and immediate context.
            DX::ThrowIfFailed(device->QueryInterface(m_d3dDevice.put()));

            DX::ThrowIfFailed(context->QueryInterface(m_d3dContext.put()));

            // Create the Direct2D device object and a corresponding context.
            winrt::com_ptr<IDXGIDevice3> dxgiDevice;
            DX::ThrowIfFailed(m_d3dDevice->QueryInterface(dxgiDevice.put()));

            DX::ThrowIfFailed(m_d2dFactory->CreateDevice(dxgiDevice.get(), m_d2dDevice.put()));

            winrt::com_ptr<ID2D1DeviceContext5> deviceContext;

            DX::ThrowIfFailed(
                m_d2dDevice->CreateDeviceContext(
                    D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
                    deviceContext.put()));

            deviceContext->QueryInterface(m_d2dContext.put());
        }

        void SetHwnd(HWND window)
        {
            m_window = window;
            CreateWindowSizeDependentResources();
        }

        void CreateWindowSizeDependentResources()
        {
            // Clear the previous window size specific context.
            ID3D11RenderTargetView* nullViews[] = { nullptr };
            m_d3dContext->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
            m_d3dRenderTargetView = nullptr;
            m_d2dContext->SetTarget(nullptr);
            m_d2dTargetBitmap = nullptr;
            m_d3dDepthStencilView = nullptr;
            m_d3dContext->Flush();

            // Calculate the necessary swap chain and render target size in pixels.
            RECT rect;
            if (!GetClientRect(m_window, &rect))
            {
                throw E_FAIL;
            }

            m_outputSize.width = std::max(1L, rect.right - rect.left);
            m_outputSize.height = std::max(1L, rect.bottom - rect.top);

            if (m_swapChain)
            {
                HRESULT hr = m_swapChain->ResizeBuffers(
                    2, // Double-buffered swap chain.
                    m_outputSize.width,
                    m_outputSize.height,
                    DXGI_FORMAT_B8G8R8A8_UNORM,
                    0);

                if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
                {
                    // If the device was removed for any reason, a new device and swap chain will need to be created.
                    HandleDeviceLost();

                    // Everything is set up now. Do not continue execution of this method. HandleDeviceLost will reenter this method
                    // and correctly set up the new device.
                    return;
                }
                else
                {
                    DX::ThrowIfFailed(hr);
                }
            }
            else
            {
                // Otherwise, create a new one using the same adapter as the existing Direct3D device.
                DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };

                swapChainDesc.Width = rect.right - rect.left; // Match the size of the window.
                swapChainDesc.Height = rect.bottom - rect.top;
                swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // This is the most common swap chain format.
                swapChainDesc.Stereo = false;
                swapChainDesc.SampleDesc.Count = 1; // Don't use multi-sampling.
                swapChainDesc.SampleDesc.Quality = 0;
                swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
                swapChainDesc.BufferCount = 2; // Use double-buffering to minimize latency.
                swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // All Windows Runtime apps must use this SwapEffect.
                swapChainDesc.Flags = 0;
                swapChainDesc.Scaling = DXGI_SCALING_NONE;
                swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

                winrt::com_ptr<IDXGIDevice3> dxgiDevice;
                DX::ThrowIfFailed(m_d3dDevice->QueryInterface(dxgiDevice.put()));

                winrt::com_ptr<IDXGIAdapter> adapter;
                winrt::com_ptr<IDXGIFactory> dxgiFactory;
                winrt::com_ptr<IDXGIFactory2> dxgiFactory2;
                winrt::com_ptr<IDXGISwapChain1> swapChain;

                DX::ThrowIfFailed(dxgiDevice->GetAdapter(adapter.put()));
                DX::ThrowIfFailed(adapter->GetParent(__uuidof(IDXGIFactory), dxgiFactory.put_void()));
                DX::ThrowIfFailed(dxgiFactory->QueryInterface(dxgiFactory2.put()));

                DX::ThrowIfFailed(dxgiFactory2->CreateSwapChainForHwnd(m_d3dDevice.get(), m_window, &swapChainDesc, NULL, NULL, swapChain.put()));
                swapChain->QueryInterface(m_swapChain.put());
                DX::ThrowIfFailed(dxgiDevice->SetMaximumFrameLatency(1));
            }

            // Create a render target view of the swap chain back buffer.
            winrt::com_ptr<ID3D11Texture2D> backBuffer;
            DX::ThrowIfFailed(
                m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)));

            DX::ThrowIfFailed(
                m_d3dDevice->CreateRenderTargetView(
                    backBuffer.get(),
                    nullptr,
                    m_d3dRenderTargetView.put()));

            // Create a depth stencil view for use with 3D rendering if needed.
            CD3D11_TEXTURE2D_DESC depthStencilDesc(
                DXGI_FORMAT_D24_UNORM_S8_UINT,
                m_outputSize.width,
                m_outputSize.height,
                1, // This depth stencil view has only one texture.
                1, // Use a single mipmap level.
                D3D11_BIND_DEPTH_STENCIL);

            winrt::com_ptr<ID3D11Texture2D> depthStencil;
            DX::ThrowIfFailed(
                m_d3dDevice->CreateTexture2D(
                    &depthStencilDesc,
                    nullptr,
                    depthStencil.put()));

            CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
            DX::ThrowIfFailed(
                m_d3dDevice->CreateDepthStencilView(
                    depthStencil.get(),
                    &depthStencilViewDesc,
                    m_d3dDepthStencilView.put()));

            // Set the 3D rendering viewport to target the entire window.
            m_screenViewport = CD3D11_VIEWPORT(
                0.0f,
                0.0f,
                (float)m_outputSize.width,
                (float)m_outputSize.height);

            m_d3dContext->RSSetViewports(1, &m_screenViewport);

            // Create a Direct2D target bitmap associated with the
            // swap chain back buffer and set it as the current target.
            D2D1_BITMAP_PROPERTIES1 bitmapProperties =
                D2D1::BitmapProperties1(
                    D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                    D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
                    96.f,
                    96.f);

            winrt::com_ptr<IDXGISurface2> dxgiBackBuffer;
            DX::ThrowIfFailed(
                m_swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer)));

            DX::ThrowIfFailed(
                m_d2dContext->CreateBitmapFromDxgiSurface(
                    dxgiBackBuffer.get(),
                    &bitmapProperties,
                    m_d2dTargetBitmap.put()));

            m_d2dContext->SetTarget(m_d2dTargetBitmap.get());

            // Grayscale text anti-aliasing is recommended for all Windows Runtime apps.
            m_d2dContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
        }

        void HandleDeviceLost()
        {
            m_swapChain = nullptr;
         
            if (m_deviceNotify)
            {
                m_deviceNotify->OnDeviceLost();
            }

            CreateDeviceResources();
            CreateWindowSizeDependentResources();

            if (m_deviceNotify)
            {
                m_deviceNotify->OnDeviceRestored();
            }
        }

        // Register our DeviceNotify to be informed on device lost and creation.
        void RegisterDeviceNotify(IDeviceNotify* deviceNotify)
        {
            m_deviceNotify = deviceNotify;
        }

        // Call this method when the app suspends. It provides a hint to the driver that the app
        // is entering an idle state and that temporary buffers can be reclaimed for use by other apps.
        void Trim()
        {
            winrt::com_ptr<IDXGIDevice3> dxgiDevice;
            m_d3dDevice->QueryInterface(dxgiDevice.put());

            dxgiDevice->Trim();
        }

        // Present the contents of the swap chain to the screen.
        void Present()
        {
            // The first argument instructs DXGI to block until VSync, putting the application
            // to sleep until the next VSync. This ensures we don't waste any cycles rendering
            // frames that will never be displayed to the screen.
            HRESULT hr = m_swapChain->Present(1, 0);

            // Discard the contents of the render target.
            // This is a valid operation only when the existing contents will be entirely
            // overwritten. If dirty or scroll rects are used, this call should be removed.
            m_d3dContext->DiscardView(m_d3dRenderTargetView.get());

            // Discard the contents of the depth stencil.
            m_d3dContext->DiscardView(m_d3dDepthStencilView.get());

            // If the device was removed either by a disconnection or a driver upgrade, we
            // must recreate all device resources.
            if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
            {
                HandleDeviceLost();
            }
            else
            {
                ThrowIfFailed(hr);
            }
        }
    };
}