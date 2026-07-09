#pragma once
#include <windows.h>
#include <gdiplus.h>

enum class OrbState {
    Normal,
    Hover,
    Pressed
};

class StartButton {
public:
    static bool GlobalInitialize(ULONG_PTR& token);
    static void GlobalCleanup(ULONG_PTR token);

    StartButton();
    ~StartButton();
    
    bool Initialize(HINSTANCE hInstance, HWND hParentTaskbar, int monitorIndex);
    void SetOrbImageFromResource(HINSTANCE hInstance, int resourceId);
    void ReloadOrbImage(HINSTANCE hInstance, int monitorIndex = -1);
    void Draw();
    void Show(int taskbarX, int taskbarY, int taskbarHeight);
    
    HWND GetHwnd() const { return m_hOrbWnd; }
    OrbState GetState() const { return m_internalOrbState; }
    void SetState(OrbState state) { m_internalOrbState = state; }
    bool IsTracking() const { return m_bOrbTracking; }
    void SetTracking(bool tracking) { m_bOrbTracking = tracking; }
    HWND GetParentTaskbar() const { return m_hParentTaskbar; }
    int GetMonitorIndex() const { return m_monitorIndex; }

private:
    Gdiplus::Image* m_pOrbImage;
    HWND m_hOrbWnd;
    HWND m_hParentTaskbar;
    int m_monitorIndex;
    OrbState m_internalOrbState;
    bool m_bOrbTracking;
};
