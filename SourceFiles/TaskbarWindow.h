#pragma once
#include <windows.h>
#include <commctrl.h>
#include "StartButton.h"
#include "ClockWidget.h"
#include "EverythingToolbar.h"

struct TaskbarInstance {
    HWND hTaskbar;
    HWND hSysPager;
    HWND hToolbar;
    HWND hReBar;
    HWND hTaskSwitch;
    HIMAGELIST hImageList;
    HMONITOR hMonitor;
    RECT monitorRect;
    int taskbarHeight;
    class StartButton* startButton;
    class EverythingToolbar* pEverythingToolbar;
};

namespace TaskbarWindow {
    bool Initialize(HINSTANCE hInstance);
    void RunMessageLoop();
    void Cleanup();
}
