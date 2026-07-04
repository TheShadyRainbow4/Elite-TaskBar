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

enum class TrayOverflowMode {
    VistaInline,
    Win7Flyout
};

struct EliteTaskbarConfig {
    std::wstring Theme;
    TaskbarMode Mode;
    ButtonWidthMode ButtonWidth;
    TrayOverflowMode OverflowMode;
    int FixedWidthSize;
    bool ShowPreviews;
    bool UseNativeTaskBand;
    std::vector<HMONITOR> MonitorTargets;
};

extern EliteTaskbarConfig g_Config;
void QueryOperationalMode();
