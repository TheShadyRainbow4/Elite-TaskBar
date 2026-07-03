#include "EverythingToolbar.h"
#include <commctrl.h>
#include <shlwapi.h>

#pragma comment(lib, "comctl32.lib")

EverythingToolbar::EverythingToolbar() : hToolbar(NULL), hEdit(NULL) {
}

EverythingToolbar::~EverythingToolbar() {
    Destroy();
}

HWND EverythingToolbar::Create(HWND hParent, HINSTANCE hInstance) {
    hToolbar = CreateWindowExW(0, TOOLBARCLASSNAMEW, NULL, 
        WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS | CCS_NODIVIDER | CCS_NORESIZE,
        0, 0, 100, 30, hParent, NULL, hInstance, NULL);

    if (!hToolbar) return NULL;

    SendMessageW(hToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);

    hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_LEFT,
        0, 0, 200, 22, hToolbar, NULL, hInstance, NULL);

    SetWindowSubclass(hToolbar, ToolbarSubclassProc, 1, (DWORD_PTR)this);
    SetWindowSubclass(hEdit, EditSubclassProc, 1, (DWORD_PTR)this);

    // Make room on toolbar for the edit control
    TBBUTTON tb = {0};
    tb.iBitmap = 200; // width
    tb.idCommand = 1001; // dummy id
    tb.fsState = TBSTATE_ENABLED;
    tb.fsStyle = BTNS_SEP;
    SendMessageW(hToolbar, TB_ADDBUTTONS, 1, (LPARAM)&tb);

    return hToolbar;
}

void EverythingToolbar::Destroy() {
    if (hToolbar) {
        RemoveWindowSubclass(hToolbar, ToolbarSubclassProc, 1);
        DestroyWindow(hToolbar);
        hToolbar = NULL;
    }
    if (hEdit) {
        RemoveWindowSubclass(hEdit, EditSubclassProc, 1);
        DestroyWindow(hEdit);
        hEdit = NULL;
    }
}

void EverythingToolbar::Resize(RECT rc) {
    if (hToolbar) {
        MoveWindow(hToolbar, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
    }
    if (hEdit) {
        MoveWindow(hEdit, 0, (rc.bottom - rc.top - 22) / 2, rc.right - rc.left, 22, TRUE);
    }
}

LRESULT CALLBACK EverythingToolbar::ToolbarSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    EverythingToolbar* pThis = (EverythingToolbar*)dwRefData;
    switch (uMsg) {
    case WM_SIZE: {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        if (pThis->hEdit) {
            MoveWindow(pThis->hEdit, 0, (height - 22) / 2, width, 22, TRUE);
        }
        break;
    }
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK EverythingToolbar::EditSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    EverythingToolbar* pThis = (EverythingToolbar*)dwRefData;
    switch (uMsg) {
    case WM_COMMAND: {
        if (HIWORD(wParam) == EN_CHANGE) {
            // Text changed! Query Everything...
            // (Flyout logic goes here in the future)
        }
        break;
    }
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
