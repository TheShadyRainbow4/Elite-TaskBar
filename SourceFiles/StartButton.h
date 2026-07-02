#pragma once
#include <windows.h>
#include <gdiplus.h>

enum class OrbState {
    Normal,
    Hover,
    Pressed
};

namespace StartButton {
    bool Initialize(HINSTANCE hInstance, HWND hParentTaskbar);
    void Cleanup();
    void SetOrbImageFromResource(HINSTANCE hInstance, int resourceId);
    void Draw();
    void Show(int taskbarX, int taskbarY, int taskbarHeight);
}
