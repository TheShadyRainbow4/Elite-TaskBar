#pragma once
#include <windows.h>
#include <string>
#include <vector>

enum class TaskbarMode {
    Independent,
    Replace,
    SecondaryOnly
};

enum class ButtonWidthMode {
    Auto,
    Fixed,
    IconsOnly
};

struct EliteTaskbarConfig {
    std::wstring Theme;
    TaskbarMode Mode;
    ButtonWidthMode ButtonWidth;
    bool ShowPreviews;
    bool UseNativeTaskBand;
    std::vector<HMONITOR> MonitorTargets;
};

extern EliteTaskbarConfig g_Config;
