#pragma once
#include <Windows.h>
#include <vector>

struct ScreenSize
{
    explicit ScreenSize(RECT rect) :
        rect(rect) {}
    RECT rect;
    int left() const { return rect.left; }
    int right() const { return rect.right; }
    int top() const { return rect.top; }
    int bottom() const { return rect.bottom; }
    int height() const { return rect.bottom - rect.top; };
    int width() const { return rect.right - rect.left; };
    POINT top_left() const { return { rect.left, rect.top }; };
    POINT top_middle() const { return { rect.left + width() / 2, rect.top }; };
    POINT top_right() const { return { rect.right, rect.top }; };
    POINT middle_left() const { return { rect.left, rect.top + height() / 2 }; };
    POINT middle() const { return { rect.left + width() / 2, rect.top + height() / 2 }; };
    POINT middle_right() const { return { rect.right, rect.top + height() / 2 }; };
    POINT bottom_left() const { return { rect.left, rect.bottom }; };
    POINT bottom_middle() const { return { rect.left + width() / 2, rect.bottom }; };
    POINT bottom_right() const { return { rect.right, rect.bottom }; };
};

struct MonitorInfo : ScreenSize
{
    explicit MonitorInfo(HMONITOR monitor, RECT rect) :
        handle(monitor), ScreenSize(rect) {}
    HMONITOR handle;

    // Returns monitor rects ordered from left to right
    static std::vector<MonitorInfo> GetMonitors(bool includeNonWorkingArea);
    static MonitorInfo GetPrimaryMonitor();
};

bool operator==(const ScreenSize& lhs, const ScreenSize& rhs);

// Returns a handle to a monitor on which there is a full screen app running and is in the foreground,
// otherwise, return a null handle.
HMONITOR FullScreenAppRunningMonitor();


template<RECT MONITORINFO::*member>
std::vector<std::pair<HMONITOR, RECT>> GetAllMonitorRects()
{
    using result_t = std::vector<std::pair<HMONITOR, RECT>>;
    result_t result;

    auto enumMonitors = [](HMONITOR monitor, HDC hdc, LPRECT pRect, LPARAM param) -> BOOL {
        MONITORINFOEX mi;
        mi.cbSize = sizeof(mi);
        result_t& result = *reinterpret_cast<result_t*>(param);
        if (GetMonitorInfo(monitor, &mi))
        {
            result.push_back({ monitor, mi.*member });
        }

        return TRUE;
    };

    EnumDisplayMonitors(NULL, NULL, enumMonitors, reinterpret_cast<LPARAM>(&result));
    return result;
}

template<RECT MONITORINFO::*member>
RECT GetAllMonitorsCombinedRect()
{
    auto allMonitors = GetAllMonitorRects<member>();
    bool empty = true;
    RECT result{ 0, 0, 0, 0 };

    for (auto& [monitor, rect] : allMonitors)
    {
        if (empty)
        {
            empty = false;
            result = rect;
        }
        else
        {
            result.left = min(result.left, rect.left);
            result.top = min(result.top, rect.top);
            result.right = max(result.right, rect.right);
            result.bottom = max(result.bottom, rect.bottom);
        }
    }

    return result;
}
