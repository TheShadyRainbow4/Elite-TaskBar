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
    HIMAGELIST hImageList;
    HMONITOR hMonitor;
    int monitorIndex;
    RECT monitorRect;
    int taskbarHeight;
    class StartButton* startButton;

};

namespace TaskbarWindow {
    bool Initialize(HINSTANCE hInstance);
    void RunMessageLoop();
    void Cleanup();
}
