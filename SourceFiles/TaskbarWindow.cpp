#include "TaskbarWindow.h"
#include "StartButton.h"
#include "ClockWidget.h"
#include "TaskbarProperties.h"
#include "Logger.h"
#include "resource.h"
#include "Config.h"
#include <dwmapi.h>
#include <windowsx.h>
#include <uxtheme.h>
#include <vector>
#include <windowsx.h>

static void InvokeNativeRunDialog(HWND hwndOwner);

extern EliteTaskbarConfig g_Config;

#include <initguid.h>
#include <Unknwn.h>
#include <objbase.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "ole32.lib")

DEFINE_GUID(GUID_Win32Clock,
    0x0A323554A,
    0x0FE1, 0x4E49, 0xae, 0xe1,
    0x67, 0x22, 0x46, 0x5d, 0x79, 0x9f
);
DEFINE_GUID(IID_Win32Clock,
    0x7A5FCA8A,
    0x76B1, 0x44C8, 0xa9, 0x7c,
    0xe7, 0x17, 0x3c, 0xca, 0x5f, 0x4f
);

typedef interface Win32Clock Win32Clock;

typedef struct Win32ClockVtbl {
    BEGIN_INTERFACE
    HRESULT(STDMETHODCALLTYPE* QueryInterface)(Win32Clock* This, REFIID riid, void** ppvObject);
    ULONG(STDMETHODCALLTYPE* AddRef)(Win32Clock* This);
    ULONG(STDMETHODCALLTYPE* Release)(Win32Clock* This);
    HRESULT(STDMETHODCALLTYPE* ShowWin32Clock)(Win32Clock* This, HWND hWnd, LPRECT lpRect);
    END_INTERFACE
} Win32ClockVtbl;

interface Win32Clock {
    CONST_VTBL struct Win32ClockVtbl* lpVtbl;
};

HWND g_hNativeTaskbar = NULL;
std::vector<TaskbarInstance*> g_Taskbars;

TaskbarInstance* GetTaskbarInstance(HWND hwnd) {
    if (!hwnd) return nullptr;
    // Check if hwnd is a taskbar
    for (auto* tb : g_Taskbars) {
        if (tb->hTaskbar == hwnd || tb->hTrayNotify == hwnd || tb->hTrayClock == hwnd || tb->hSysPager == hwnd || tb->hToolbar == hwnd || tb->hReBar == hwnd) {
            return tb;
        }
    }
    // Might be a child window (e.g. within TrayNotify)
    HWND hParent = GetParent(hwnd);
    if (hParent) return GetTaskbarInstance(hParent);
    return nullptr;
}

LRESULT CALLBACK TaskbarPropertiesProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

HWINEVENTHOOK g_hClockFlyoutHook = NULL;
void CALLBACK ClockFlyoutWinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {
    if (hwnd && idObject == OBJID_WINDOW) {
        WCHAR szClass[256];
        GetClassNameW(hwnd, szClass, 256);
        if (wcscmp(szClass, L"ClockFlyoutWindow") == 0) {
            RECT rc;
            GetWindowRect(hwnd, &rc);
            int width = rc.right - rc.left;
            int height = rc.bottom - rc.top;
            
            HWND hTargetTaskbar = NULL;
            if (!g_Taskbars.empty()) hTargetTaskbar = g_Taskbars[0]->hTaskbar; // Default to first
            if (hTargetTaskbar) {
                RECT rcTaskbar;
                GetWindowRect(hTargetTaskbar, &rcTaskbar);
                int x = rcTaskbar.right - width;
                int y = rcTaskbar.top - height;
                SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
            }
            
            UnhookWinEvent(g_hClockFlyoutHook);
            g_hClockFlyoutHook = NULL;
        }
    }
}

BOOL ShowLegacyClockExperience(HWND hWnd) {
    HRESULT hr = CoInitialize(NULL);
    Win32Clock* pClock = NULL;
    hr = CoCreateInstance(GUID_Win32Clock, NULL, CLSCTX_LOCAL_SERVER, IID_Win32Clock, (void**)&pClock);
    if (SUCCEEDED(hr) && pClock) {
        RECT rc;
        GetWindowRect(hWnd, &rc);
        pClock->lpVtbl->ShowWin32Clock(pClock, hWnd, &rc);
        pClock->lpVtbl->Release(pClock);
        CoUninitialize();
        return TRUE;
    }
    
    // Fallback if COM fails
    if (g_hNativeTaskbar) {
        HWND hwndNotify = FindWindowExW(g_hNativeTaskbar, NULL, L"TrayNotifyWnd", NULL);
        if (hwndNotify) {
            HWND hwndClock = FindWindowExW(hwndNotify, NULL, L"TrayClockWClass", NULL);
            if (hwndClock) {
                if (!g_hClockFlyoutHook) {
                    g_hClockFlyoutHook = SetWinEventHook(EVENT_OBJECT_SHOW, EVENT_OBJECT_SHOW, NULL, ClockFlyoutWinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT);
                }
                PostMessageW(hwndClock, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(5, 5));
                PostMessageW(hwndClock, WM_LBUTTONUP, 0, MAKELPARAM(5, 5));
                CoUninitialize();
                return TRUE;
            }
        }
    }
    
    CoUninitialize();
    Logger::Log(L"Failed to instantiate native TrayClockWClass via COM or fallback.");
    return FALSE;
}

#define CLASS_NAME L"Shell_TrayWnd"
#define TRAY_CLASS_NAME L"Elite_SecondaryTrayWnd"

#define IDM_TASKBAR_TOOLBARS        3001
#define IDM_TASKBAR_CASCADE         3002
#define IDM_TASKBAR_STACKED         3003
#define IDM_TASKBAR_SIDEBYSIDE      3004
#define IDM_TASKBAR_SHOWDESKTOP     3005
#define IDM_TASKBAR_TASKMGR         3006
#define IDM_TASKBAR_LOCK            3007
#define IDM_TASKBAR_PROPERTIES      3008
#define IDM_TASKBAR_RUN             3009
#define IDM_EXIT_ELITETASKBAR       3010
#define IDM_RESTART_SHELL           3011
#define IDM_START_EXPLORER          3012
#define IDM_TASKBAR_SETTINGS        3013

UINT g_uTaskbarCreatedMsg = 0;
OrbState g_orbState = OrbState::Normal;
bool g_bOrbTrackingMouse = false;

#include <vector>

struct TaskButtonInfo {
    HWND hwnd;
    HICON hIcon;
    std::wstring title;
    bool isActive;
};

std::vector<TaskButtonInfo> g_TaskButtons;
UINT g_uShellHookMsg = 0;

struct EliteTrayIcon {
    HWND hWnd;
    UINT uID;
    UINT uCallbackMessage;
    HICON hIcon;
    WCHAR szTip[128];
    DWORD dwState;
};

std::vector<EliteTrayIcon> g_TrayIcons;

struct TRAYDATA {
    DWORD dwSignature;
    DWORD dwMessage;
    NOTIFYICONDATAW nid;
};

LRESULT CALLBACK TrayNotifyProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_PRINTCLIENT:
    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam;
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        DrawThemeParentBackground(hwnd, hdc, &rcClient);
        return 1;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        DrawThemeParentBackground(hwnd, hdc, &rcClient);

        int x = 2;
        int y = (rcClient.bottom - rcClient.top - 16) / 2;
        for (const auto& icon : g_TrayIcons) {
            if (icon.hIcon && !(icon.dwState & NIS_HIDDEN)) {
                DrawIconEx(hdc, x, y, icon.hIcon, 16, 16, 0, NULL, DI_NORMAL);
                x += 24;
            }
        }

        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_COPYDATA: {
        COPYDATASTRUCT* pcds = (COPYDATASTRUCT*)lParam;
        if (pcds->dwData == 1 && pcds->cbData >= 8) { // Tray notification
            TRAYDATA* pTrayData = (TRAYDATA*)pcds->lpData;
            if (pTrayData->dwSignature == 0x34753423) {
                DWORD dwMessage = pTrayData->dwMessage;
                NOTIFYICONDATAW* nid = (NOTIFYICONDATAW*)((BYTE*)pTrayData + 8);

                if (dwMessage == NIM_ADD || dwMessage == NIM_MODIFY) {
                    bool found = false;
                    for (auto& icon : g_TrayIcons) {
                        if (icon.hWnd == nid->hWnd && icon.uID == nid->uID) {
                            if (nid->uFlags & NIF_ICON) icon.hIcon = CopyIcon(nid->hIcon);
                            if (nid->uFlags & NIF_TIP) wcscpy_s(icon.szTip, nid->szTip);
                            if (nid->uFlags & NIF_MESSAGE) icon.uCallbackMessage = nid->uCallbackMessage;
                            if (nid->uFlags & NIF_STATE) icon.dwState = nid->dwState;
                            found = true;
                            break;
                        }
                    }
                    if (!found && dwMessage == NIM_ADD) {
                        EliteTrayIcon newIcon = {0};
                        newIcon.hWnd = nid->hWnd;
                        newIcon.uID = nid->uID;
                        if (nid->uFlags & NIF_ICON) newIcon.hIcon = CopyIcon(nid->hIcon);
                        if (nid->uFlags & NIF_TIP) wcscpy_s(newIcon.szTip, nid->szTip);
                        if (nid->uFlags & NIF_MESSAGE) newIcon.uCallbackMessage = nid->uCallbackMessage;
                        if (nid->uFlags & NIF_STATE) newIcon.dwState = nid->dwState;
                        g_TrayIcons.push_back(newIcon);
                    }
                    InvalidateRect(hwnd, NULL, FALSE);
                }
                else if (dwMessage == NIM_DELETE) {
                    for (auto it = g_TrayIcons.begin(); it != g_TrayIcons.end(); ++it) {
                        if (it->hWnd == nid->hWnd && it->uID == nid->uID) {
                            if (it->hIcon) DestroyIcon(it->hIcon);
                            g_TrayIcons.erase(it);
                            break;
                        }
                    }
                    InvalidateRect(hwnd, NULL, FALSE);
                }
            }
        }
        return TRUE;
    }
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MOUSEMOVE:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK: {
        int xPos = GET_X_LPARAM(lParam);
        int iconIndex = (xPos - 2) / 24;
        if (iconIndex < 0) return 0;
        
        int current = 0;
        for (const auto& icon : g_TrayIcons) {
            if (icon.hIcon && !(icon.dwState & NIS_HIDDEN)) {
                if (current == iconIndex) {
                    PostMessageW(icon.hWnd, icon.uCallbackMessage, icon.uID, uMsg);
                    break;
                }
                current++;
            }
        }
        return 0;
    }
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

LRESULT CALLBACK TrayClockProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        SetTimer(hwnd, 1, 1000, NULL);
        return 0;
    case WM_HOTKEY:
        if (wParam == 1) { // Win+E
            ShellExecuteW(NULL, L"open", L"explorer.exe", L"shell:::{20D04FE0-3AEA-1069-A2D8-08002B30309D}", NULL, SW_SHOWNORMAL);
        } else if (wParam == 2) { // Win+R
            InvokeNativeRunDialog(hwnd);
        } else if (wParam == 3) { // Win+D
            SendMessageW(hwnd, WM_COMMAND, IDM_TASKBAR_SHOWDESKTOP, 0);
        }
        return 0;

    case WM_TIMER:
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);

        // Draw Clock text on Glass using BufferedPaint for Alpha preservation
        BP_PAINTPARAMS params = { sizeof(BP_PAINTPARAMS) };
        params.dwFlags = BPPF_ERASE;
        HDC hdcBuffer;
        HPAINTBUFFER hBufferedPaint = BeginBufferedPaint(hdc, &rcClient, BPBF_TOPDOWNDIB, &params, &hdcBuffer);
        
        SYSTEMTIME st;
        GetLocalTime(&st);
        wchar_t timeBuf[32];
        wchar_t dateBuf[32];
        GetTimeFormatW(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, timeBuf, 32);
        GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, dateBuf, 32);
        
        wchar_t clockText[128];
        swprintf_s(clockText, L"%s\n%s", timeBuf, dateBuf);
        
        LOGFONTW lf = {0};
        lf.lfHeight = -11; // Smaller font to fit both
        lf.lfWeight = FW_NORMAL;
        wcscpy_s(lf.lfFaceName, L"Segoe UI");

        if (hBufferedPaint) {
            DrawThemeParentBackground(hwnd, hdcBuffer, &rcClient);
            
            HTHEME hTheme = OpenThemeData(hwnd, L"Taskbar");
            if (hTheme) {
                HFONT hFont = CreateFontIndirectW(&lf);
                HFONT hOldFont = (HFONT)SelectObject(hdcBuffer, hFont);

                DTTOPTS dttOpts = { sizeof(DTTOPTS) };
                dttOpts.dwFlags = DTT_COMPOSITED | DTT_GLOWSIZE | DTT_TEXTCOLOR;
                dttOpts.iGlowSize = 0; // Neutralize glow but keep alpha channel intact
                dttOpts.crText = RGB(255, 255, 255);
                
                // Manually offset for vertical centering to avoid DT_CALCRECT bugs
                rcClient.top += 6; 
                rcClient.right -= 25; // Massive 25px buffer from the right edge to avoid any show desktop button overlap
                
                DrawThemeTextEx(hTheme, hdcBuffer, 0, 0, clockText, -1, DT_RIGHT, &rcClient, &dttOpts);
                
                SelectObject(hdcBuffer, hOldFont);
                DeleteObject(hFont);
                CloseThemeData(hTheme);
            } else {
                // Fallback if Theme fails inside BufferedPaint
                HFONT hFont = CreateFontIndirectW(&lf);
                HFONT hOldFont = (HFONT)SelectObject(hdcBuffer, hFont);
                SetTextColor(hdcBuffer, RGB(255, 255, 255));
                SetBkMode(hdcBuffer, TRANSPARENT);
                rcClient.top += 6; 
                rcClient.right -= 25;
                DrawTextW(hdcBuffer, clockText, -1, &rcClient, DT_RIGHT | DT_NOCLIP);
                SelectObject(hdcBuffer, hOldFont);
                DeleteObject(hFont);
            }
            EndBufferedPaint(hBufferedPaint, TRUE);
        } else {
            // Absolute Fallback if BufferedPaint fails completely for Standard User
            HFONT hFont = CreateFontIndirectW(&lf);
            HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
            SetTextColor(hdc, RGB(255, 255, 255));
            SetBkMode(hdc, TRANSPARENT);
            rcClient.top += 6; 
            rcClient.right -= 25;
            DrawTextW(hdc, clockText, -1, &rcClient, DT_RIGHT | DT_NOCLIP);
            SelectObject(hdc, hOldFont);
            DeleteObject(hFont);
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_LBUTTONDBLCLK:
        ShellExecuteW(NULL, L"open", L"control", L"timedate.cpl", NULL, SW_SHOWNORMAL);
        return 0;
    case WM_LBUTTONDOWN: {
        DWORD trayMode = 0;
        HKEY hKey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD cbData = sizeof(DWORD);
            RegQueryValueExW(hKey, L"TrayMode", NULL, NULL, (LPBYTE)&trayMode, &cbData);
            RegCloseKey(hKey);
        }

        if (trayMode == 1) {
            // Legacy Logic (ReactOS/WinXP style - just open date and time applet)
            ShellExecuteW(NULL, L"open", L"control", L"timedate.cpl", NULL, SW_SHOWNORMAL);
        } else {
            // Native Windows 7 Flyout
            if (!FindWindowW(L"ClockFlyoutWindow", NULL)) {
                if (!ShowLegacyClockExperience(hwnd)) {
                    Logger::Log(L"Native clock flyout COM failed. Need custom C++ flyout implementation.");
                }
            }
        }
        return 0;
    }
    case WM_DESTROY:
        KillTimer(hwnd, 1);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}
typedef void (WINAPI *RUNFILEDLG)(HWND hwndOwner, HICON hIcon, LPCWSTR lpstrDirectory, LPCWSTR lpstrTitle, LPCWSTR lpstrDescription, UINT uFlags);

static HHOOK g_hRunDlgCbtHook = NULL;
static LRESULT CALLBACK RunDlgCbtProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HCBT_ACTIVATE) {
        HWND hwnd = (HWND)wParam;
        WCHAR szClass[256];
        GetClassNameW(hwnd, szClass, 256);
        if (wcscmp(szClass, L"#32770") == 0) {
            int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);
            RECT rc;
            GetWindowRect(hwnd, &rc);
            int width = rc.right - rc.left;
            int height = rc.bottom - rc.top;
            int x = (screenWidth - width) / 2;
            int y = (screenHeight - height) / 2 + 100;
            SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
            UnhookWindowsHookEx(g_hRunDlgCbtHook);
            g_hRunDlgCbtHook = NULL;
        }
    }
    return CallNextHookEx(g_hRunDlgCbtHook, nCode, wParam, lParam);
}

static void InvokeNativeRunDialog(HWND hwndOwner) {
    HMODULE hShell32 = LoadLibraryW(L"shell32.dll");
    if (hShell32) {
        RUNFILEDLG RunFileDlg = (RUNFILEDLG)GetProcAddress(hShell32, (LPCSTR)61);
        if (RunFileDlg) {
            g_hRunDlgCbtHook = SetWindowsHookExW(WH_CBT, RunDlgCbtProc, NULL, GetCurrentThreadId());
            RunFileDlg(hwndOwner, NULL, NULL, NULL, NULL, 0);
            if (g_hRunDlgCbtHook) {
                UnhookWindowsHookEx(g_hRunDlgCbtHook);
                g_hRunDlgCbtHook = NULL;
            }
        }
        FreeLibrary(hShell32);
    }
}

LRESULT CALLBACK TrayShowDesktopButtonProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_LBUTTONDOWN:
        // Simulate Win+D
        keybd_event(VK_LWIN, 0, 0, 0);
        keybd_event('D', 0, 0, 0);
        keybd_event('D', 0, KEYEVENTF_KEYUP, 0);
        keybd_event(VK_LWIN, 0, KEYEVENTF_KEYUP, 0);
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        DrawThemeParentBackground(hwnd, hdc, &rcClient);
        
        // Draw a subtle vertical line to mimic the Windows 7 show desktop button
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        MoveToEx(hdc, 0, 0, NULL);
        LineTo(hdc, 0, rcClient.bottom);
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
        
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == g_uTaskbarCreatedMsg && g_uTaskbarCreatedMsg != 0) {
        // Re-apply AppBar reservation if Explorer restarts
        for (auto* inst : g_Taskbars) {
            APPBARDATA abd = {0};
            abd.cbSize = sizeof(APPBARDATA);
            abd.hWnd = inst->hTaskbar;
            SHAppBarMessage(ABM_NEW, &abd);
        }
        return 0;
    }
    if (uMsg == g_uShellHookMsg && g_uShellHookMsg != 0) {
        int nCode = (int)wParam;
        HWND hwndShell = (HWND)lParam;
        
        if (nCode == HSHELL_WINDOWCREATED) {
            // Ignore hidden/tooltips/zero size
            if (IsWindowVisible(hwndShell) && GetWindowTextLengthW(hwndShell) > 0) {
                TaskButtonInfo info;
                info.hwnd = hwndShell;
                info.isActive = false;
                
                WCHAR szTitle[256] = {0};
                GetWindowTextW(hwndShell, szTitle, 256);
                info.title = szTitle;
                
                DWORD_PTR dwRes;
                if (!SendMessageTimeoutW(hwndShell, WM_GETICON, ICON_SMALL, 0, SMTO_ABORTIFHUNG, 500, &dwRes)) dwRes = 0;
                info.hIcon = (HICON)dwRes;
                
                if (!info.hIcon) {
                    info.hIcon = (HICON)GetClassLongPtrW(hwndShell, GCLP_HICONSM);
                }
                
                g_TaskButtons.push_back(info);
                InvalidateRect(hwnd, NULL, TRUE);
            }
        }
        else if (nCode == HSHELL_WINDOWDESTROYED) {
            for (auto it = g_TaskButtons.begin(); it != g_TaskButtons.end(); ++it) {
                if (it->hwnd == hwndShell) {
                    g_TaskButtons.erase(it);
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;
                }
            }
        }
        else if (nCode == HSHELL_WINDOWACTIVATED || nCode == HSHELL_RUDEAPPACTIVATED) {
            for (auto& btn : g_TaskButtons) {
                btn.isActive = (btn.hwnd == hwndShell);
            }
            InvalidateRect(hwnd, NULL, TRUE);
        }
        else if (nCode == HSHELL_REDRAW) {
            for (auto& btn : g_TaskButtons) {
                if (btn.hwnd == hwndShell) {
                    WCHAR szTitle[256] = {0};
                    GetWindowTextW(hwndShell, szTitle, 256);
                    btn.title = szTitle;
                    
                    DWORD_PTR dwRes;
                    if (SendMessageTimeoutW(hwndShell, WM_GETICON, ICON_SMALL, 0, SMTO_ABORTIFHUNG, 500, &dwRes)) {
                        if (dwRes) btn.hIcon = (HICON)dwRes;
                    }
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;
                }
            }
        }
        return 0;
    }

    switch (uMsg) {
    case WM_CREATE: {
        SetTimer(hwnd, 9999, 100, NULL);
        DWM_BLURBEHIND bb = {0};
        bb.dwFlags = DWM_BB_ENABLE;
        bb.fEnable = TRUE;
        bb.hRgnBlur = NULL;
        DwmEnableBlurBehindWindow(hwnd, &bb);
        return 0;
    }
    case WM_PRINTCLIENT: {
        HDC hdc = (HDC)wParam;
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        HTHEME hTheme = OpenThemeData(hwnd, L"Taskbar");
        if (hTheme) {
            DrawThemeBackground(hTheme, hdc, 1 /*TBP_BACKGROUNDBOTTOM*/, 0, &rcClient, NULL);
            CloseThemeData(hTheme);
        } else {
            FillRect(hdc, &rcClient, (HBRUSH)GetStockObject(BLACK_BRUSH));
        }
        return 0;
    }
    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam;
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        HTHEME hTheme = OpenThemeData(hwnd, L"Taskbar");
        if (hTheme) {
            DrawThemeBackground(hTheme, hdc, 1 /*TBP_BACKGROUNDBOTTOM*/, 0, &rcClient, NULL);
            CloseThemeData(hTheme);
        } else {
            FillRect(hdc, &rcClient, (HBRUSH)GetStockObject(BLACK_BRUSH));
        }
        return 1;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        
        // Draw Taskbar Background using System Theme (UXTheme)
        HTHEME hTheme = OpenThemeData(hwnd, L"Taskbar");
        if (hTheme) {
            DrawThemeBackground(hTheme, hdc, 1 /*TBP_BACKGROUNDBOTTOM*/, 0, &rcClient, NULL);
        } else {
            HBRUSH hbr = CreateSolidBrush(RGB(40, 40, 40));
            FillRect(hdc, &rcClient, hbr);
            DeleteObject(hbr);
        }

        // Draw Task Buttons (Vista Style Wide Labels + Small Icons)
        int btnWidth = 160;
        int btnHeight = rcClient.bottom - rcClient.top;
        int startX = 60; // Offset after Start button
        
        HFONT hFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
                                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 
                                DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));

        for (const auto& btn : g_TaskButtons) {
            RECT rcBtn = { startX, 0, startX + btnWidth, btnHeight };
            
            // Draw Button Background
            if (hTheme) {
                int state = btn.isActive ? 3 /*TS_PRESSED*/ : 1 /*TS_NORMAL*/;
                DrawThemeBackground(hTheme, hdc, 3 /*TBP_BUTTON*/, state, &rcBtn, NULL);
            } else {
                if (btn.isActive) {
                    HBRUSH hbrActive = CreateSolidBrush(RGB(80, 80, 80));
                    FillRect(hdc, &rcBtn, hbrActive);
                    DeleteObject(hbrActive);
                    DrawEdge(hdc, &rcBtn, EDGE_SUNKEN, BF_RECT);
                } else {
                    DrawEdge(hdc, &rcBtn, EDGE_RAISED, BF_RECT);
                }
            }

            // Draw Icon
            if (btn.hIcon) {
                DrawIconEx(hdc, startX + 4, (btnHeight - 16) / 2, btn.hIcon, 16, 16, 0, NULL, DI_NORMAL);
            }

            // Draw Text
            RECT rcText = rcBtn;
            rcText.left += 24; // padding for icon
            rcText.right -= 4; // padding on right
            DrawTextW(hdc, btn.title.c_str(), -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

            startX += btnWidth + 2;
        }

        if (hTheme) CloseThemeData(hTheme);
        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);

        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
            case IDM_TASKBAR_CASCADE:
                if (g_hNativeTaskbar) PostMessageW(g_hNativeTaskbar, WM_COMMAND, 403, 0); // Cascade
                else CascadeWindows(NULL, MDITILE_SKIPDISABLED | MDITILE_ZORDER, NULL, 0, NULL);
                break;
            case IDM_TASKBAR_STACKED:
                if (g_hNativeTaskbar) PostMessageW(g_hNativeTaskbar, WM_COMMAND, 404, 0); // Tile Horizontally
                else TileWindows(NULL, MDITILE_HORIZONTAL, NULL, 0, NULL);
                break;
            case IDM_TASKBAR_SIDEBYSIDE:
                if (g_hNativeTaskbar) PostMessageW(g_hNativeTaskbar, WM_COMMAND, 405, 0); // Tile Vertically
                else TileWindows(NULL, MDITILE_VERTICAL, NULL, 0, NULL);
                break;
            case IDM_TASKBAR_SHOWDESKTOP:
                if (g_hNativeTaskbar) PostMessageW(g_hNativeTaskbar, WM_COMMAND, 407, 0); // ID_SHELL_CMD_SHOW_DESKTOP
                else {
                    // Win+D fails if Explorer isn't running (PE Environment). Use native ToggleDesktop fallback.
                    static std::vector<HWND> s_minimizedWindows;
                    if (s_minimizedWindows.empty()) {
                        EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
                            if (IsWindowVisible(hwnd) && !GetWindow(hwnd, GW_OWNER)) {
                                LONG exStyle = GetWindowLongW(hwnd, GWL_EXSTYLE);
                                if (!(exStyle & WS_EX_TOOLWINDOW)) {
                                    ShowWindowAsync(hwnd, SW_MINIMIZE);
                                    ((std::vector<HWND>*)lParam)->push_back(hwnd);
                                }
                            }
                            return TRUE;
                        }, (LPARAM)&s_minimizedWindows);
                    } else {
                        for (HWND h : s_minimizedWindows) {
                            ShowWindowAsync(h, SW_RESTORE);
                        }
                        s_minimizedWindows.clear();
                    }
                }
                break;
            case IDM_TASKBAR_TASKMGR:
                if (g_hNativeTaskbar) PostMessageW(g_hNativeTaskbar, WM_COMMAND, 420, 0); // 420 is Win10 TaskMgr, 416 on Win7
                else ShellExecuteW(hwnd, L"open", L"taskmgr.exe", NULL, NULL, SW_SHOWNORMAL);
                break;
            case IDM_TASKBAR_LOCK:
                if (g_hNativeTaskbar) PostMessageW(g_hNativeTaskbar, WM_COMMAND, 424, 0); // 424 is ID_LOCKTASKBAR
                else {
                    HKEY hKey;
                    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS) {
                        DWORD dwValue = 0;
                        DWORD cbData = sizeof(DWORD);
                        if (RegQueryValueExW(hKey, L"TaskbarSizeMove", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                            DWORD newValue = (dwValue == 1) ? 0 : 1;
                            RegSetValueExW(hKey, L"TaskbarSizeMove", 0, REG_DWORD, (const BYTE*)&newValue, sizeof(DWORD));
                            SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"TraySettings", SMTO_ABORTIFHUNG, 5000, NULL);
                        }
                        RegCloseKey(hKey);
                    }
                }
                break;
            case IDM_TASKBAR_RUN:
                if (g_hNativeTaskbar) PostMessageW(g_hNativeTaskbar, WM_COMMAND, 401, 0); // Native 'Run...' (401 on modern Windows)
                else InvokeNativeRunDialog(hwnd); // Native Run Dialog undocumented ordinal 61
                break;
            case IDM_TASKBAR_PROPERTIES:
                if (g_hNativeTaskbar) PostMessageW(g_hNativeTaskbar, WM_COMMAND, 413, 0); // 413 is ID_SHELL_CMD_PROPERTIES
                else ShellExecuteW(hwnd, L"open", L"rundll32.exe", L"shell32.dll,Options_RunDLL 1", NULL, SW_SHOWNORMAL);
                break;
            case IDM_TASKBAR_SETTINGS:
                ShowTaskbarProperties(hwnd);
                break;
            case IDM_EXIT_ELITETASKBAR:
                SendMessageW(hwnd, WM_CLOSE, 0, 0);
                break;
            case IDM_START_EXPLORER:
                ShellExecuteW(NULL, L"open", L"explorer.exe", L"shell:::{20D04FE0-3AEA-1069-A2D8-08002B30309D}", NULL, SW_SHOWNORMAL); // Opens "This PC"
                break;
            case IDM_RESTART_SHELL:
                // Restart Explorer Shell
                ShellExecuteW(NULL, NULL, L"cmd.exe", L"/c taskkill /f /im explorer.exe & start explorer.exe", NULL, SW_HIDE);
                break;
        }
        return 0;
    }
    case WM_LBUTTONDOWN: {
        int xPos = GET_X_LPARAM(lParam);
        if (xPos > 60) {
            int btnWidth = 160;
            int startX = 60;
            int btnIndex = (xPos - startX) / (btnWidth + 2);
            if (btnIndex >= 0 && btnIndex < g_TaskButtons.size()) {
                HWND targetHwnd = g_TaskButtons[btnIndex].hwnd;
                if (IsIconic(targetHwnd)) {
                    ShowWindowAsync(targetHwnd, SW_RESTORE);
                } else if (g_TaskButtons[btnIndex].isActive) {
                    ShowWindowAsync(targetHwnd, SW_MINIMIZE);
                } else {
                    SetForegroundWindow(targetHwnd);
                }
            }
        }
        return 0;
    }
    case WM_RBUTTONUP: {
        int xPos = GET_X_LPARAM(lParam);
        POINT pt;
        GetCursorPos(&pt);

        HMENU hMenu = CreatePopupMenu();
        if (xPos <= 60) {
            // Start Button Context Menu
            AppendMenuW(hMenu, MF_STRING, IDM_START_EXPLORER, L"Open Windows Explorer");
            AppendMenuW(hMenu, MF_STRING, IDM_TASKBAR_PROPERTIES, L"Properties");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hMenu, MF_STRING, IDM_EXIT_ELITETASKBAR, L"Exit Elite Taskbar");
        } else {
            // Taskbar Context Menu
            HMENU hToolbars = CreatePopupMenu();
            AppendMenuW(hToolbars, MF_STRING | MF_UNCHECKED, 3020, L"Address");
            AppendMenuW(hToolbars, MF_STRING | MF_UNCHECKED, 3021, L"Links");
            AppendMenuW(hToolbars, MF_STRING | MF_UNCHECKED, 3022, L"Desktop");
            AppendMenuW(hToolbars, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hToolbars, MF_STRING, 3023, L"New toolbar...");
            AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hToolbars, L"Toolbars");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hMenu, MF_STRING, IDM_TASKBAR_CASCADE, L"Cascade windows");
            AppendMenuW(hMenu, MF_STRING, IDM_TASKBAR_STACKED, L"Show windows stacked");
            AppendMenuW(hMenu, MF_STRING, IDM_TASKBAR_SIDEBYSIDE, L"Show windows side by side");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hMenu, MF_STRING, IDM_TASKBAR_SHOWDESKTOP, L"Show the desktop");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hMenu, MF_STRING, IDM_TASKBAR_TASKMGR, L"Task Manager");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
            
            bool isLocked = false;
            HKEY hKey;
            if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                DWORD dwValue = 0;
                DWORD cbData = sizeof(DWORD);
                if (RegQueryValueExW(hKey, L"TaskbarSizeMove", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                    isLocked = (dwValue == 0);
                }
                RegCloseKey(hKey);
            }
            
            AppendMenuW(hMenu, MF_STRING | (isLocked ? MF_CHECKED : MF_UNCHECKED), IDM_TASKBAR_LOCK, L"Lock the taskbar");
            AppendMenuW(hMenu, MF_STRING, IDM_TASKBAR_RUN, L"Run...");
            AppendMenuW(hMenu, MF_STRING, IDM_TASKBAR_SETTINGS, L"Elite Taskbar Settings");
            AppendMenuW(hMenu, MF_STRING, IDM_TASKBAR_PROPERTIES, L"Properties");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
            if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
                AppendMenuW(hMenu, MF_STRING, IDM_RESTART_SHELL, L"Restart Shell");
                AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
            }
            AppendMenuW(hMenu, MF_STRING, IDM_EXIT_ELITETASKBAR, L"Exit Elite Taskbar");
        }

        SetForegroundWindow(hwnd);
        int cmd = TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, hwnd, NULL);
        if (cmd != 0) {
            PostMessageW(hwnd, WM_COMMAND, MAKEWPARAM(cmd, 0), 0);
        }
        DestroyMenu(hMenu);
        return 0;
    }
    case WM_TIMER:
        if (wParam == 9999 && g_Config.Mode == TaskbarMode::Replace && g_hNativeTaskbar) {
            if (IsWindowVisible(g_hNativeTaskbar)) {
                ShowWindow(g_hNativeTaskbar, SW_HIDE);
                SetWindowPos(g_hNativeTaskbar, NULL, -10000, -10000, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
            }
        }
        return 0;
    case WM_SETTINGCHANGE: {
        if (lParam && wcscmp((LPCWSTR)lParam, L"TraySettings") == 0) {
            HKEY hKey;
            if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                DWORD dwValue = 0;
                DWORD cbData = sizeof(DWORD);
                if (RegQueryValueExW(hKey, L"TaskbarMode", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                    bool newIsReplace = (dwValue == 1);
                    bool oldIsReplace = (g_Config.Mode == TaskbarMode::Replace);
                    if (newIsReplace != oldIsReplace) {
                        PostMessageW(hwnd, WM_COMMAND, IDM_RESTART_SHELL, 0);
                    }
                }
                RegCloseKey(hKey);
            }
            InvalidateRect(hwnd, NULL, TRUE);
        }
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

struct MonitorEnumData {
    std::vector<HMONITOR> monitors;
    std::vector<RECT> rects;
    std::vector<bool> isPrimary;
};

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    MonitorEnumData* data = (MonitorEnumData*)dwData;
    MONITORINFO mi = {0};
    mi.cbSize = sizeof(MONITORINFO);
    if (GetMonitorInfoW(hMonitor, &mi)) {
        data->monitors.push_back(hMonitor);
        data->rects.push_back(mi.rcMonitor);
        data->isPrimary.push_back((mi.dwFlags & MONITORINFOF_PRIMARY) != 0);
    }
    return TRUE;
}

bool TaskbarWindow::Initialize(HINSTANCE hInstance) {
    g_uTaskbarCreatedMsg = RegisterWindowMessageW(L"TaskbarCreated");
    g_uShellHookMsg = RegisterWindowMessageW(L"SHELLHOOK");

    int taskbarHeight = 40; // Default fallback
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD dwValue = 0;
        DWORD cbData = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"TaskbarSmallIcons", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
            taskbarHeight = (dwValue == 1) ? 30 : 40;
        }
        RegCloseKey(hKey);
    }

    g_hNativeTaskbar = FindWindowW(L"Shell_TrayWnd", NULL);
    if (g_hNativeTaskbar) {
        RECT nativeRect = {0};
        GetWindowRect(g_hNativeTaskbar, &nativeRect);
        int nativeHeight = nativeRect.bottom - nativeRect.top;
        if (nativeHeight > 0 && nativeHeight < 100) {
            taskbarHeight = nativeHeight;
        }
        if (g_Config.Mode == TaskbarMode::Replace) {
            ShowWindow(g_hNativeTaskbar, SW_HIDE);
            SetWindowPos(g_hNativeTaskbar, NULL, -10000, -10000, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
        }
    }

    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = (g_Config.Mode == TaskbarMode::Replace) ? CLASS_NAME : TRAY_CLASS_NAME;
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_MAIN_PROGRAM));
    wc.hIconSm = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_MAIN_PROGRAM));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    
    if (!RegisterClassExW(&wc)) {
        Logger::Log(L"Failed to register taskbar window class.");
        return false;
    }

    WNDCLASSEXW wcChild = {0};
    wcChild.cbSize = sizeof(WNDCLASSEXW);
    wcChild.hInstance = hInstance;
    wcChild.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcChild.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    
    wcChild.lpfnWndProc = TrayNotifyProc;
    wcChild.lpszClassName = L"TrayNotifyWnd";
    RegisterClassExW(&wcChild);

    wcChild.style = CS_DBLCLKS;
    wcChild.lpfnWndProc = TrayClockProc;
    wcChild.lpszClassName = L"TrayClockWClass";
    RegisterClassExW(&wcChild);

    wcChild.style = 0;
    wcChild.lpfnWndProc = TrayShowDesktopButtonProc;
    wcChild.lpszClassName = L"TrayShowDesktopButtonWClass";
    RegisterClassExW(&wcChild);

    MonitorEnumData monData;
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)&monData);
    
    bool bHookRegistered = false;
    for (size_t i = 0; i < monData.monitors.size(); i++) {
        if (g_Config.Mode == TaskbarMode::SecondaryOnly && monData.isPrimary[i]) {
            continue; // Skip primary monitor in SecondaryOnly mode
        }
        
        TaskbarInstance* inst = new TaskbarInstance();
        inst->hMonitor = monData.monitors[i];
        inst->monitorRect = monData.rects[i];
        inst->taskbarHeight = taskbarHeight;
        
        int screenWidth = inst->monitorRect.right - inst->monitorRect.left;
        int screenHeight = inst->monitorRect.bottom - inst->monitorRect.top;
        int xPos = inst->monitorRect.left;
        int yPos = inst->monitorRect.bottom - taskbarHeight;

        inst->hTaskbar = CreateWindowExW(
            WS_EX_TOOLWINDOW,
            (g_Config.Mode == TaskbarMode::Replace) ? CLASS_NAME : TRAY_CLASS_NAME, L"", WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
            xPos, yPos, screenWidth, taskbarHeight,
            NULL, NULL, hInstance, NULL
        );
        
        if (!inst->hTaskbar) {
            delete inst;
            continue;
        }

        if (!bHookRegistered) {
            RegisterShellHookWindow(inst->hTaskbar);
            bHookRegistered = true;
        }

        inst->hReBar = CreateWindowExW(0, L"ReBarWindow32", L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | RBS_VARHEIGHT | RBS_BANDBORDERS, 
            45, 0, screenWidth - 280, taskbarHeight, inst->hTaskbar, NULL, hInstance, NULL);

        inst->hTrayNotify = CreateWindowExW(0, L"TrayNotifyWnd", L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, screenWidth - 255, 0, 240, taskbarHeight, inst->hTaskbar, NULL, hInstance, NULL);
        inst->hSysPager = CreateWindowExW(0, L"SysPager", L"", WS_CHILD, 0, 0, 100, taskbarHeight, inst->hTrayNotify, NULL, hInstance, NULL);
        inst->hToolbar = CreateWindowExW(0, L"ToolbarWindow32", L"", WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0, 100, taskbarHeight, inst->hSysPager, NULL, hInstance, NULL);
        inst->hTrayClock = CreateWindowExW(0, L"TrayClockWClass", L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, 100, 0, 140, taskbarHeight, inst->hTrayNotify, NULL, hInstance, NULL);
        
        CreateWindowExW(0, L"TrayShowDesktopButtonWClass", L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, screenWidth - 15, 0, 15, taskbarHeight, inst->hTaskbar, NULL, hInstance, NULL);

        SetWindowPos(inst->hTaskbar, HWND_TOPMOST, xPos, yPos, screenWidth, taskbarHeight, SWP_SHOWWINDOW);

        if (g_Config.Mode == TaskbarMode::Replace && monData.isPrimary[i]) {
            RegisterHotKey(inst->hTaskbar, 1, MOD_WIN | MOD_NOREPEAT, 'E');
            RegisterHotKey(inst->hTaskbar, 2, MOD_WIN | MOD_NOREPEAT, 'R');
            RegisterHotKey(inst->hTaskbar, 3, MOD_WIN | MOD_NOREPEAT, 'D');
        }

        if (monData.isPrimary[i] || g_Config.Mode == TaskbarMode::Replace) {
            APPBARDATA abd = {0};
            abd.cbSize = sizeof(APPBARDATA);
            abd.hWnd = inst->hTaskbar;
            abd.uEdge = ABE_BOTTOM;
            abd.rc.left = xPos;
            abd.rc.right = xPos + screenWidth;
            abd.rc.top = yPos;
            abd.rc.bottom = yPos + taskbarHeight;
            SHAppBarMessage(ABM_NEW, &abd);
            SHAppBarMessage(ABM_QUERYPOS, &abd);
            SHAppBarMessage(ABM_SETPOS, &abd);
        }

        ShowWindow(inst->hTaskbar, SW_SHOW);
        UpdateWindow(inst->hTaskbar);

        inst->startButton = new StartButton();
        if (inst->startButton->Initialize(hInstance, inst->hTaskbar)) {
            inst->startButton->SetOrbImageFromResource(hInstance, IDB_START_ORB);
            inst->startButton->Show(xPos, yPos, taskbarHeight);
        }
        
        g_Taskbars.push_back(inst);
    }

    if (g_Taskbars.empty()) {
        Logger::Log(L"No taskbars were created. Exiting.");
        return false;
    }

    if (g_uTaskbarCreatedMsg != 0) {
        PostMessageW(HWND_BROADCAST, g_uTaskbarCreatedMsg, 0, 0);
    }

    return true;
}

void TaskbarWindow::RunMessageLoop() {
    MSG msg = {0};
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void TaskbarWindow::Cleanup() {
    for (auto* inst : g_Taskbars) {
        APPBARDATA abd = {0};
        abd.cbSize = sizeof(APPBARDATA);
        abd.hWnd = inst->hTaskbar;
        SHAppBarMessage(ABM_REMOVE, &abd);
        if (inst->startButton) {
            delete inst->startButton;
        }
        DestroyWindow(inst->hTaskbar);
        delete inst;
    }
    g_Taskbars.clear();
    
    if (g_hNativeTaskbar && IsWindow(g_hNativeTaskbar)) {
        ShowWindow(g_hNativeTaskbar, SW_SHOW);
        PostMessageW(g_hNativeTaskbar, WM_DISPLAYCHANGE, 0, 0);
    } else {
        ShellExecuteW(NULL, L"open", L"explorer.exe", NULL, NULL, SW_SHOWNORMAL);
    }
}
