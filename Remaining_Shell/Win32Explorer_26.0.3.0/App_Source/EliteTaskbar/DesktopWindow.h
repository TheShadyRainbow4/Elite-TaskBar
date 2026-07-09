#pragma once
#include <windows.h>

namespace DesktopWindow {
    bool Initialize();
    void Cleanup();
    bool TranslateAccelerator(MSG* pmsg);
    HWND GetHWND();
}
