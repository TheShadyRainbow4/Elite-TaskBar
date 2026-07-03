#include "EverythingToolbar.h"
#include <commctrl.h>
#include <shlwapi.h>

#pragma comment(lib, "comctl32.lib")

EverythingToolbar::EverythingToolbar() : hToolbar(NULL), hEdit(NULL), hFlyout(NULL), hListbox(NULL) {
}

EverythingToolbar::~EverythingToolbar() {
    Destroy();
}

LRESULT CALLBACK EverythingToolbar::FlyoutWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);
        HBRUSH hbr = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hdc, &rc, hbr);
        DeleteObject(hbr);
        EndPaint(hWnd, &ps);
        return 0;
    }
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
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

    WNDCLASSW wc = {0};
    wc.lpfnWndProc = FlyoutWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"Elite_EverythingFlyout";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wc);

    hFlyout = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, L"Elite_EverythingFlyout", L"",
        WS_POPUP | WS_BORDER | WS_CLIPCHILDREN,
        0, 0, 400, 300, NULL, NULL, hInstance, NULL);

    hListbox = CreateWindowExW(0, L"LISTBOX", L"",
        WS_CHILD | WS_VISIBLE | LBS_STANDARD | LBS_HASSTRINGS | LBS_NOTIFY,
        0, 0, 400, 300, hFlyout, (HMENU)1002, hInstance, NULL);

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
    if (hFlyout) {
        DestroyWindow(hFlyout);
        hFlyout = NULL;
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
            WCHAR text[256];
            GetWindowTextW(hWnd, text, 256);
            if (wcslen(text) > 0) {
                // Show flyout
                RECT rcEdit;
                GetWindowRect(hWnd, &rcEdit);
                SetWindowPos(pThis->hFlyout, HWND_TOPMOST, rcEdit.left, rcEdit.top - 300, 400, 300, SWP_SHOWWINDOW);
                SetWindowPos(pThis->hListbox, NULL, 0, 0, 400, 300, SWP_NOZORDER);
                
                SendMessageW(pThis->hListbox, LB_RESETCONTENT, 0, 0);

                // Query Everything SDK v3
                EVERYTHING3_CLIENT *client = Everything3_ConnectW(NULL);
                if (client) {
                    EVERYTHING3_SEARCH_STATE *search_state = Everything3_CreateSearchState();
                    if (search_state) {
                        Everything3_SetSearchTextW(search_state, text);
                        EVERYTHING3_RESULT_LIST *result_list = Everything3_Search(client, search_state);
                        
                        if (result_list) {
                            SIZE_T numResults = Everything3_GetResultListViewportCount(result_list);
                            if (numResults > 50) numResults = 50; // Cap to 50 results
                            
                            for (SIZE_T i = 0; i < numResults; i++) {
                                WCHAR filename[MAX_PATH];
                                Everything3_GetResultFullPathNameW(result_list, i, filename, MAX_PATH);
                                SendMessageW(pThis->hListbox, LB_ADDSTRING, 0, (LPARAM)filename);
                            }
                            Everything3_DestroyResultList(result_list);
                        }
                        Everything3_DestroySearchState(search_state);
                    }
                    Everything3_DestroyClient(client);
                }
            } else {
                ShowWindow(pThis->hFlyout, SW_HIDE);
            }
        }
        break;
    }
    case WM_KILLFOCUS: {
        // Hide flyout if losing focus, unless to the flyout itself
        HWND hTarget = (HWND)wParam;
        if (hTarget != pThis->hFlyout && hTarget != pThis->hListbox) {
            ShowWindow(pThis->hFlyout, SW_HIDE);
        }
        break;
    }
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
