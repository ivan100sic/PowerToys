#pragma once

#include <map>
#include <vector>
#include "common/Display/DirectX.h"

#include "util.h"
#include "Zone.h"
#include "ZoneSet.h"
#include "FancyZones.h"

class ZoneWindowDrawing
{
    struct DrawableRect
    {
        D2D1_RECT_F rect;
        D2D1_COLOR_F borderColor;
        D2D1_COLOR_F fillColor;
        size_t id;
    };

    struct AnimationInfo
    {
        std::chrono::steady_clock::time_point tStart;
        unsigned duration;
    };

    HWND m_window = nullptr;
    std::unique_ptr<DX::DeviceResourcesHwnd> m_dxResources;
    std::optional<AnimationInfo> m_animation;

    std::mutex m_mutex;
    std::vector<DrawableRect> m_sceneRects;

    float GetAnimationAlpha();
    static D2D1_COLOR_F ConvertColor(COLORREF color);
    static D2D1_RECT_F ConvertRect(RECT rect);
    void Render();

    std::atomic<bool> m_shouldRender = false;
    std::atomic<bool> m_abortThread = false;
    std::atomic<bool> m_lowLatencyLock = false;
    std::condition_variable m_cv;
    std::thread m_renderThread;

public:

    ~ZoneWindowDrawing();
    ZoneWindowDrawing(HWND window);
    void Hide();
    void Show(unsigned animationMillis);
    void ForceRender();
    void DrawActiveZoneSet(const IZoneSet::ZonesMap& zones,
                           const std::vector<size_t>& highlightZones,
                           winrt::com_ptr<IZoneWindowHost> host);
};
