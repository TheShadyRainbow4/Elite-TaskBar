#pragma once

#include <windows.h>
#include <string>
#define EVERYTHING3_USERAPI
#include "Everything3.h"

class EverythingToolbar {
public:
    EverythingToolbar();
    ~EverythingToolbar();

    HWND Create(HWND hParent, HINSTANCE hInstance);
    void Destroy();
    void Resize(RECT rc);
    
    HWND GetHwnd() const { return hToolbar; }
    HWND GetEditHwnd() const { return hEdit; }

private:
    HWND hToolbar;
    HWND hEdit;
    HWND hFlyout;
    HWND hListbox;
    
    static LRESULT CALLBACK FlyoutWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    static LRESULT CALLBACK ToolbarSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    static LRESULT CALLBACK EditSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
};
