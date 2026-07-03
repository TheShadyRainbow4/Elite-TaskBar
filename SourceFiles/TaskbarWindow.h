#pragma once
#include <windows.h>
#include <commctrl.h>

struct TaskbarInstance {
    HWND hTaskbar;
    HWND hTrayNotify;
    HWND hTrayClock;
    HWND hSysPager;
    HWND hToolbar;
    HWND hReBar;
    HWND hTaskSwitch;
    HIMAGELIST hImageList;
    HMONITOR hMonitor;
    RECT monitorRect;
    int taskbarHeight;
    class StartButton* startButton;
};

namespace TaskbarWindow {
    bool Initialize(HINSTANCE hInstance);
    void RunMessageLoop();
    void Cleanup();
}
