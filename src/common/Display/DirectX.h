#pragma once

#include <winrt/base.h>
#include <d3d11_4.h>
#include <d2d1_3.h>
#include <dwrite_3.h>
#include <wincodec.h>
#include <dcomp.h>
#pragma comment(lib, "dcomp")

#include <algorithm>

namespace DX
{
    // Check for SDK Layer support.
    inline bool SdkLayersAvailable();
    
    struct IDeviceNotify
    {
        virtual void OnDeviceLost() = 0;
        virtual void OnDeviceRestored() = 0;
    };

    class DeviceResourcesHwnd
    {
    public:
        DeviceResourcesHwnd();

        // Set output window target.
        void SetHwnd(HWND window);

        // Register our DeviceNotify to be informed on device lost and creation.
        void RegisterDeviceNotify(IDeviceNotify* deviceNotify);

        // Call this method when the app suspends. It provides a hint to the driver that the app
        // is entering an idle state and that temporary buffers can be reclaimed for use by other apps.
        void Trim();

        // Present the contents of the swap chain to the screen.
        void Present();

        // Load an SVG document from given file name. Drawing <text> SVG elements is not supported.
        winrt::com_ptr<ID2D1SvgDocument> LoadSVGFromFile(const std::wstring& fileName);

        // Device Accessors.
        D2D1_SIZE_U GetOutputSize() const { return m_outputSize; }

        // D3D Accessors.
        ID3D11Device5* GetD3DDevice() const { return m_d3dDevice.get(); }
        ID3D11DeviceContext4* GetD3DDeviceContext() const { return m_d3dContext.get(); }
        IDXGISwapChain1* GetSwapChain() const { return m_swapChain.get(); }
        D3D_FEATURE_LEVEL GetDeviceFeatureLevel() const { return m_d3dFeatureLevel; }
        ID3D11RenderTargetView* GetBackBufferRenderTargetView() const { return m_d3dRenderTargetView.get(); }
        ID3D11DepthStencilView* GetDepthStencilView() const { return m_d3dDepthStencilView.get(); }
        D3D11_VIEWPORT GetScreenViewport() const { return m_screenViewport; }

        // D2D Accessors.
        ID2D1Factory7* GetD2DFactory() const { return m_d2dFactory.get(); }
        ID2D1Device5* GetD2DDevice() const { return m_d2dDevice.get(); }
        ID2D1DeviceContext6* GetD2DDeviceContext() const { return m_d2dContext.get(); }
        ID2D1Bitmap1* GetD2DTargetBitmap() const { return m_d2dTargetBitmap.get(); }
        IDWriteFactory3* GetDWriteFactory() const { return m_dwriteFactory.get(); }
        IWICImagingFactory2* GetWicImagingFactory() const { return m_wicFactory.get(); }

    private:
        winrt::com_ptr<ID3D11Device5> m_d3dDevice;
        winrt::com_ptr<ID3D11DeviceContext4> m_d3dContext;
        winrt::com_ptr<ID2D1Factory7> m_d2dFactory;
        winrt::com_ptr<ID2D1Device5> m_d2dDevice;
        winrt::com_ptr<IDWriteFactory3> m_dwriteFactory;
        winrt::com_ptr<IWICImagingFactory2> m_wicFactory;
        winrt::com_ptr<ID2D1DeviceContext6> m_d2dContext;
        winrt::com_ptr<IDXGISwapChain1> m_swapChain;
        winrt::com_ptr<ID3D11RenderTargetView> m_d3dRenderTargetView;
        winrt::com_ptr<ID3D11DepthStencilView> m_d3dDepthStencilView;
        winrt::com_ptr<ID2D1Bitmap1> m_d2dTargetBitmap;
        winrt::com_ptr<IDCompositionDevice> m_compositionDevice;
        winrt::com_ptr<IDCompositionTarget> m_compositionTarget;
        winrt::com_ptr<IDCompositionVisual> m_compositionVisual;
        winrt::com_ptr<IDXGIFactory2> m_dxgiFactory;
        winrt::com_ptr<IDXGIDevice3> m_dxgiDevice;
        winrt::com_ptr<IDXGISurface2> m_dxgiSurface;


        HWND m_window{};
        D2D1_SIZE_U m_outputSize;
        D3D11_VIEWPORT m_screenViewport;
        D3D_FEATURE_LEVEL m_d3dFeatureLevel;

        // The IDeviceNotify can be held directly as it owns the DeviceResources.
        IDeviceNotify* m_deviceNotify;

        void CreateDeviceIndependentResources();
        void CreateDeviceResources();
        void CreateWindowSizeDependentResources();
        void HandleDeviceLost();
    };
}