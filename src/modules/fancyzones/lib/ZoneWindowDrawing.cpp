#include "pch.h"
#include "ZoneWindowDrawing.h"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include <common/logger/logger.h>

namespace NonLocalizable
{
    const wchar_t SegoeUiFont[] = L"Segoe ui";
}

float ZoneWindowDrawing::GetAnimationAlpha()
{
    // Lock is being held
    if (!m_animation)
    {
        return 1.f;
    }

    auto tNow = std::chrono::steady_clock().now();
    auto alpha = (tNow - m_animation->tStart).count() / (1e6f * m_animation->duration);
    if (alpha < 1.f)
    {
        return alpha;
    }
    else
    {
        return 1.f;
    }
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
    return D2D1::RectF((float)rect.left + 0.5f, (float)rect.top + 0.5f, (float)rect.right - 0.5f, (float)rect.bottom - 0.5f);
}

ZoneWindowDrawing::ZoneWindowDrawing(HWND window)
{
    m_window = window;
    try
    {
        m_dxResources = std::make_unique<DX::DeviceResourcesHwnd>();
        m_dxResources->SetHwnd(window);
    }
    catch (HRESULT hr)
    {
        m_dxResources = nullptr;
        Logger::error("couldn't initialize ZoneWindowDrawing: m_dxResources failed to initialize: {}", hr);
        return;
    }

    m_renderThread = std::thread([this]() {
        while (!m_abortThread)
        {
            // Force repeated rendering while in the animation loop.
            // Yield if low latency locking was requested
            if (!m_lowLatencyLock)
            {
                float animationAlpha;
                {
                    std::unique_lock lock(m_mutex);
                    animationAlpha = GetAnimationAlpha();
                }

                if (animationAlpha < 1.f)
                {
                    m_shouldRender = true;
                }
            }

            Render();
        }
    });
}

void ZoneWindowDrawing::Render()
{
    std::unique_lock lock(m_mutex);

    if (!m_dxResources)
    {
        return;
    }

    m_cv.wait(lock, [this]() { return (bool)m_shouldRender; });

    auto renderTarget = m_dxResources->GetD2DDeviceContext();
    renderTarget->BeginDraw();
    
    float animationAlpha = GetAnimationAlpha();

    // Draw backdrop
    renderTarget->Clear(D2D1::ColorF(0.f, 0.f, 0.f, 0.f));

    ID2D1SolidColorBrush* textBrush = nullptr;
    IDWriteTextFormat* textFormat = nullptr;

    renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black, animationAlpha), &textBrush);
    auto writeFactory = m_dxResources->GetDWriteFactory();

    if (writeFactory)
    {
        writeFactory->CreateTextFormat(NonLocalizable::SegoeUiFont, nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 80.f, L"en-US", &textFormat);
    }

    for (auto drawableRect : m_sceneRects)
    {
        ID2D1SolidColorBrush* borderBrush = nullptr;
        ID2D1SolidColorBrush* fillBrush = nullptr;

        // Need to copy the rect from m_sceneRects
        drawableRect.borderColor.a *= animationAlpha;
        drawableRect.fillColor.a *= animationAlpha;

        renderTarget->CreateSolidColorBrush(drawableRect.borderColor, &borderBrush);
        renderTarget->CreateSolidColorBrush(drawableRect.fillColor, &fillBrush);

        if (fillBrush)
        {
            renderTarget->FillRectangle(drawableRect.rect, fillBrush);
            fillBrush->Release();
        }

        if (borderBrush)
        {
            renderTarget->DrawRectangle(drawableRect.rect, borderBrush);
            borderBrush->Release();
        }

        std::wstring idStr = std::to_wstring(drawableRect.id + 1);

        if (textFormat && textBrush)
        {
            textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            renderTarget->DrawTextW(idStr.c_str(), (UINT32)idStr.size(), textFormat, drawableRect.rect, textBrush);
        }
    }

    if (textFormat)
    {
        textFormat->Release();
    }

    if (textBrush)
    {
        textBrush->Release();
    }

    renderTarget->EndDraw();
    m_dxResources->Present();

    m_shouldRender = false;
}

void ZoneWindowDrawing::Hide()
{
    m_lowLatencyLock = true;
    std::unique_lock lock(m_mutex);
    m_lowLatencyLock = false;

    if (m_animation)
    {
        m_animation.reset();
        ShowWindow(m_window, SW_HIDE);
    }
}

void ZoneWindowDrawing::Show(unsigned animationMillis)
{
    m_lowLatencyLock = true;
    std::unique_lock lock(m_mutex);
    m_lowLatencyLock = false;

    if (!m_animation)
    {
        ShowWindow(m_window, SW_SHOWNA);
        if (animationMillis > 0)
        {
            m_animation.emplace(AnimationInfo{ std::chrono::steady_clock().now(), animationMillis });
        }
        m_shouldRender = true;
        m_cv.notify_all();
    }
}

void ZoneWindowDrawing::DrawActiveZoneSet(const IZoneSet::ZonesMap& zones,
                       const std::vector<size_t>& highlightZones,
                       winrt::com_ptr<IZoneWindowHost> host)
{
    m_lowLatencyLock = true;
    std::unique_lock lock(m_mutex);
    m_lowLatencyLock = false;

    m_sceneRects = {};

    auto borderColor = ConvertColor(host->GetZoneBorderColor());
    auto inactiveColor = ConvertColor(host->GetZoneColor());
    auto highlightColor = ConvertColor(host->GetZoneHighlightColor());

    inactiveColor.a = host->GetZoneHighlightOpacity() / 100.f;
    highlightColor.a = host->GetZoneHighlightOpacity() / 100.f;

    std::vector<bool> isHighlighted(zones.size() + 1, false);
    for (size_t x : highlightZones)
    {
        isHighlighted[x] = true;
    }

    // First draw the inactive zones
    for (const auto& [zoneId, zone] : zones)
    {
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
    for (const auto& [zoneId, zone] : zones)
    {
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

    m_shouldRender = true;
    m_cv.notify_all();
}

void ZoneWindowDrawing::ForceRender()
{
    m_lowLatencyLock = true;
    std::unique_lock lock(m_mutex);
    m_lowLatencyLock = false;
    m_shouldRender = true;
    m_cv.notify_all();
}

ZoneWindowDrawing::~ZoneWindowDrawing()
{
    {
        std::unique_lock lock(m_mutex);
        m_abortThread = true;
        m_shouldRender = true;
    }
    m_cv.notify_all();
    m_renderThread.join();
}
