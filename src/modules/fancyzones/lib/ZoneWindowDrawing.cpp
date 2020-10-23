#include "pch.h"
#include "ZoneWindowDrawing.h"

#include <string>

namespace NonLocalizable
{
    const wchar_t SegoeUiFont[] = L"Segoe ui";
}

void ZoneWindowDrawing::DrawBackdrop()
{
    // Lock is being held
    m_renderTarget->Clear(D2D1::ColorF(0.f, 0.f, 0.f, 0.f));
}

float ZoneWindowDrawing::GetAnimationAlpha()
{
    // Lock is being held
    if (!m_tAnimationStart)
    {
        return 1.f;
    }

    auto tNow = std::chrono::steady_clock().now();
    auto alpha = (tNow - *m_tAnimationStart).count() / (1e6f * m_animationDuration);
    if (alpha < 1.f)
    {
        return alpha;
    }
    else
    {
        return 1.f;
    }
}

ID2D1Factory* ZoneWindowDrawing::GetD2DFactory()
{
    static ID2D1Factory* pD2DFactory = nullptr;
    if (!pD2DFactory)
    {
        D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &pD2DFactory);
    }
    return pD2DFactory;

    // TODO: Destroy factory
}

IDWriteFactory* ZoneWindowDrawing::GetWriteFactory()
{
    static IUnknown* pDWriteFactory = nullptr;
    if (!pDWriteFactory)
    {
        DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), &pDWriteFactory);
    }
    return reinterpret_cast<IDWriteFactory*>(pDWriteFactory);

    // TODO: Destroy factory
}

D2D1_COLOR_F ZoneWindowDrawing::ConvertColor(COLORREF color)
{
    return D2D1::ColorF(GetRValue(color) / 255.f,
                        GetGValue(color) / 255.f,
                        GetBValue(color) / 255.f,
                        1.f);
}

D2D1_RECT_F ZoneWindowDrawing::ConvertRect(RECT rect)
{
    return D2D1::RectF((float)rect.left, (float)rect.top, (float)rect.right, (float)rect.bottom);
}

ZoneWindowDrawing::ZoneWindowDrawing(HWND window)
{
    m_window = window;
    m_renderTarget = nullptr;
    m_animationDuration = 0;

    // Obtain the size of the drawing area.
    if (!GetClientRect(window, &m_clientRect))
    {
        return;
    }

    // Create a Direct2D render target
    GetD2DFactory()->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED)),
        D2D1::HwndRenderTargetProperties(
            window,
            D2D1::SizeU(
                m_clientRect.right - m_clientRect.left,
                m_clientRect.bottom - m_clientRect.top)),
        &m_renderTarget);
}

void ZoneWindowDrawing::Render()
{
    std::unique_lock lock(m_mutex);

    if (!m_renderTarget)
    {
        return;
    }

    m_renderTarget->BeginDraw();
    DrawBackdrop();
    float animationAlpha = GetAnimationAlpha();

    for (auto drawableRect : m_sceneRects)
    {
        ID2D1SolidColorBrush* borderBrush = nullptr;
        ID2D1SolidColorBrush* fillBrush = nullptr;
        ID2D1SolidColorBrush* textBrush = nullptr;

        // Need to copy the rect from m_sceneRects
        drawableRect.borderColor.a *= animationAlpha;
        drawableRect.fillColor.a *= animationAlpha;

        m_renderTarget->CreateSolidColorBrush(drawableRect.borderColor, &borderBrush);
        m_renderTarget->CreateSolidColorBrush(drawableRect.fillColor, &fillBrush);
        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black, animationAlpha), &textBrush);

        if (fillBrush)
        {
            m_renderTarget->FillRectangle(drawableRect.rect, fillBrush);
            fillBrush->Release();
        }

        if (borderBrush)
        {
            m_renderTarget->DrawRectangle(drawableRect.rect, borderBrush);
            borderBrush->Release();
        }

        std::wstring idStr = std::to_wstring(drawableRect.id);

        IDWriteTextFormat* textFormat = nullptr;
        auto writeFactory = GetWriteFactory();

        if (writeFactory)
        {
            writeFactory->CreateTextFormat(NonLocalizable::SegoeUiFont, nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 80.f, L"en-US", &textFormat);
        }

        if (textFormat && textBrush)
        {
            textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            m_renderTarget->DrawTextW(idStr.c_str(), (UINT32)idStr.size(), textFormat, drawableRect.rect, textBrush);
        }

        if (textFormat)
        {
            textFormat->Release();
        }

        if (textBrush)
        {
            textBrush->Release();
        }
    }

    m_renderTarget->EndDraw();
}

void ZoneWindowDrawing::Hide()
{
    std::unique_lock lock(m_mutex);
    if (m_tAnimationStart)
    {
        m_tAnimationStart.reset();
        ShowWindow(m_window, SW_HIDE);
    }
}

void ZoneWindowDrawing::Show(unsigned animationMillis)
{
    std::unique_lock lock(m_mutex);
    if (!m_tAnimationStart)
    {
        ShowWindow(m_window, SW_SHOWDEFAULT);
        m_tAnimationStart = std::chrono::steady_clock().now();
        m_animationDuration = animationMillis;
    }
}

void ZoneWindowDrawing::DrawActiveZoneSet(const std::vector<winrt::com_ptr<IZone>>& zones,
                       const std::vector<size_t>& highlightZones,
                       winrt::com_ptr<IZoneWindowHost> host)
{
    std::unique_lock lock(m_mutex);

    m_sceneRects = {};

    auto borderColor = ConvertColor(host->GetZoneBorderColor());
    auto inactiveColor = ConvertColor(host->GetZoneColor());
    auto highlightColor = ConvertColor(host->GetZoneHighlightColor());

    inactiveColor.a = host->GetZoneHighlightOpacity() / 100.f;
    highlightColor.a = host->GetZoneHighlightOpacity() / 100.f;

    std::vector<bool> isHighlighted(zones.size(), false);
    for (size_t x : highlightZones)
    {
        isHighlighted[x] = true;
    }

    // First draw the inactive zones
    for (auto iter = zones.begin(); iter != zones.end(); iter++)
    {
        int zoneId = static_cast<int>(iter - zones.begin());
        winrt::com_ptr<IZone> zone = iter->try_as<IZone>();
        if (!zone)
        {
            continue;
        }

        if (!isHighlighted[zoneId])
        {
            DrawableRect drawableRect{
                .rect = ConvertRect(zone->GetZoneRect()),
                .borderColor = borderColor,
                .fillColor = inactiveColor,
                .id = zone->Id()
            };

            m_sceneRects.push_back(drawableRect);
        }
    }

    // Draw the active zones on top of the inactive zones
    for (auto iter = zones.begin(); iter != zones.end(); iter++)
    {
        int zoneId = static_cast<int>(iter - zones.begin());
        winrt::com_ptr<IZone> zone = iter->try_as<IZone>();
        if (!zone)
        {
            continue;
        }

        if (isHighlighted[zoneId])
        {
            DrawableRect drawableRect{
                .rect = ConvertRect(zone->GetZoneRect()),
                .borderColor = borderColor,
                .fillColor = highlightColor,
                .id = zone->Id()
            };

            m_sceneRects.push_back(drawableRect);
        }
    }
}
