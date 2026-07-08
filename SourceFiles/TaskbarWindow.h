#pragma once
#include <windows.h>
#include <commctrl.h>
#include "StartButton.h"
#include "ClockWidget.h"

struct TaskbarInstance {
    HWND hTaskbar;
    HWND hSysPager;
    HWND hToolbar;

    HWND hTaskSwitch;
    HWND hTrayNotify;
    HWND hTrayClock;
    HWND hRebar; // Container for folder-based toolbars - Builder-Bob
    HIMAGELIST hImageList;
    HIMAGELIST hTrayImageList;
    HMONITOR hMonitor;
    int monitorIndex;
    RECT monitorRect;
    int taskbarHeight;
    class StartButton* startButton;
    HWND hNativeTrayNotify;
    bool bStolenSysPager;
};

namespace TaskbarWindow {
    bool Initialize(HINSTANCE hInstance);
    void RunMessageLoop();
    void Cleanup();
}
