#include "stdafx.h"
#pragma warning(disable: 4100 4244 4267 4189 4312)
#include <windows.h>
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
#include <shellapi.h>
#include <shobjidl.h>
#include <propkey.h>
#include "TrayIconScraper.h"
#include <commctrl.h>

static void InvokeNativeRunDialog(HWND hwndOwner);

extern EliteTaskbarConfig g_Config;

#include <initguid.h>
#include <Unknwn.h>
#include <objbase.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "propsys.lib")
#pragma comment(lib, "shlwapi.lib")

#include <shobjidl.h>
#include <propkey.h>
#include <propvarutil.h>
#include <shlwapi.h>

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
        if (tb->hTaskbar == hwnd || tb->hTrayNotify == hwnd || tb->hTrayClock == hwnd || tb->hSysPager == hwnd || tb->hToolbar == hwnd) {
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
#define SEC_CLASS_NAME L"Shell_SecondaryTrayWnd"

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
#define IDM_EXIT_ALL_ELITETASKBAR   3014

UINT g_uTaskbarCreatedMsg = 0;
OrbState g_orbState = OrbState::Normal;
bool g_bOrbTrackingMouse = false;

#include <vector>
#include <dwmapi.h>

struct TaskButtonInfo {
    HWND hwnd;
    HICON hIcon;
    std::wstring title;
    bool isActive;
    int cmdId;
};

HWND g_hPreviewWindow = NULL;
HTHUMBNAIL g_hPreviewThumb = NULL;
UINT_PTR g_PreviewTimer = 0;
HWND g_PreviewTargetHwnd = NULL;

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

LRESULT CALLBACK PreviewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            MARGINS margins = { -1, -1, -1, -1 };
            DwmExtendFrameIntoClientArea(hwnd, &margins);
            return 0;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            RECT rcClient;
            GetClientRect(hwnd, &rcClient);
            
            // Draw glass background (transparent black so DWM picks it up)
            HBRUSH hbrBlack = CreateSolidBrush(RGB(0, 0, 0));
            FillRect(hdc, &rcClient, hbrBlack);
            DeleteObject(hbrBlack);
            
            // Get window title
            WCHAR szTitle[256] = {0};
            HWND targetHwnd = (HWND)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
            if (IsWindow(targetHwnd)) {
                GetWindowTextW(targetHwnd, szTitle, 256);
            }
            
            // Draw text using DrawThemeTextEx for Aero Glow
            HTHEME hTheme = OpenThemeData(hwnd, L"WINDOW");
            if (hTheme) {
                RECT rcText = rcClient;
                rcText.top += 5;
                rcText.left += 5;
                rcText.right -= 20; // Space for close button
                rcText.bottom = rcText.top + 20;
                
                DTTOPTS dttOpts = {sizeof(DTTOPTS)};
                dttOpts.dwFlags = DTT_GLOWSIZE | DTT_COMPOSITED | DTT_TEXTCOLOR;
                dttOpts.iGlowSize = 8;
                dttOpts.crText = RGB(255, 255, 255);
                
                LOGFONTW lf = {0};
                lf.lfHeight = -12;
                lf.lfWeight = FW_NORMAL;
                wcscpy_s(lf.lfFaceName, L"Segoe UI");
                HFONT hFont = CreateFontIndirectW(&lf);
                HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
                
                DrawThemeTextEx(hTheme, hdc, 0, 0, szTitle, -1, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS, &rcText, &dttOpts);
                
                SelectObject(hdc, hOldFont);
                DeleteObject(hFont);
                CloseThemeData(hTheme);
            }
            
            // Draw close button border (Scaled down to avoid clipping)
            RECT rcClose = { rcClient.right - 18, 6, rcClient.right - 6, 18 };
            DrawFrameControl(hdc, &rcClose, DFC_CAPTION, DFCS_CAPTIONCLOSE | DFCS_FLAT);

            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_LBUTTONUP: {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            RECT rcClient;
            GetClientRect(hwnd, &rcClient);
            RECT rcClose = { rcClient.right - 18, 6, rcClient.right - 6, 18 };
            
            HWND targetHwnd = (HWND)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
            if (PtInRect(&rcClose, pt)) {
                if (IsWindow(targetHwnd)) {
                    PostMessageW(targetHwnd, WM_CLOSE, 0, 0);
                }
            } else {
                if (IsWindow(targetHwnd)) {
                    SetForegroundWindow(targetHwnd);
                    if (IsIconic(targetHwnd)) {
                        ShowWindowAsync(targetHwnd, SW_RESTORE);
                    }
                }
            }
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        }
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

#include <list>
std::list<TaskButtonInfo> g_TaskButtons;
int g_NextCmdId = 4000;

extern std::vector<TaskbarInstance*> g_Taskbars;

LRESULT CALLBACK TaskSwitchSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    static int s_dragIndex = -1;
    static BOOL s_dragging = FALSE;
    static POINT s_ptDragStart;
    
    switch (uMsg) {
        case WM_LBUTTONDOWN: {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            int idx = SendMessageW(hWnd, TB_HITTEST, 0, (LPARAM)&pt);
            if (idx >= 0) {
                s_dragIndex = idx;
                s_dragging = FALSE;
                s_ptDragStart = pt;
            }
            break;
        }
        case WM_MOUSEMOVE: {
            if (s_dragIndex >= 0 && (wParam & MK_LBUTTON)) {
                POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                if (!s_dragging) {
                    if (abs(pt.x - s_ptDragStart.x) > 3 || abs(pt.y - s_ptDragStart.y) > 3) {
                        s_dragging = TRUE;
                        SetCapture(hWnd);
                    }
                }
                
                if (s_dragging) {
                    int currentHover = SendMessageW(hWnd, TB_HITTEST, 0, (LPARAM)&pt);
                    if (currentHover >= 0 && currentHover != s_dragIndex) {
                        SendMessageW(hWnd, TB_MOVEBUTTON, s_dragIndex, currentHover);
                        s_dragIndex = currentHover;
                    }
                }
            }
            break;
        }
        case WM_LBUTTONUP: {
            if (s_dragging) {
                ReleaseCapture();
                s_dragging = FALSE;
                s_dragIndex = -1;
                return 0; // Consume the click so it doesn't activate the window!
            }
            if (s_dragIndex >= 0) {
                TBBUTTON tbb = {0};
                SendMessageW(hWnd, TB_GETBUTTON, s_dragIndex, (LPARAM)&tbb);
                PostMessageW(GetParent(hWnd), WM_COMMAND, MAKEWPARAM(tbb.idCommand, 0), (LPARAM)hWnd);
            }
            s_dragIndex = -1;
            break;
        }
        case WM_RBUTTONUP: {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            int idx = SendMessageW(hWnd, TB_HITTEST, 0, (LPARAM)&pt);
            if (idx >= 0) {
                TBBUTTON tbb = {0};
                SendMessageW(hWnd, TB_GETBUTTON, idx, (LPARAM)&tbb);
                HWND targetHwnd = (HWND)tbb.dwData;
                
                if (targetHwnd && IsWindow(targetHwnd)) {
                    if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
                        HMENU hJumpMenu = CreatePopupMenu();
                        AppendMenuW(hJumpMenu, MF_STRING | MF_GRAYED, 0, L"Jump List (Recent/Frequent)");
                        AppendMenuW(hJumpMenu, MF_SEPARATOR, 0, NULL);
                        
                        IPropertyStore* pps = NULL;
                        PWSTR appID = NULL;
                        if (SUCCEEDED(SHGetPropertyStoreForWindow(targetHwnd, IID_PPV_ARGS(&pps)))) {
                            PROPVARIANT pv;
                            PropVariantInit(&pv);
                            if (SUCCEEDED(pps->GetValue(PKEY_AppUserModel_ID, &pv)) && pv.vt == VT_LPWSTR) {
                                SHStrDupW(pv.pwszVal, &appID);
                            }
                            PropVariantClear(&pv);
                            pps->Release();
                        }
                        
                        if (appID) {
                            IApplicationDocumentLists* padl = NULL;
                            if (SUCCEEDED(CoCreateInstance(CLSID_ApplicationDocumentLists, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&padl)))) {
                                padl->SetAppID(appID);
                                IObjectArray* poaRecent = NULL;
                                if (SUCCEEDED(padl->GetList(ADLT_RECENT, 0, IID_PPV_ARGS(&poaRecent)))) {
                                    UINT count = 0;
                                    poaRecent->GetCount(&count);
                                    for (UINT i = 0; i < count && i < 10; ++i) {
                                        IShellItem* psi = NULL;
                                        if (SUCCEEDED(poaRecent->GetAt(i, IID_PPV_ARGS(&psi)))) {
                                            PWSTR name = NULL;
                                            if (SUCCEEDED(psi->GetDisplayName(SIGDN_NORMALDISPLAY, &name))) {
                                                AppendMenuW(hJumpMenu, MF_STRING, 2000 + i, name);
                                                CoTaskMemFree(name);
                                            }
                                            psi->Release();
                                        }
                                    }
                                    poaRecent->Release();
                                }
                                padl->Release();
                            }
                            CoTaskMemFree(appID);
                        } else {
                            AppendMenuW(hJumpMenu, MF_STRING | MF_GRAYED, 0, L"(No AppUserModelID found)");
                        }
                        
                        ClientToScreen(hWnd, &pt);
                        SetForegroundWindow(hWnd);
                        TrackPopupMenuEx(hJumpMenu, TPM_RIGHTBUTTON, pt.x, pt.y, hWnd, NULL);
                        DestroyMenu(hJumpMenu);
                    } else {
                        HMENU hSysMenu = GetSystemMenu(targetHwnd, FALSE);
                        if (hSysMenu) {
                            ClientToScreen(hWnd, &pt);
                            SetForegroundWindow(hWnd);
                            int cmd = TrackPopupMenuEx(hSysMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, hWnd, NULL);
                            if (cmd > 0) {
                                SendMessageW(targetHwnd, WM_SYSCOMMAND, cmd, 0);
                            }
                        }
                    }
                }
            } else {
                POINT ptObj = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                MapWindowPoints(hWnd, GetParent(hWnd), &ptObj, 1);
                PostMessageW(GetParent(hWnd), uMsg, wParam, MAKELPARAM(ptObj.x, ptObj.y));
            }
            return 0;
        }
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void AddTaskButton(TaskButtonInfo& btn);

void SyncTaskbarButtonsAcrossMonitors() {
    for (auto* inst : g_Taskbars) {
        if (!inst->hTaskSwitch) continue;
        HMONITOR hTaskbarMonitor = MonitorFromWindow(inst->hTaskbar, MONITOR_DEFAULTTONEAREST);
        
        int count = SendMessageW(inst->hTaskSwitch, TB_BUTTONCOUNT, 0, 0);
        for (int i = count - 1; i >= 0; i--) {
            TBBUTTON tbb = {0};
            SendMessageW(inst->hTaskSwitch, TB_GETBUTTON, i, (LPARAM)&tbb);
            HWND hwnd = (HWND)tbb.dwData;
            if (hwnd && IsWindow(hwnd)) {
                HMONITOR hWindowMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
                if (hWindowMonitor != hTaskbarMonitor && hWindowMonitor != NULL) {
                    SendMessageW(inst->hTaskSwitch, TB_DELETEBUTTON, i, 0);
                    
                    TaskButtonInfo info;
                    info.hwnd = hwnd;
                    info.cmdId = tbb.idCommand;
                    info.isActive = (tbb.fsState & TBSTATE_CHECKED) != 0;
                    
                    WCHAR szTitle[256] = {0};
                    GetWindowTextW(hwnd, szTitle, 256);
                    info.title = szTitle;
                    
                    DWORD_PTR dwRes;
                    if (!SendMessageTimeoutW(hwnd, WM_GETICON, ICON_SMALL, 0, SMTO_ABORTIFHUNG, 500, &dwRes)) dwRes = 0;
                    info.hIcon = (HICON)dwRes;
                    if (!info.hIcon) info.hIcon = (HICON)GetClassLongPtrW(hwnd, GCLP_HICONSM);
                    
                    bool found = false;
                    for (auto& tb : g_TaskButtons) {
                        if (tb.hwnd == hwnd) {
                            tb.title = szTitle;
                            tb.hIcon = info.hIcon;
                            tb.isActive = info.isActive;
                            AddTaskButton(tb);
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        g_TaskButtons.push_back(info);
                        AddTaskButton(g_TaskButtons.back());
                    }
                }
            }
        }
        SendMessageW(inst->hTaskSwitch, TB_AUTOSIZE, 0, 0);
    }
}

HMONITOR GetWindowMonitor(HWND hwnd) {
    if (IsIconic(hwnd)) {
        WINDOWPLACEMENT wp = { sizeof(WINDOWPLACEMENT) };
        if (GetWindowPlacement(hwnd, &wp)) {
            HMONITOR hMon = MonitorFromRect(&wp.rcNormalPosition, MONITOR_DEFAULTTONEAREST);
            if (hMon) return hMon;
        }
    }
    return MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
}

void AddTaskButton(TaskButtonInfo& btn) {
    HMONITOR hWindowMonitor = GetWindowMonitor(btn.hwnd);
    
    for (auto* inst : g_Taskbars) {
        if (!inst->hTaskSwitch) continue;
        
        HMONITOR hTaskbarMonitor = MonitorFromWindow(inst->hTaskbar, MONITOR_DEFAULTTONEAREST);
        if (hWindowMonitor != NULL && hWindowMonitor != hTaskbarMonitor) {
            continue;
        }

        int imgIdx = -1;
        if (btn.hIcon) imgIdx = ImageList_AddIcon(inst->hImageList, btn.hIcon);
        
        TBBUTTON tbb = {0};
        tbb.iBitmap = imgIdx;
        tbb.idCommand = btn.cmdId;
        tbb.fsState = TBSTATE_ENABLED;
        if (btn.isActive) tbb.fsState |= TBSTATE_CHECKED;
        
        tbb.fsStyle = BTNS_BUTTON;
        if (g_Config.ButtonWidth == ButtonWidthMode::Auto) {
            tbb.fsStyle |= BTNS_AUTOSIZE | BTNS_SHOWTEXT;
        } else if (g_Config.ButtonWidth == ButtonWidthMode::Fixed) {
            tbb.fsStyle |= BTNS_SHOWTEXT;
        } else if (g_Config.ButtonWidth == ButtonWidthMode::IconsOnly) {
            tbb.fsStyle |= BTNS_NOPREFIX;
        }
        
        tbb.dwData = (DWORD_PTR)btn.hwnd;
        tbb.iString = (INT_PTR)btn.title.c_str();
        
        SendMessageW(inst->hTaskSwitch, TB_ADDBUTTONSW, 1, (LPARAM)&tbb);
        
        TBBUTTONINFOW tbbi = { sizeof(TBBUTTONINFOW) };
        tbbi.dwMask = TBIF_TEXT;
        tbbi.pszText = (LPWSTR)btn.title.c_str();
        SendMessageW(inst->hTaskSwitch, TB_SETBUTTONINFOW, tbb.idCommand, (LPARAM)&tbbi);
        SendMessageW(inst->hTaskSwitch, TB_AUTOSIZE, 0, 0);
    }
}

void RemoveTaskButton(HWND hwnd) {
    for (auto* inst : g_Taskbars) {
        if (!inst->hTaskSwitch) continue;
        int count = SendMessageW(inst->hTaskSwitch, TB_BUTTONCOUNT, 0, 0);
        for (int i = 0; i < count; i++) {
            TBBUTTON tbb = {0};
            SendMessageW(inst->hTaskSwitch, TB_GETBUTTON, i, (LPARAM)&tbb);
            if (tbb.dwData == (DWORD_PTR)hwnd) {
                SendMessageW(inst->hTaskSwitch, TB_DELETEBUTTON, i, 0);
                break;
            }
        }
        SendMessageW(inst->hTaskSwitch, TB_AUTOSIZE, 0, 0);
    }
}

void UpdateTaskButtonActive(HWND hwndActive) {
    for (auto* inst : g_Taskbars) {
        if (!inst->hTaskSwitch) continue;
        int count = SendMessageW(inst->hTaskSwitch, TB_BUTTONCOUNT, 0, 0);
        for (int i = 0; i < count; i++) {
            TBBUTTON tbb = {0};
            SendMessageW(inst->hTaskSwitch, TB_GETBUTTON, i, (LPARAM)&tbb);
            bool isActive = (tbb.dwData == (DWORD_PTR)hwndActive);
            LRESULT state = SendMessageW(inst->hTaskSwitch, TB_GETSTATE, tbb.idCommand, 0);
            if (isActive) state |= TBSTATE_CHECKED;
            else state &= ~TBSTATE_CHECKED;
            SendMessageW(inst->hTaskSwitch, TB_SETSTATE, tbb.idCommand, state);
        }
    }
}

void SyncWindowsAcrossMonitors() {
    for (auto& btn : g_TaskButtons) {
        if (!IsWindow(btn.hwnd)) continue;
        HMONITOR hWinMon = GetWindowMonitor(btn.hwnd);
        bool needsAdd = true;

        for (auto* inst : g_Taskbars) {
            if (!inst->hTaskSwitch) continue;
            HMONITOR hTaskbarMon = MonitorFromWindow(inst->hTaskbar, MONITOR_DEFAULTTONEAREST);
            
            int count = SendMessageW(inst->hTaskSwitch, TB_BUTTONCOUNT, 0, 0);
            for (int i = 0; i < count; i++) {
                TBBUTTON tbb = {0};
                SendMessageW(inst->hTaskSwitch, TB_GETBUTTON, i, (LPARAM)&tbb);
                if ((HWND)tbb.dwData == btn.hwnd) {
                    if (hWinMon != hTaskbarMon) {
                        SendMessageW(inst->hTaskSwitch, TB_DELETEBUTTON, i, 0);
                    } else {
                        needsAdd = false;
                    }
                    break;
                }
            }
        }
        
        if (needsAdd) {
            AddTaskButton(btn);
        }
    }
}

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

HWND g_hTrayFlyout = NULL;

LRESULT CALLBACK TrayFlyoutProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        FillRect(hdc, &rcClient, (HBRUSH)GetStockObject(BLACK_BRUSH));
        
        int x = 4, y = 4;
        int totalVisible = 0;
        for (const auto& icon : g_TrayIcons) {
            if (icon.hIcon && !(icon.dwState & NIS_HIDDEN)) totalVisible++;
        }
        
        int drawn = 0;
        for (const auto& icon : g_TrayIcons) {
            if (icon.hIcon && !(icon.dwState & NIS_HIDDEN)) {
                if (drawn < totalVisible - 4) {
                    DrawIconEx(hdc, x, y, icon.hIcon, 16, 16, 0, NULL, DI_NORMAL);
                    x += 24;
                    if (x > rcClient.right - 20) { x = 4; y += 24; }
                }
                drawn++;
            }
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST) {
        int xPos = GET_X_LPARAM(lParam);
        int yPos = GET_Y_LPARAM(lParam);
        int col = (xPos - 4) / 24;
        int row = (yPos - 4) / 24;
        int cols = (160 - 8) / 24;
        int clickIndex = row * cols + col;
        
        int totalVisible = 0;
        for (const auto& icon : g_TrayIcons) {
            if (icon.hIcon && !(icon.dwState & NIS_HIDDEN)) totalVisible++;
        }
        
        int drawn = 0;
        for (const auto& icon : g_TrayIcons) {
            if (icon.hIcon && !(icon.dwState & NIS_HIDDEN)) {
                if (drawn < totalVisible - 4) {
                    if (drawn == clickIndex) {
                        PostMessageW(icon.hWnd, icon.uCallbackMessage, icon.uID, uMsg);
                        if (uMsg == WM_LBUTTONUP) ShowWindow(hwnd, SW_HIDE);
                        break;
                    }
                }
                drawn++;
            }
        }
        return 0;
    }
    if (uMsg == WM_KILLFOCUS) {
        ShowWindow(hwnd, SW_HIDE);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

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
        
        bool bIsWin7Mode = (g_Config.OverflowMode == TrayOverflowMode::Win7Flyout);
        bool bIsExpanded = (GetPropW(hwnd, L"TrayExpanded") != NULL);
        
        int totalVisible = 0;
        for (const auto& icon : g_TrayIcons) {
            if (icon.hIcon && !(icon.dwState & NIS_HIDDEN)) totalVisible++;
        }
        
        if (totalVisible > 4) {
            RECT rcChevron = { x, y, x + 16, y + 16 };
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(255, 255, 255));
            DrawTextW(hdc, bIsWin7Mode ? L"^" : (bIsExpanded ? L">" : L"<"), -1, &rcChevron, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
            x += 16;
        }

        int drawn = 0;
        for (const auto& icon : g_TrayIcons) {
            if (icon.hIcon && !(icon.dwState & NIS_HIDDEN)) {
                if (totalVisible > 4) {
                    if (bIsWin7Mode) {
                        if (drawn < totalVisible - 4) { drawn++; continue; }
                    } else {
                        if (!bIsExpanded && drawn < totalVisible - 4) { drawn++; continue; }
                    }
                }
                DrawIconEx(hdc, x, y, icon.hIcon, 16, 16, 0, NULL, DI_NORMAL);
                x += 24;
                drawn++;
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
        
        bool bIsWin7Mode = (g_Config.OverflowMode == TrayOverflowMode::Win7Flyout);
        bool bIsExpanded = (GetPropW(hwnd, L"TrayExpanded") != NULL);
        
        int totalVisible = 0;
        for (const auto& icon : g_TrayIcons) {
            if (icon.hIcon && !(icon.dwState & NIS_HIDDEN)) totalVisible++;
        }
        
        if (totalVisible > 4 && xPos < 18) {
            if (uMsg == WM_LBUTTONDOWN) {
                if (bIsWin7Mode) {
                    if (!g_hTrayFlyout) {
                        g_hTrayFlyout = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, L"TrayFlyoutWnd", L"", WS_POPUP | WS_BORDER, 0, 0, 160, 200, hwnd, NULL, GetModuleHandle(NULL), NULL);
                    }
                    RECT rcTaskbar;
                    GetWindowRect(GetParent(hwnd), &rcTaskbar);
                    SetWindowPos(g_hTrayFlyout, HWND_TOPMOST, rcTaskbar.right - 200, rcTaskbar.top - 200, 160, 200, SWP_SHOWWINDOW);
                } else {
                    if (bIsExpanded) RemovePropW(hwnd, L"TrayExpanded");
                    else SetPropW(hwnd, L"TrayExpanded", (HANDLE)1);
                    InvalidateRect(hwnd, NULL, TRUE);
                }
            }
            return 0;
        }
        
        int iconOffset = (totalVisible > 4) ? 18 : 2;
        int iconIndex = (xPos - iconOffset) / 24;
        if (iconIndex < 0) return 0;
        
        int current = 0;
        int drawn = 0;
        for (const auto& icon : g_TrayIcons) {
            if (icon.hIcon && !(icon.dwState & NIS_HIDDEN)) {
                if (totalVisible > 4) {
                    if (bIsWin7Mode) {
                        if (current < totalVisible - 4) { current++; continue; }
                    } else {
                        if (!bIsExpanded && current < totalVisible - 4) { current++; continue; }
                    }
                }
                
                if (drawn == iconIndex) {
                    PostMessageW(icon.hWnd, icon.uCallbackMessage, icon.uID, uMsg);
                    break;
                }
                drawn++;
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
    case WM_RBUTTONUP: {
        HMENU hMenu = CreatePopupMenu();
        AppendMenuW(hMenu, MF_STRING, 1000, L"Adjust date/time");
        AppendMenuW(hMenu, MF_STRING, 1001, L"Customize notification icons");
        POINT pt;
        GetCursorPos(&pt);
        SetForegroundWindow(hwnd);
        int cmd = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, NULL);
        if (cmd == 1000) {
            ShellExecuteW(NULL, L"open", L"control", L"timedate.cpl", NULL, SW_SHOWNORMAL);
        } else if (cmd == 1001) {
            ShellExecuteW(NULL, L"open", L"explorer.exe", L"shell:::{05d7b0f4-2121-4eff-bf6b-ed3f69b894d9}", NULL, SW_SHOWNORMAL);
        }
        DestroyMenu(hMenu);
        return 0;
    }
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
                
                rcClient.right -= 15;
                
                HRESULT hr = DrawThemeTextEx(hTheme, hdcBuffer, 0, 0, clockText, -1, DT_RIGHT | DT_VCENTER, &rcClient, &dttOpts);
                if (FAILED(hr)) {
                    SetTextColor(hdcBuffer, RGB(255, 255, 255));
                    SetBkMode(hdcBuffer, TRANSPARENT);
                    DrawTextW(hdcBuffer, clockText, -1, &rcClient, DT_RIGHT | DT_VCENTER | DT_NOCLIP);
                }
                
                SelectObject(hdcBuffer, hOldFont);
                DeleteObject(hFont);
                CloseThemeData(hTheme);
            } else {
                // Fallback if Theme fails inside BufferedPaint
                HFONT hFont = CreateFontIndirectW(&lf);
                HFONT hOldFont = (HFONT)SelectObject(hdcBuffer, hFont);
                SetTextColor(hdcBuffer, RGB(255, 255, 255));
                SetBkMode(hdcBuffer, TRANSPARENT);
                rcClient.right -= 15;
                DrawTextW(hdcBuffer, clockText, -1, &rcClient, DT_RIGHT | DT_VCENTER | DT_NOCLIP);
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
            rcClient.right -= 15;
            DrawTextW(hdc, clockText, -1, &rcClient, DT_RIGHT | DT_VCENTER | DT_NOCLIP);
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
    case WM_MOUSEMOVE: {
        if (!GetPropW(hwnd, L"Hover")) {
            TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, hwnd, 0 };
            TrackMouseEvent(&tme);
            SetPropW(hwnd, L"Hover", (HANDLE)1);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }
    case WM_MOUSELEAVE: {
        RemovePropW(hwnd, L"Hover");
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }
    case WM_LBUTTONDOWN: {
        SetPropW(hwnd, L"Pressed", (HANDLE)1);
        InvalidateRect(hwnd, NULL, FALSE);
        SetCapture(hwnd);
        return 0;
    }
    case WM_LBUTTONUP: {
        RemovePropW(hwnd, L"Pressed");
        InvalidateRect(hwnd, NULL, FALSE);
        ReleaseCapture();
        
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        RECT rc; GetClientRect(hwnd, &rc);
        if (PtInRect(&rc, pt)) {
            // Simulate Win+D
            keybd_event(VK_LWIN, 0, 0, 0);
            keybd_event('D', 0, 0, 0);
            keybd_event('D', 0, KEYEVENTF_KEYUP, 0);
            keybd_event(VK_LWIN, 0, KEYEVENTF_KEYUP, 0);
        }
        return 0;
    }
    case WM_NCHITTEST:
        return HTCLIENT;
        
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        
        // Completely clear background
        DrawThemeParentBackground(hwnd, hdc, &rcClient);
        
        bool isHover = GetPropW(hwnd, L"Hover") != NULL;
        bool isPressed = GetPropW(hwnd, L"Pressed") != NULL;
        
        BOOL isCompositionEnabled = FALSE;
        DwmIsCompositionEnabled(&isCompositionEnabled);
        
        if (isCompositionEnabled) {
            // Draw etched glass separator using Gdiplus
            Gdiplus::Graphics graphics(hdc);
            Gdiplus::Pen pen(Gdiplus::Color(100, 255, 255, 255), 1); // Faint semi-transparent white
            graphics.DrawLine(&pen, 0, 2, 0, rcClient.bottom - 2);
            
            if (isHover || isPressed) {
                Gdiplus::SolidBrush hoverBrush(isPressed ? Gdiplus::Color(80, 255, 255, 255) : Gdiplus::Color(40, 255, 255, 255));
                graphics.FillRectangle(&hoverBrush, 1, 0, rcClient.right - 1, rcClient.bottom);
            }
        } else {
            if (isPressed || isHover) {
                HBRUSH hbr = CreateSolidBrush(isPressed ? RGB(100, 100, 100) : RGB(150, 150, 150));
                FillRect(hdc, &rcClient, hbr);
                DeleteObject(hbr);
            }
            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(180, 180, 180));
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
            MoveToEx(hdc, 0, 0, NULL);
            LineTo(hdc, 0, rcClient.bottom);
            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
        }
        
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
                
                info.cmdId = g_NextCmdId++;
                g_TaskButtons.push_back(info);
                AddTaskButton(g_TaskButtons.back());
            }
        }
        else if (nCode == HSHELL_WINDOWDESTROYED) {
            for (auto it = g_TaskButtons.begin(); it != g_TaskButtons.end(); ++it) {
                if (it->hwnd == hwndShell) {
                    g_TaskButtons.erase(it);
                    RemoveTaskButton(hwndShell);
                    break;
                }
            }
        }
        else if (nCode == HSHELL_WINDOWACTIVATED || nCode == HSHELL_RUDEAPPACTIVATED) {
            for (auto& btn : g_TaskButtons) {
                btn.isActive = (btn.hwnd == hwndShell);
            }
            UpdateTaskButtonActive(hwndShell);
        }
        else if (nCode == HSHELL_REDRAW) {
            for (auto& btn : g_TaskButtons) {
                if (btn.hwnd == hwndShell) {
                    WCHAR szTitle[256] = {0};
                    GetWindowTextW(hwndShell, szTitle, 256);
                    btn.title = szTitle;
                    
                    DWORD_PTR dwRes = 0;
                    if (SendMessageTimeoutW(hwndShell, WM_GETICON, ICON_SMALL, 0, SMTO_ABORTIFHUNG, 500, &dwRes)) {
                        if (dwRes) btn.hIcon = (HICON)dwRes;
                    }
                    RemoveTaskButton(hwndShell);
                    AddTaskButton(btn);
                    break;
                }
            }
        }
        return 0;
    }

    if (uMsg == WM_COPYDATA) {
        for (auto* inst : g_Taskbars) {
            if (inst->hTaskbar == hwnd && inst->hTrayNotify) {
                return SendMessageW(inst->hTrayNotify, uMsg, wParam, lParam);
            }
        }
    }

    switch (uMsg) {
    case WM_CREATE: {
        SetTimer(hwnd, 9999, 100, NULL);
        SetTimer(hwnd, 9998, 500, NULL);
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
            HBRUSH hbr = CreateSolidBrush(RGB(40, 40, 40));
            FillRect(hdc, &rcClient, hbr);
            DeleteObject(hbr);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_NOTIFY: {
        TaskbarInstance* inst = GetTaskbarInstance(hwnd);
        LPNMHDR nmhdr = (LPNMHDR)lParam;
        if (nmhdr->code == NM_CUSTOMDRAW) {
            LPNMTBCUSTOMDRAW lpNMCustomDraw = (LPNMTBCUSTOMDRAW)lParam;
            if (inst && (lpNMCustomDraw->nmcd.hdr.hwndFrom == inst->hTaskSwitch || lpNMCustomDraw->nmcd.hdr.hwndFrom == inst->hToolbar)) {
                if (lpNMCustomDraw->nmcd.dwDrawStage == CDDS_PREPAINT) {
                    return CDRF_NOTIFYITEMDRAW;
                } else if (lpNMCustomDraw->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
                    lpNMCustomDraw->clrText = RGB(255, 255, 255);
                    return CDRF_DODEFAULT;
                }
            }
        }
        else if (nmhdr->code == TBN_HOTITEMCHANGE) {
            LPNMTBHOTITEM lpnmhi = (LPNMTBHOTITEM)lParam;
            if (g_Config.ShowPreviews) {
                if (lpnmhi->dwFlags & HICF_ENTERING) {
                    TBBUTTON tbb = {0};
                    if (SendMessageW(nmhdr->hwndFrom, TB_GETBUTTON, lpnmhi->idNew, (LPARAM)&tbb)) {
                        g_PreviewTargetHwnd = (HWND)tbb.dwData;
                        if (g_PreviewTimer) KillTimer(hwnd, g_PreviewTimer);
                        g_PreviewTimer = SetTimer(hwnd, 2000, 400, NULL);
                    }
                } else if (lpnmhi->dwFlags & HICF_LEAVING) {
                    if (g_PreviewTimer) {
                        KillTimer(hwnd, g_PreviewTimer);
                        g_PreviewTimer = 0;
                    }
                    if (g_hPreviewWindow) {
                        ShowWindow(g_hPreviewWindow, SW_HIDE);
                    }
                }
            }
        }
        else if (nmhdr->code == NM_RCLICK) {
            POINT pt;
            GetCursorPos(&pt);

            HMENU hMenu = CreatePopupMenu();
            POINT ptClient = pt;
            ScreenToClient(nmhdr->hwndFrom, &ptClient);
            int btnIndex = SendMessageW(nmhdr->hwndFrom, TB_HITTEST, 0, (LPARAM)&ptClient);
            if (btnIndex >= 0) {
                TBBUTTON tbb = {0};
                if (SendMessageW(nmhdr->hwndFrom, TB_GETBUTTON, btnIndex, (LPARAM)&tbb)) {
                    HWND targetHwnd = (HWND)tbb.dwData;
                    if (IsWindow(targetHwnd)) {
                        HMENU hJumpList = CreatePopupMenu();
                        
                        WCHAR szTitle[256] = {0};
                        GetWindowTextW(targetHwnd, szTitle, 256);
                        
                        // Basic JumpList look: Title at top, Close at bottom.
                        AppendMenuW(hJumpList, MF_STRING | MF_DISABLED, 0, szTitle);
                        AppendMenuW(hJumpList, MF_SEPARATOR, 0, NULL);
                        AppendMenuW(hJumpList, MF_STRING, 1, L"Close window");
                        
                        SetForegroundWindow(hwnd);
                        int cmd = TrackPopupMenu(hJumpList, TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, NULL);
                        if (cmd == 1) {
                            PostMessageW(targetHwnd, WM_CLOSE, 0, 0);
                        }
                        DestroyMenu(hJumpList);
                    }
                }
                DestroyMenu(hMenu);
                return 1;
            }

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
                AppendMenuW(hMenu, MF_STRING, IDM_RESTART_SHELL, L"Restart Explorer");
            }
            AppendMenuW(hMenu, MF_STRING, IDM_EXIT_ELITETASKBAR, L"Exit This Taskbar");
            AppendMenuW(hMenu, MF_STRING, IDM_EXIT_ALL_ELITETASKBAR, L"Exit All Taskbars");

            SetForegroundWindow(hwnd);
            TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
            DestroyMenu(hMenu);
            return 1;
        }
        break;
    }
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        
        // Handle Toolbar buttons
        if (wmId >= 4000) {
            for (const auto& btn : g_TaskButtons) {
                if (btn.cmdId == wmId) {
                    if (IsIconic(btn.hwnd)) {
                        PostMessageW(btn.hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
                        SetForegroundWindow(btn.hwnd);
                    } else if (GetForegroundWindow() == btn.hwnd) {
                        PostMessageW(btn.hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
                    } else {
                        SetForegroundWindow(btn.hwnd);
                    }
                    break;
                }
            }
            return 0;
        }

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
            case IDM_EXIT_ALL_ELITETASKBAR:
                for (auto* tb : g_Taskbars) {
                    if (IsWindow(tb->hTaskbar)) {
                        PostMessageW(tb->hTaskbar, WM_CLOSE, 0, 0);
                    }
                }
                break;
            case IDM_START_EXPLORER:
                ShellExecuteW(NULL, L"open", L"explorer.exe", L"shell:::{20D04FE0-3AEA-1069-A2D8-08002B30309D}", NULL, SW_SHOWNORMAL); // Opens "This PC"
                break;
            case IDM_RESTART_SHELL:
                // Restart Explorer Shell
                ShellExecuteW(NULL, NULL, L"powershell.exe", L"-NoProfile -WindowStyle Hidden -Command \"Stop-Process -Name explorer -Force; Start-Process explorer.exe\"", NULL, SW_HIDE);
                break;
        }
        return 0;
    }
    case WM_LBUTTONDOWN: {
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
            AppendMenuW(hMenu, MF_STRING, IDM_EXIT_ELITETASKBAR, L"Exit This Taskbar");
            AppendMenuW(hMenu, MF_STRING, IDM_EXIT_ALL_ELITETASKBAR, L"Exit All Taskbars");
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
            AppendMenuW(hMenu, MF_STRING, IDM_EXIT_ELITETASKBAR, L"Exit This Taskbar");
            AppendMenuW(hMenu, MF_STRING, IDM_EXIT_ALL_ELITETASKBAR, L"Exit All Taskbars");
        }

        SetForegroundWindow(hwnd);
        int cmd = TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, hwnd, NULL);
        if (cmd != 0) {
            PostMessageW(hwnd, WM_COMMAND, MAKEWPARAM(cmd, 0), 0);
        }
        DestroyMenu(hMenu);
        return 0;
    }
    case WM_TIMER: {
        if (wParam == 1001) {
            TaskbarInstance* inst = GetTaskbarInstance(hwnd);
            if (inst && inst->hToolbar) {
                std::vector<ScrapedTrayIcon> icons = ScrapeTrayIcons();
                UpdateTrayToolbar(inst->hToolbar, inst->hTrayImageList, icons);
            }
            return 0;
        }
        if (wParam == 9998) {
            SyncWindowsAcrossMonitors();
            return 0;
        }
        if (wParam == 9999 && g_Config.Mode == TaskbarMode::Replace && g_hNativeTaskbar) {
            if (IsWindowVisible(g_hNativeTaskbar)) {
                ShowWindow(g_hNativeTaskbar, SW_HIDE);
                SetWindowPos(g_hNativeTaskbar, NULL, -10000, -10000, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
            }
            SyncTaskbarButtonsAcrossMonitors();
        } else if (wParam == 9999) {
            SyncTaskbarButtonsAcrossMonitors();
        } else if (wParam == 2000) { // Preview timer
            KillTimer(hwnd, 2000);
            g_PreviewTimer = 0;
            if (g_Config.ShowPreviews && IsWindow(g_PreviewTargetHwnd)) {
                if (!g_hPreviewWindow) {
                    g_hPreviewWindow = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED, L"EliteTaskbarPreview", L"", WS_POPUP | WS_CLIPCHILDREN, 0, 0, 200, 150, hwnd, NULL, GetModuleHandle(NULL), NULL);
                }
                
                SetWindowLongPtrW(g_hPreviewWindow, GWLP_USERDATA, (LONG_PTR)g_PreviewTargetHwnd);
                
                POINT pt; GetCursorPos(&pt);
                int previewWidth = 240;
                int previewHeight = 160;
                
                RECT rcTaskbar;
                GetWindowRect(hwnd, &rcTaskbar);
                int yPos = rcTaskbar.top - previewHeight - 5;
                int xPos = pt.x - (previewWidth / 2);
                
                SetWindowPos(g_hPreviewWindow, HWND_TOPMOST, xPos, yPos, previewWidth, previewHeight, SWP_SHOWWINDOW | SWP_NOACTIVATE);
                SetLayeredWindowAttributes(g_hPreviewWindow, 0, 255, LWA_ALPHA);
                InvalidateRect(g_hPreviewWindow, NULL, TRUE);
                
                if (g_hPreviewThumb) {
                    DwmUnregisterThumbnail(g_hPreviewThumb);
                    g_hPreviewThumb = NULL;
                }
                
                if (SUCCEEDED(DwmRegisterThumbnail(g_hPreviewWindow, g_PreviewTargetHwnd, &g_hPreviewThumb))) {
                    DWM_THUMBNAIL_PROPERTIES props = {0};
                    props.dwFlags = DWM_TNP_VISIBLE | DWM_TNP_RECTDESTINATION | DWM_TNP_SOURCECLIENTAREAONLY;
                    props.fVisible = TRUE;
                    props.rcDestination.left = 10;
                    props.rcDestination.top = 30; // Leave 30px for title and close button
                    props.rcDestination.right = previewWidth - 10;
                    props.rcDestination.bottom = previewHeight - 10;
                    props.fSourceClientAreaOnly = FALSE;
                    DwmUpdateThumbnailProperties(g_hPreviewThumb, &props);
                }
            }
        }
        return 0;
    }
    case WM_SETTINGCHANGE: {
        if (lParam && wcscmp((LPCWSTR)lParam, L"TraySettings") == 0) {
            bool requiresRestart = false;
            HKEY hKey;
            if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                DWORD dwValue = 0;
                DWORD cbData = sizeof(DWORD);
                if (RegQueryValueExW(hKey, L"TaskbarMode", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                    bool newIsReplace = (dwValue == 1);
                    bool oldIsReplace = (g_Config.Mode == TaskbarMode::Replace);
                    if (newIsReplace != oldIsReplace) {
                        requiresRestart = true;
                    }
                }
                cbData = sizeof(DWORD);
                if (RegQueryValueExW(hKey, L"UseNativeTaskBand", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                    bool newIsNative = (dwValue == 1);
                    if (newIsNative != g_Config.UseNativeTaskBand) {
                        requiresRestart = true;
                    }
                }
                RegCloseKey(hKey);
            }
            
            if (requiresRestart) {
                PostMessageW(hwnd, WM_COMMAND, IDM_RESTART_SHELL, 0);
            } else {
                QueryOperationalMode();
                
                for (auto* inst : g_Taskbars) {
                    if (inst->hTaskSwitch) {
                        int count = SendMessageW(inst->hTaskSwitch, TB_BUTTONCOUNT, 0, 0);
                        for (int i = count - 1; i >= 0; i--) {
                            SendMessageW(inst->hTaskSwitch, TB_DELETEBUTTON, i, 0);
                        }
                    }
                    if (inst->hTrayNotify) {
                        InvalidateRect(inst->hTrayNotify, NULL, TRUE);
                    }
                }
                
                for (auto& btn : g_TaskButtons) {
                    AddTaskButton(btn);
                }
                
                InvalidateRect(hwnd, NULL, TRUE);
            }
        } else if (lParam && wcscmp((LPCWSTR)lParam, L"EliteTaskbarSettings") == 0) {
            for (auto* inst : g_Taskbars) {
                if (inst->hTaskbar == hwnd && inst->startButton && inst->startButton->GetHwnd()) {
                    inst->startButton->ReloadOrbImage(GetModuleHandleW(NULL), inst->monitorIndex);
                    
                    RECT rcClient;
                    GetClientRect(hwnd, &rcClient);
                    inst->startButton->Show(0, 0, rcClient.bottom);
                    break;
                }
            }
        }
        return 0;
    }
    case WM_DESTROY:
    {
        for (auto it = g_Taskbars.begin(); it != g_Taskbars.end(); ++it) {
            if ((*it)->hTaskbar == hwnd) {
                g_Taskbars.erase(it);
                break;
            }
        }
        
        bool hasBrowser = false;
        EnumThreadWindows(GetCurrentThreadId(), [](HWND hwndEnum, LPARAM lParam) -> BOOL {
            wchar_t className[256];
            if (GetClassNameW(hwndEnum, className, 256)) {
                if (wcscmp(className, L"Win32Explorer") == 0) {
                    *(bool*)lParam = true;
                    return FALSE;
                }
            }
            return TRUE;
        }, (LPARAM)&hasBrowser);
        
        if (!hasBrowser && g_Taskbars.empty()) {
            PostQuitMessage(0);
        }
        return 0;
    }
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

    WNDCLASSW wcPrev = {0};
    wcPrev.lpfnWndProc = PreviewWndProc;
    wcPrev.hInstance = hInstance;
    wcPrev.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcPrev.lpszClassName = L"EliteTaskbarPreview";
    RegisterClassW(&wcPrev);

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

    auto setVis = [](HWND hwnd, bool vis) {
        if (!hwnd) return;
        if (vis) {
            SetWindowRgn(hwnd, NULL, TRUE);
            ShowWindow(hwnd, SW_SHOW);
        } else {
            HRGN hRgn = CreateRectRgn(0, 0, 0, 0);
            SetWindowRgn(hwnd, hRgn, TRUE);
        }
    };

    g_hNativeTaskbar = FindWindowW(L"Shell_TrayWnd", NULL);
    if (g_hNativeTaskbar) {
        RECT nativeRect = {0};
        GetWindowRect(g_hNativeTaskbar, &nativeRect);
        int nativeHeight = nativeRect.bottom - nativeRect.top;
        if (nativeHeight > 0 && nativeHeight < 100) {
            taskbarHeight = nativeHeight;
        }
        if (g_Config.Mode == TaskbarMode::Replace) {
            setVis(g_hNativeTaskbar, false);
            HWND hSec = NULL;
            while ((hSec = FindWindowExW(NULL, hSec, L"Shell_SecondaryTrayWnd", NULL)) != NULL) {
                setVis(hSec, false);
            }
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

    if (g_Config.Mode == TaskbarMode::Replace) {
        wc.lpszClassName = SEC_CLASS_NAME;
        RegisterClassExW(&wc);
        wc.lpszClassName = CLASS_NAME; // Restore original
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

    wcChild.lpfnWndProc = TrayFlyoutProc;
    wcChild.lpszClassName = L"TrayFlyoutWnd";
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
        inst->monitorIndex = i;
        inst->monitorRect = monData.rects[i];
        inst->taskbarHeight = taskbarHeight;
        inst->hNativeTrayNotify = NULL;
        inst->bStolenSysPager = false;
        
        int screenWidth = inst->monitorRect.right - inst->monitorRect.left;
        int screenHeight = inst->monitorRect.bottom - inst->monitorRect.top;
        int xPos = inst->monitorRect.left;
        int yPos = inst->monitorRect.bottom - taskbarHeight;

        LPCWSTR szClassName = TRAY_CLASS_NAME;
        if (g_Config.Mode == TaskbarMode::Replace) {
            szClassName = monData.isPrimary[i] ? CLASS_NAME : SEC_CLASS_NAME;
        }

        inst->hTaskbar = CreateWindowExW(
            WS_EX_TOOLWINDOW,
            szClassName, L"", WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
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

        // Read multi-monitor settings for this monitor
        DWORD enableTray = 1;
        DWORD enableClock = 1;
        DWORD enableTaskBtns = 1;
        HKEY hKeyAdv;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKeyAdv) == ERROR_SUCCESS) {
            DWORD cbData = sizeof(DWORD);
            WCHAR valName[64];
            wsprintfW(valName, L"EnableTray_Mon%d", (int)i);
            RegQueryValueExW(hKeyAdv, valName, NULL, NULL, (LPBYTE)&enableTray, &cbData);
            cbData = sizeof(DWORD);
            wsprintfW(valName, L"EnableClock_Mon%d", (int)i);
            RegQueryValueExW(hKeyAdv, valName, NULL, NULL, (LPBYTE)&enableClock, &cbData);
            cbData = sizeof(DWORD);
            wsprintfW(valName, L"EnableTaskBtns_Mon%d", (int)i);
            RegQueryValueExW(hKeyAdv, valName, NULL, NULL, (LPBYTE)&enableTaskBtns, &cbData);
            RegCloseKey(hKeyAdv);
        }

        inst->hTaskSwitch = NULL;
        if (enableTaskBtns) {
            inst->hTaskSwitch = CreateWindowExW(0, TOOLBARCLASSNAMEW, L"", 
                WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TBSTYLE_LIST | TBSTYLE_FLAT | TBSTYLE_WRAPABLE | CCS_NODIVIDER | CCS_NORESIZE | TBSTYLE_TRANSPARENT, 
                60, 0, screenWidth - 315, taskbarHeight, inst->hTaskbar, (HMENU)2000, hInstance, NULL);
            SetWindowSubclass(inst->hTaskSwitch, TaskSwitchSubclassProc, 1, 0);
            SendMessageW(inst->hTaskSwitch, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
            
            // Add padding so icons aren't cramped to the left edge
            SendMessageW(inst->hTaskSwitch, TB_SETPADDING, 0, MAKELPARAM(10, 4));
            if (g_Config.ButtonWidth == ButtonWidthMode::Fixed) {
                int width = 160;
                if (g_Config.FixedWidthSize == 0) width = 100; // Small
                else if (g_Config.FixedWidthSize == 2) width = 220; // Large
                SendMessageW(inst->hTaskSwitch, TB_SETBUTTONWIDTH, 0, MAKELPARAM(width, width));
            } else if (g_Config.ButtonWidth == ButtonWidthMode::IconsOnly) {
                SendMessageW(inst->hTaskSwitch, TB_SETBUTTONWIDTH, 0, MAKELPARAM(40, 40));
            }
            
            SetWindowTheme(inst->hTaskSwitch, L"TaskBand", NULL);
            inst->hImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 10);
            SendMessageW(inst->hTaskSwitch, TB_SETIMAGELIST, 0, (LPARAM)inst->hImageList);
        }

        inst->hTrayNotify = NULL;
        inst->hSysPager = NULL;
        inst->hToolbar = NULL;
        inst->hTrayClock = NULL;

        if (enableTray || enableClock) {
            inst->hTrayNotify = CreateWindowExW(0, L"TrayNotifyWnd", L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, screenWidth - 255, 0, 240, taskbarHeight, inst->hTaskbar, NULL, hInstance, NULL);
            
            if (enableTray) {
                inst->hSysPager = CreateWindowExW(0, L"SysPager", L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0, 100, taskbarHeight, inst->hTrayNotify, NULL, hInstance, NULL);
                inst->hToolbar = CreateWindowExW(0, L"ToolbarWindow32", L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | CCS_NODIVIDER | CCS_NORESIZE, 0, 0, 100, taskbarHeight, inst->hSysPager, NULL, hInstance, NULL);
                SendMessageW(inst->hToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
                inst->hTrayImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 20);
                SendMessageW(inst->hToolbar, TB_SETIMAGELIST, 0, (LPARAM)inst->hTrayImageList);
            }
            
            if (enableClock) {
                inst->hTrayClock = CreateWindowExW(0, L"TrayClockWClass", L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, 100, 0, 140, taskbarHeight, inst->hTrayNotify, NULL, hInstance, NULL);
            }
            
            CreateWindowExW(0, L"TrayShowDesktopButtonWClass", L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, screenWidth - 15, 0, 15, taskbarHeight, inst->hTaskbar, NULL, hInstance, NULL);
        }

        SetWindowPos(inst->hTaskbar, HWND_TOPMOST, xPos, yPos, screenWidth, taskbarHeight, SWP_SHOWWINDOW);
        
        // Timer for scraper
        if (inst->hToolbar) {
            SetTimer(inst->hTaskbar, 1001, 2000, NULL);
        }


        if (g_Config.Mode == TaskbarMode::Replace && monData.isPrimary[i]) {
            RegisterHotKey(inst->hTaskbar, 1, MOD_WIN | MOD_NOREPEAT, 'E');
            RegisterHotKey(inst->hTaskbar, 2, MOD_WIN | MOD_NOREPEAT, 'R');
            RegisterHotKey(inst->hTaskbar, 3, MOD_WIN | MOD_NOREPEAT, 'D');
        }

        if (monData.isPrimary[i] || g_Config.Mode == TaskbarMode::Replace) {
            APPBARDATA abd = {0};
            abd.cbSize = sizeof(APPBARDATA);
            abd.hWnd = inst->hTaskbar;
            abd.uCallbackMessage = WM_USER + 100;
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
        if (inst->startButton->Initialize(hInstance, inst->hTaskbar, inst->monitorIndex)) {
            inst->startButton->ReloadOrbImage(hInstance, inst->monitorIndex);
            inst->startButton->Show(xPos, yPos, taskbarHeight);
        }
        
        g_Taskbars.push_back(inst);
    }

    if (g_Taskbars.empty()) {
        Logger::Log(L"No taskbars were created. Exiting.");
        return false;
    }

    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        if (IsWindowVisible(hwnd) && GetWindowTextLengthW(hwnd) > 0) {
            if (GetWindow(hwnd, GW_OWNER) == NULL) {
                LONG exStyle = GetWindowLongW(hwnd, GWL_EXSTYLE);
                if (!(exStyle & WS_EX_TOOLWINDOW)) {
                    TaskButtonInfo info;
                    info.hwnd = hwnd;
                    info.isActive = (hwnd == GetForegroundWindow());
                    
                    WCHAR szTitle[256] = {0};
                    GetWindowTextW(hwnd, szTitle, 256);
                    info.title = szTitle;
                    
                    DWORD_PTR dwRes = 0;
                    SendMessageTimeoutW(hwnd, WM_GETICON, ICON_SMALL, 0, SMTO_ABORTIFHUNG, 500, &dwRes);
                    info.hIcon = (HICON)dwRes;
                    if (!info.hIcon) {
                        info.hIcon = (HICON)GetClassLongPtrW(hwnd, GCLP_HICONSM);
                    }
                    
                    info.cmdId = g_NextCmdId++;
                    g_TaskButtons.push_back(info);
                    AddTaskButton(g_TaskButtons.back());
                }
            }
        }
        return TRUE;
    }, 0);

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
        
        if (inst->bStolenSysPager && inst->hSysPager && inst->hNativeTrayNotify) {
            SetParent(inst->hSysPager, inst->hNativeTrayNotify);
            SetWindowPos(inst->hSysPager, NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE);
        }
        
        DestroyWindow(inst->hTaskbar);
        delete inst;
    }
    g_Taskbars.clear();
    
    if (g_hNativeTaskbar && IsWindow(g_hNativeTaskbar)) {
        SetWindowRgn(g_hNativeTaskbar, NULL, TRUE);
        ShowWindow(g_hNativeTaskbar, SW_SHOW);
        PostMessageW(g_hNativeTaskbar, WM_DISPLAYCHANGE, 0, 0);

        HWND hSec = NULL;
        while ((hSec = FindWindowExW(NULL, hSec, L"Shell_SecondaryTrayWnd", NULL)) != NULL) {
            SetWindowRgn(hSec, NULL, TRUE);
            ShowWindow(hSec, SW_SHOW);
        }
    } else {
        ShellExecuteW(NULL, L"open", L"explorer.exe", NULL, NULL, SW_SHOWNORMAL);
    }
}





bool IsEliteTaskbarRunning() {
    return !g_Taskbars.empty();
}
