#pragma once
#include <windows.h>
#include <string>
#include <vector>

enum class TaskbarMode {
    Independent,
    Replace
};

struct EliteTaskbarConfig {
    std::wstring Theme;
    TaskbarMode Mode;
    std::vector<HMONITOR> MonitorTargets;
};

extern EliteTaskbarConfig g_Config;
