#pragma once
#include <windows.h>

namespace DesktopWindow {
    bool Initialize();
    void Cleanup();
    HWND GetHWND();
}
