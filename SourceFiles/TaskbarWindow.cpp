#include "TaskbarWindow.h"
#include "StartButton.h"
#include "ClockWidget.h"
#include "Logger.h"
#include "resource.h"
#include <dwmapi.h>
#include <windowsx.h>
#include <uxtheme.h>
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

BOOL ShowLegacyClockExperience(HWND hWnd) {
    if (!hWnd) return FALSE;
    HRESULT hr = S_OK;
    Win32Clock* pWin32Clock = NULL;
    hr = CoCreateInstance(
        GUID_Win32Clock,
        NULL,
        CLSCTX_ALL,
        IID_Win32Clock,
        (void**)&pWin32Clock
    );
    if (SUCCEEDED(hr)) {
        RECT rc;
        GetWindowRect(hWnd, &rc);
        pWin32Clock->lpVtbl->ShowWin32Clock(pWin32Clock, hWnd, &rc);
        pWin32Clock->lpVtbl->Release(pWin32Clock);
        return TRUE;
    } else {
        wchar_t buf[256];
        swprintf_s(buf, L"CoCreateInstance for Win32Clock failed with HRESULT: 0x%08X", hr);
        Logger::Log(buf);
    }
    return FALSE;
}

HWND g_hTaskbar = NULL;
HWND g_hNativeTaskbar = NULL;
HWND g_hTrayNotify = NULL;
HWND g_hTrayClock = NULL;
HWND g_hSysPager = NULL;
HWND g_hReBar = NULL;
HWND g_hToolbar = NULL;

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
#define IDM_START_EXPLORER          3009
#define IDM_EXIT_ELITETASKBAR       3010
#define IDM_RESTART_SHELL           3011

UINT g_uTaskbarCreatedMsg = 0;
OrbState g_orbState = OrbState::Normal;
bool g_bOrbTrackingMouse = false;

#include <vector>

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
        
        if (hBufferedPaint) {
            DrawThemeParentBackground(hwnd, hdcBuffer, &rcClient);
            
            HTHEME hTheme = OpenThemeData(hwnd, L"Taskbar");
            if (hTheme) {
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
                HFONT hFont = CreateFontIndirectW(&lf);
                HFONT hOldFont = (HFONT)SelectObject(hdcBuffer, hFont);

                DTTOPTS dttOpts = { sizeof(DTTOPTS) };
                dttOpts.dwFlags = DTT_COMPOSITED | DTT_TEXTCOLOR;
                dttOpts.crText = RGB(255, 255, 255);
                
                rcClient.right -= 12; // Pad from the right edge
                DrawThemeTextEx(hTheme, hdcBuffer, 0, 0, clockText, -1, DT_RIGHT | DT_VCENTER, &rcClient, &dttOpts);
                
                SelectObject(hdcBuffer, hOldFont);
                DeleteObject(hFont);
                CloseThemeData(hTheme);
            }
            EndBufferedPaint(hBufferedPaint, TRUE);
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN: {
        if (!FindWindowW(L"ClockFlyoutWindow", NULL)) {
            if (!ShowLegacyClockExperience(hwnd)) {
                // Fallback to control panel if COM interface fails (e.g., pre-Win10 or corrupted)
                ShellExecuteW(NULL, L"open", L"control", L"timedate.cpl", NULL, SW_SHOWNORMAL);
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
        APPBARDATA abd = {0};
        abd.cbSize = sizeof(APPBARDATA);
        abd.hWnd = g_hTaskbar;
        SHAppBarMessage(ABM_NEW, &abd);
        return 0;
    }

    switch (uMsg) {
    case WM_CREATE: {
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
            CloseThemeData(hTheme);
        } else {
            // Fallback: Fill with black (Glass handles alpha)
            FillRect(hdc, &rcClient, (HBRUSH)GetStockObject(BLACK_BRUSH));
        }

        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
            case IDM_TASKBAR_TASKMGR:
                ShellExecuteW(NULL, L"open", L"taskmgr.exe", NULL, NULL, SW_SHOWNORMAL);
                break;
            case IDM_TASKBAR_LOCK: {
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
                break;
            }
            case IDM_TASKBAR_PROPERTIES:
                ShellExecuteW(NULL, L"open", L"rundll32.exe", L"shell32.dll,Options_RunDLL 1", NULL, SW_SHOWNORMAL);
                break;
            case IDM_EXIT_ELITETASKBAR:
                SendMessageW(hwnd, WM_CLOSE, 0, 0);
                break;
            case IDM_START_EXPLORER:
                ShellExecuteW(NULL, L"open", L"explorer.exe", NULL, NULL, SW_SHOWNORMAL);
                break;
            case IDM_RESTART_SHELL:
                // Restart Explorer Shell
                ShellExecuteW(NULL, L"open", L"cmd.exe", L"/c taskkill /f /im explorer.exe & start explorer.exe", NULL, SW_HIDE);
                break;
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
            
            bool isLocked = true; // Default
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
            SendMessageW(hwnd, WM_COMMAND, MAKEWPARAM(cmd, 0), 0);
        }
        DestroyMenu(hMenu);
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    MONITORINFO mi = {0};
    mi.cbSize = sizeof(MONITORINFO);
    if (GetMonitorInfoW(hMonitor, &mi)) {
        if (mi.dwFlags & MONITORINFOF_PRIMARY) {
            RECT* primaryRect = (RECT*)dwData;
            *primaryRect = mi.rcMonitor;
        }
    }
    return TRUE;
}

bool TaskbarWindow::Initialize(HINSTANCE hInstance) {
    g_uTaskbarCreatedMsg = RegisterWindowMessageW(L"TaskbarCreated");

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

    // Hide native taskbar if in Replace mode and grab its height to respect user settings (Small/Large)
    g_hNativeTaskbar = FindWindowW(L"Shell_TrayWnd", NULL);
    if (g_hNativeTaskbar) {
        RECT nativeRect = {0};
        GetWindowRect(g_hNativeTaskbar, &nativeRect);
        int nativeHeight = nativeRect.bottom - nativeRect.top;
        if (nativeHeight > 0 && nativeHeight < 100) {
            taskbarHeight = nativeHeight;
        }
        ShowWindow(g_hNativeTaskbar, SW_HIDE);
    }

    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_MAIN_PROGRAM));
    wc.hIconSm = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_MAIN_PROGRAM));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    
    if (!RegisterClassExW(&wc)) {
        Logger::Log(L"Failed to register Shell_TrayWnd.");
        return false;
    }

    // Register child classes
    WNDCLASSEXW wcChild = {0};
    wcChild.cbSize = sizeof(WNDCLASSEXW);
    wcChild.hInstance = hInstance;
    wcChild.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcChild.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH); // Black brush is transparent in DWM Glass
    
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

    RECT primaryRect = {0};
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)&primaryRect);
    
    int screenWidth = primaryRect.right - primaryRect.left;
    int screenHeight = primaryRect.bottom - primaryRect.top;
    int xPos = primaryRect.left;
    int yPos = primaryRect.bottom - taskbarHeight;

    g_hTaskbar = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        CLASS_NAME,
        L"", // Empty title for taskbar
        WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        xPos, yPos, screenWidth, taskbarHeight,
        NULL, NULL, hInstance, NULL
    );

    if (!g_hTaskbar) {
        Logger::Log(L"Failed to create window HWND.");
        return false;
    }
    
    // Create children (Start Button removed, rendered directly on parent DC)
    g_hReBar = CreateWindowExW(0, L"ReBarWindow32", L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | RBS_VARHEIGHT | RBS_BANDBORDERS, 
        45, 0, screenWidth - 325, taskbarHeight, g_hTaskbar, NULL, hInstance, NULL);

    // Tray area (Moved left to give clock plenty of room and padding)
    g_hTrayNotify = CreateWindowExW(0, L"TrayNotifyWnd", L"", WS_CHILD | WS_VISIBLE, screenWidth - 280, 0, 265, taskbarHeight, g_hTaskbar, NULL, hInstance, NULL);
    
    // Hide SysPager and ToolbarWindow32 until Phase 6 when we implement custom rendering
    g_hSysPager = CreateWindowExW(0, L"SysPager", L"", WS_CHILD, 0, 0, 125, taskbarHeight, g_hTrayNotify, NULL, hInstance, NULL);
    g_hToolbar = CreateWindowExW(0, L"ToolbarWindow32", L"", WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0, 125, taskbarHeight, g_hSysPager, NULL, hInstance, NULL);
    
    // Clock widget placed with wide bounds to fit any date format
    g_hTrayClock = CreateWindowExW(0, L"TrayClockWClass", L"", WS_CHILD | WS_VISIBLE, 125, 0, 140, taskbarHeight, g_hTrayNotify, NULL, hInstance, NULL);
    
    // Show Desktop button at far right
    CreateWindowExW(0, L"TrayShowDesktopButtonWClass", L"", WS_CHILD | WS_VISIBLE, screenWidth - 15, 0, 15, taskbarHeight, g_hTaskbar, NULL, hInstance, NULL);

    // Apply DWM Aero Glass Effect
    MARGINS margins = { -1, -1, -1, -1 };
    DwmExtendFrameIntoClientArea(g_hTaskbar, &margins);

    SetLayeredWindowAttributes(g_hTaskbar, 0, 255, LWA_ALPHA);
    SetWindowPos(g_hTaskbar, HWND_TOPMOST, xPos, yPos, screenWidth, taskbarHeight, SWP_SHOWWINDOW);

    APPBARDATA abd = {0};
    abd.cbSize = sizeof(APPBARDATA);
    abd.hWnd = g_hTaskbar;
    abd.uEdge = ABE_BOTTOM;
    abd.rc.left = xPos;
    abd.rc.right = xPos + screenWidth;
    abd.rc.top = yPos;
    abd.rc.bottom = yPos + taskbarHeight;

    SHAppBarMessage(ABM_NEW, &abd);
    SHAppBarMessage(ABM_QUERYPOS, &abd);
    SHAppBarMessage(ABM_SETPOS, &abd);

    ShowWindow(g_hTaskbar, SW_SHOW);
    UpdateWindow(g_hTaskbar);

    // Initialize the floating layered Start Button
    StartButton::Initialize(hInstance, g_hTaskbar);
    StartButton::SetOrbImageFromResource(hInstance, IDB_START_ORB);
    StartButton::Show(xPos, yPos, taskbarHeight);

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
    if (g_hTaskbar) {
        APPBARDATA abd = {0};
        abd.cbSize = sizeof(APPBARDATA);
        abd.hWnd = g_hTaskbar;
        SHAppBarMessage(ABM_REMOVE, &abd);
    }
    
    if (g_hNativeTaskbar && IsWindow(g_hNativeTaskbar)) {
        ShowWindow(g_hNativeTaskbar, SW_SHOW);
    } else {
        ShellExecuteW(NULL, L"open", L"explorer.exe", NULL, NULL, SW_SHOWNORMAL);
    }
}
