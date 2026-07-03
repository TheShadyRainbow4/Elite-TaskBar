#pragma once
#include <windows.h>

struct TaskbarInstance {
    HWND hTaskbar;
    HWND hTrayNotify;
    HWND hTrayClock;
    HWND hSysPager;
    HWND hToolbar;
    HWND hReBar;
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
