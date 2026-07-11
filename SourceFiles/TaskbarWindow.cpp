#include "stdafx.h" // - Draftsman-Dan
#pragma warning(disable: 4100 4244 4267 4189 4312)
#include <windows.h>
#include "TaskbarWindow.h"
#include "StartButton.h"
#include "ClockWidget.h"
#include "TaskbarProperties.h"
#include "Logger.h"
#include "resource.h"
#include "Config.h"
#include "DesktopWindow.h"
#include <dwmapi.h>
#include <windowsx.h>
#include <uxtheme.h>
#include <vector>
#include <shellapi.h>
#include <shobjidl.h>
#include <propkey.h>
#include "TrayIconScraper.h"
#include <commctrl.h>
#include <shlobj.h>
#include <tlhelp32.h>

#define WM_TRAY_CALLBACK_WIN32EXPLORER (WM_USER + 500)
#define WM_TRAY_CALLBACK_TASKBAR (WM_USER + 501)
#define WM_TRAY_CALLBACK_DESKTOP (WM_USER + 502)
#define TRAY_LIMIT 48


static void InvokeNativeRunDialog(HWND hwndOwner);

#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

class FolderBand {
public:
    HWND hToolbar;
    HIMAGELIST hImgListSmall;
    HIMAGELIST hImgListLarge;
    std::wstring folderName;
    std::wstring folderPath;
    std::vector<std::wstring*> buttonTargets;

    bool showTitle;
    bool showText;
    int iconSize; // 0 = small, 1 = large

    FolderBand() : hToolbar(NULL), hImgListSmall(NULL), hImgListLarge(NULL), showTitle(true), showText(false), iconSize(0) {}

    ~FolderBand() {
        if (hToolbar) {
            DestroyWindow(hToolbar);
        }
        if (hImgListSmall) {
            ImageList_Destroy(hImgListSmall);
        }
        if (hImgListLarge) {
            ImageList_Destroy(hImgListLarge);
        }
        for (auto* ptr : buttonTargets) {
            delete ptr;
        }
    }

    void LoadCacheSettings() {
        HKEY hKey;
        std::wstring keyPath = L"SOFTWARE\\EliteSoftware\\Win32Explorer\\Toolbars\\Cache\\" + folderName;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, keyPath.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD val = 0, cb = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"ShowTitle", NULL, NULL, (LPBYTE)&val, &cb) == ERROR_SUCCESS) {
                showTitle = (val != 0);
            }
            cb = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"ShowText", NULL, NULL, (LPBYTE)&val, &cb) == ERROR_SUCCESS) {
                showText = (val != 0);
            }
            cb = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"IconSize", NULL, NULL, (LPBYTE)&val, &cb) == ERROR_SUCCESS) {
                iconSize = (int)val;
            }
            RegCloseKey(hKey);
        }
    }

    void SaveCacheSettings() {
        HKEY hKey;
        std::wstring keyPath = L"SOFTWARE\\EliteSoftware\\Win32Explorer\\Toolbars\\Cache\\" + folderName;
        if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, keyPath.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
            DWORD val = showTitle ? 1 : 0;
            RegSetValueExW(hKey, L"ShowTitle", 0, REG_DWORD, (const BYTE*)&val, sizeof(DWORD));
            val = showText ? 1 : 0;
            RegSetValueExW(hKey, L"ShowText", 0, REG_DWORD, (const BYTE*)&val, sizeof(DWORD));
            val = iconSize;
            RegSetValueExW(hKey, L"IconSize", 0, REG_DWORD, (const BYTE*)&val, sizeof(DWORD));
            RegCloseKey(hKey);
        }
    }

    bool Initialize(HWND hParentRebar, const std::wstring& name, const std::wstring& rawPath) {
        folderName = name;
        
        wchar_t expanded[MAX_PATH];
        ExpandEnvironmentStringsW(rawPath.c_str(), expanded, MAX_PATH);
        folderPath = expanded;

        LoadCacheSettings();

        hImgListSmall = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 10);
        hImgListLarge = ImageList_Create(32, 32, ILC_COLOR32 | ILC_MASK, 0, 10);

        DWORD style = WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | CCS_NORESIZE | CCS_NODIVIDER;
        if (showText) {
            style |= TBSTYLE_LIST;
        }
        hToolbar = CreateWindowExW(0, TOOLBARCLASSNAMEW, NULL, style, 0, 0, 0, 0, hParentRebar, NULL, GetModuleHandle(NULL), NULL);
        if (!hToolbar) return false;

        SendMessageW(hToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
        SendMessageW(hToolbar, TB_SETIMAGELIST, 0, (LPARAM)(iconSize == 1 ? hImgListLarge : hImgListSmall));
        int btnDim = iconSize == 1 ? 32 : 16;
        SendMessageW(hToolbar, TB_SETBITMAPSIZE, 0, MAKELPARAM(btnDim, btnDim));

        std::wstring searchPath = folderPath + L"\\*";
        WIN32_FIND_DATAW fd;
        HANDLE hFind = FindFirstFileW(searchPath.c_str(), &fd);
        if (hFind != INVALID_HANDLE_VALUE) {
            int imageIndex = 0;
            std::vector<TBBUTTON> buttons;
            do {
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    continue;
                }
                std::wstring fileName = fd.cFileName;
                std::wstring fullPath = folderPath + L"\\" + fileName;

                HICON hIconSmall = NULL;
                HICON hIconLarge = NULL;
                SHFILEINFOW sfi;
                if (SHGetFileInfoW(fullPath.c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_SMALLICON)) {
                    hIconSmall = sfi.hIcon;
                }
                if (SHGetFileInfoW(fullPath.c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON)) {
                    hIconLarge = sfi.hIcon;
                }

                if (hIconSmall && hIconLarge) {
                    ImageList_AddIcon(hImgListSmall, hIconSmall);
                    ImageList_AddIcon(hImgListLarge, hIconLarge);
                    DestroyIcon(hIconSmall);
                    DestroyIcon(hIconLarge);

                    TBBUTTON btn = {0};
                    btn.iBitmap = imageIndex++;
                    btn.idCommand = 3000 + imageIndex;
                    btn.fsState = TBSTATE_ENABLED;
                    btn.fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
                    
                    std::wstring* pPath = new std::wstring(fullPath);
                    buttonTargets.push_back(pPath);
                    btn.dwData = (DWORD_PTR)pPath;

                    wchar_t label[MAX_PATH];
                    wcscpy_s(label, fileName.c_str());
                    PathRemoveExtensionW(label);
                    btn.iString = (INT_PTR)label;

                    buttons.push_back(btn);
                }
            } while (FindNextFileW(hFind, &fd));
            FindClose(hFind);

            if (!buttons.empty()) {
                SendMessageW(hToolbar, TB_ADDBUTTONS, buttons.size(), (LPARAM)buttons.data());
            }
        }

        SendMessageW(hToolbar, TB_SETPADDING, 0, MAKELPARAM(6, 4));
        SendMessageW(hToolbar, TB_AUTOSIZE, 0, 0);

        return true;
    }

    void ToggleTitle(HWND hRebar, int bandIndex) {
        showTitle = !showTitle;
        REBARBANDINFOW rbbi = {0};
        rbbi.cbSize = sizeof(rbbi);
        rbbi.fMask = RBBIM_STYLE;
        SendMessageW(hRebar, RB_GETBANDINFO, bandIndex, (LPARAM)&rbbi);
        if (showTitle) {
            rbbi.fStyle &= ~RBBS_HIDETITLE;
        } else {
            rbbi.fStyle |= RBBS_HIDETITLE;
        }
        SendMessageW(hRebar, RB_SETBANDINFO, bandIndex, (LPARAM)&rbbi);
        SaveCacheSettings();
    }

    void ToggleText() {
        showText = !showText;
        DWORD style = GetWindowLongW(hToolbar, GWL_STYLE);
        if (showText) {
            style |= TBSTYLE_LIST;
        } else {
            style &= ~TBSTYLE_LIST;
        }
        SetWindowLongW(hToolbar, GWL_STYLE, style);
        SendMessageW(hToolbar, TB_AUTOSIZE, 0, 0);
        SaveCacheSettings();
    }

    void ToggleIconSize() {
        iconSize = (iconSize == 0) ? 1 : 0;
        SendMessageW(hToolbar, TB_SETIMAGELIST, 0, (LPARAM)(iconSize == 1 ? hImgListLarge : hImgListSmall));
        int btnDim = iconSize == 1 ? 32 : 16;
        SendMessageW(hToolbar, TB_SETBITMAPSIZE, 0, MAKELPARAM(btnDim, btnDim));
        SendMessageW(hToolbar, TB_SETBUTTONSIZE, 0, MAKELPARAM(btnDim + 12, btnDim + 12));
        SendMessageW(hToolbar, TB_AUTOSIZE, 0, 0);
        SaveCacheSettings();
    }
};

std::vector<FolderBand*> g_FolderBands;
bool g_IsRestarting = false;

std::wstring ResolveShortcut(const std::wstring& shortcutPath) {
    CoInitialize(NULL);
    IShellLinkW* psl = NULL;
    std::wstring targetPath = shortcutPath;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&psl);
    if (SUCCEEDED(hr)) {
        IPersistFile* ppf = NULL;
        hr = psl->QueryInterface(IID_IPersistFile, (void**)&ppf);
        if (SUCCEEDED(hr)) {
            hr = ppf->Load(shortcutPath.c_str(), STGM_READ);
            if (SUCCEEDED(hr)) {
                wchar_t szGotPath[MAX_PATH];
                WIN32_FIND_DATAW wfd;
                hr = psl->GetPath(szGotPath, MAX_PATH, &wfd, SLGP_UNCPRIORITY);
                if (SUCCEEDED(hr) && wcslen(szGotPath) > 0) {
                    targetPath = szGotPath;
                }
            }
            ppf->Release();
        }
        psl->Release();
    }
    return targetPath;
}

void ExecuteTarget(const std::wstring& path) {
    std::wstring finalPath = path;
    if (path.length() > 4 && _wcsicmp(path.c_str() + path.length() - 4, L".lnk") == 0) {
        finalPath = ResolveShortcut(path);
    }
    
    SHELLEXECUTEINFOW sei = {0};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_DEFAULT;
    sei.lpFile = finalPath.c_str();
    sei.nShow = SW_SHOWNORMAL;
    ShellExecuteExW(&sei);
}

void LoadFolderToolbars(HWND hRebar) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\EliteSoftware\\Win32Explorer\\Toolbars", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        wchar_t valueName[16384];
        DWORD cbValueName = 16384;
        wchar_t valueData[16384];
        DWORD cbValueData = 16384;
        DWORD dwType = 0;
        DWORD dwIndex = 0;
        
        while (RegEnumValueW(hKey, dwIndex, valueName, &cbValueName, NULL, &dwType, (LPBYTE)valueData, &cbValueData) == ERROR_SUCCESS) {
            if (dwType == REG_SZ) {
                FolderBand* band = new FolderBand();
                if (band->Initialize(hRebar, valueName, valueData)) {
                    REBARBANDINFOW rbbi = {0};
                    rbbi.cbSize = sizeof(rbbi);
                    rbbi.fMask = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_TEXT;
                    rbbi.fStyle = RBBS_GRIPPERALWAYS | RBBS_CHILDEDGE;
                    if (!band->showTitle) {
                        rbbi.fStyle |= RBBS_HIDETITLE;
                    }
                    rbbi.hwndChild = band->hToolbar;
                    rbbi.lpText = (LPWSTR)band->folderName.c_str();
                    
                    RECT rc;
                    GetWindowRect(band->hToolbar, &rc);
                    rbbi.cxMinChild = rc.right - rc.left;
                    rbbi.cyMinChild = rc.bottom - rc.top;
                    rbbi.cx = rbbi.cxMinChild + 40;

                    SendMessageW(hRebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbbi);
                    g_FolderBands.push_back(band);
                } else {
                    delete band;
                }
            }
            cbValueName = 16384;
            cbValueData = 16384;
            dwIndex++;
        }
        RegCloseKey(hKey);
    }
}

extern EliteTaskbarConfig g_Config;
static bool s_UseSecondaryTrayWndAsFallback = false;

#include <string>
#include <map>

extern std::wstring GetScrapedTrayTooltip(HWND hwnd, UINT uID);
extern std::vector<ScrapedTrayIcon> g_CurrentTrayIcons;

const IID MyIID_IShellItemImageFactory = { 0xbcc18b79, 0xba61, 0x4927, { 0xb5, 0x92, 0x6c, 0x4c, 0x7d, 0x07, 0xef, 0x3c } };
const IID MyIID_IPropertyStore = { 0x886d8eeb, 0x8cf2, 0x4446, { 0x8d, 0x02, 0xcd, 0xba, 0x1d, 0xbd, 0xcf, 0x99 } };
const IID MyIID_IShellItem = { 0x43826d1e, 0xe718, 0x42ee, { 0xbc, 0x55, 0xa1, 0xe2, 0x61, 0xc3, 0x7b, 0xfe } };
const PROPERTYKEY MyPKEY_AppUserModel_ID = { { 0x9f4c6855, 0x9979, 0x4ee3, { 0xa0, 0x8a, 0x31, 0xe3, 0xac, 0x3f, 0x00, 0xd7 } }, 5 };

struct EliteTrayIcon {
    HWND hWnd;
    UINT uID;
    UINT uCallbackMessage;
    HICON hIcon;
    WCHAR szTip[128];
    DWORD dwState;
    GUID guidItem; // - Draftsman-Dan
    bool bUseGUID; // - Draftsman-Dan
};



std::vector<EliteTrayIcon> g_TrayIcons;

// Helper to get DPI for a window - Builder-Bob
int GetDpiForWindowHelper(HWND hwnd) {
    UINT dpi = 96;
    HMODULE hShcore = LoadLibraryW(L"shcore.dll");
    if (hShcore) {
        typedef HRESULT(STDAPICALLTYPE* GetDpiForMonitorFn)(HMONITOR, int, UINT*, UINT*);
        GetDpiForMonitorFn fn = (GetDpiForMonitorFn)GetProcAddress(hShcore, "GetDpiForMonitor");
        if (fn) {
            HMONITOR hMon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
            UINT dpiX = 96, dpiY = 96;
            fn(hMon, 0, &dpiX, &dpiY);
            dpi = dpiX;
        }
        FreeLibrary(hShcore);
    } else {
        HDC hdc = GetDC(NULL);
        if (hdc) {
            dpi = GetDeviceCaps(hdc, LOGPIXELSX);
            ReleaseDC(NULL, hdc);
        }
    }
    return dpi;
}

// Calculate the number of icons that fit in the tray - Builder-Bob
int GetTrayVisibleLimit(HWND hwndTrayNotify, int dpi, int totalVisible) {
    bool bIsWin7Mode = (g_Config.OverflowMode == TrayOverflowMode::Win7Flyout);
    bool bIsExpanded = (GetPropW(hwndTrayNotify, L"TrayExpanded") != NULL);
    
    if (g_Config.ManualTrayWidth > 0) {
        int W_tray = MulDiv(g_Config.ManualTrayWidth, dpi, 96);
        if (g_Config.EnableTwoRowTray) {
            int colWidth = MulDiv(18, dpi, 96);
            int cols = W_tray / colWidth;
            if (cols < 1) cols = 1;
            return cols * 2;
        } else {
            int btnWidth = MulDiv(24, dpi, 96);
            int cols = W_tray / btnWidth;
            if (cols < 1) cols = 1;
            return cols;
        }
    } else {
        if (bIsWin7Mode) {
            return TRAY_LIMIT;
        } else {
            if (bIsExpanded) {
                return totalVisible;
            } else {
                return g_Config.EnableTwoRowTray ? 8 : 5;
            }
        }
    }
}

void UpdateTrayToolbarFromIndependent(TaskbarInstance* inst) {
    // Filter out non-existent windows/threads - Builder-Bob
    for (auto it = g_TrayIcons.begin(); it != g_TrayIcons.end(); ) {
        if (it->hWnd && !IsWindow(it->hWnd)) {
            if (it->hIcon) DestroyIcon(it->hIcon);
            it = g_TrayIcons.erase(it);
        } else {
            ++it;
        }
    }

    std::vector<ScrapedTrayIcon> icons;
    for (const auto& icon : g_TrayIcons) {
        if (icon.hIcon && !(icon.dwState & NIS_HIDDEN)) {
            ScrapedTrayIcon si;
            si.hwnd = icon.hWnd;
            si.uCallbackMessage = icon.uCallbackMessage;
            si.uID = icon.uID;
            si.hIcon = icon.hIcon;
            si.bOwnsIcon = false; // Managed by g_TrayIcons - Builder-Bob
            icons.push_back(si);
        }
    }
    UpdateTrayToolbar(inst->hToolbar, inst->hTrayImageList, icons);
}

void UpdateTaskbarLayout(TaskbarInstance* inst);

inline int GetTooltipLastIndex(HWND hwnd) {
    return (int)(intptr_t)GetPropW(hwnd, L"TooltipLastIndex") - 1;
}
inline void SetTooltipLastIndex(HWND hwnd, int idx) {
    SetPropW(hwnd, L"TooltipLastIndex", (HANDLE)(intptr_t)(idx + 1));
}
inline bool GetTooltipTracking(HWND hwnd) {
    return (GetPropW(hwnd, L"TooltipTracking") != NULL);
}
inline void SetTooltipTracking(HWND hwnd, bool tracking) {
    if (tracking) SetPropW(hwnd, L"TooltipTracking", (HANDLE)1);
    else RemovePropW(hwnd, L"TooltipTracking");
}

HWND GetOrCreateTrayTooltip(HWND hParent) {
    static HWND hTip = NULL;
    if (!hTip || !IsWindow(hTip)) {
        hTip = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, NULL,
                                WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                NULL, NULL, GetModuleHandle(NULL), NULL);
    }
    return hTip;
}

void UpdateTooltipText(HWND hTip, HWND hParent, const std::wstring& text, POINT ptScreen) {
    TOOLINFOW ti = {0};
    ti.cbSize = sizeof(ti);
    ti.uFlags = TTF_TRACK | TTF_ABSOLUTE;
    ti.hwnd = hParent;
    ti.uId = (UINT_PTR)hParent;
    ti.lpszText = (LPWSTR)text.c_str();

    SendMessageW(hTip, TTM_ADDTOOLW, 0, (LPARAM)&ti);
    SendMessageW(hTip, TTM_UPDATETIPTEXTW, 0, (LPARAM)&ti);
    SendMessageW(hTip, TTM_TRACKPOSITION, 0, MAKELPARAM(ptScreen.x, ptScreen.y - 24));
    SendMessageW(hTip, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);
}

void HideTooltip(HWND hTip, HWND hParent) {
    if (hTip) {
        TOOLINFOW ti = {0};
        ti.cbSize = sizeof(ti);
        ti.hwnd = hParent;
        ti.uId = (UINT_PTR)hParent;
        SendMessageW(hTip, TTM_TRACKACTIVATE, FALSE, (LPARAM)&ti);
        SendMessageW(hTip, TTM_DELTOOLW, 0, (LPARAM)&ti);
    }
}

LRESULT CALLBACK TrayToolbarSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    // Forward mouse events in left 0-5px margin to parent hTrayNotify - Draftsman-Dan
    if (uMsg == WM_SETCURSOR) {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hWnd, &pt);
        if (pt.x >= 0 && pt.x <= 5) {
            HWND hParent = GetParent(hWnd);
            return SendMessageW(hParent, uMsg, wParam, lParam);
        }
    }
    if (uMsg == WM_MOUSEMOVE || uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP ||
        uMsg == WM_LBUTTONDBLCLK || uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONUP ||
        uMsg == WM_RBUTTONDBLCLK || uMsg == WM_MBUTTONDOWN || uMsg == WM_MBUTTONUP ||
        uMsg == WM_MBUTTONDBLCLK)
    {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        if (x >= 0 && x <= 5) {
            POINT pt = { x, y };
            ClientToScreen(hWnd, &pt);
            HWND hParent = GetParent(hWnd);
            ScreenToClient(hParent, &pt);
            return SendMessageW(hParent, uMsg, wParam, MAKELPARAM(pt.x, pt.y));
        }
    }

    switch (uMsg) {
    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hWnd, &rc);
        DrawThemeParentBackground(hWnd, hdc, &rc);
        return TRUE;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        if (hdc) {
            RECT rc;
            GetClientRect(hWnd, &rc);
            DrawThemeParentBackground(hWnd, hdc, &rc);
            SendMessageW(hWnd, WM_PRINTCLIENT, (WPARAM)hdc, PRF_CLIENT);
        }
        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_PRINTCLIENT: {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hWnd, &rc);
        DrawThemeParentBackground(hWnd, hdc, &rc);
        break; // Let default procedure draw buttons/icons - Builder-Bob
    }
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MBUTTONDBLCLK: {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        int index = (int)SendMessageW(hWnd, TB_HITTEST, 0, (LPARAM)&pt);
        if (index >= 0 && index < (int)g_CurrentTrayIcons.size()) {
            const auto& icon = g_CurrentTrayIcons[index];
            TaskbarInstance* inst = (TaskbarInstance*)dwRefData;
            extern void StartNativeTaskbarSpoof(HWND hClickedTaskbar);
            if (inst) {
                StartNativeTaskbarSpoof(inst->hTaskbar);
            }
            
            bool isClick = (uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP || uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONUP || uMsg == WM_LBUTTONDBLCLK || uMsg == WM_RBUTTONDBLCLK);
            HWND hShellTrayWnd = NULL;
            RECT rcOriginal = { 0 };
            bool shifted = false;
            
            if (isClick) {
                hShellTrayWnd = FindWindowW(L"Shell_TrayWnd", NULL);
                if (hShellTrayWnd) {
                    HMONITOR hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONULL);
                    if (hMon) {
                        SendMessageW(hShellTrayWnd, WM_SETREDRAW, FALSE, 0);
                        GetWindowRect(hShellTrayWnd, &rcOriginal);
                        POINT ptScreen = pt;
                        ClientToScreen(hWnd, &ptScreen);
                        SetWindowPos(hShellTrayWnd, NULL, ptScreen.x - 16, rcOriginal.top, rcOriginal.right - rcOriginal.left, rcOriginal.bottom - rcOriginal.top, SWP_NOZORDER | SWP_NOACTIVATE);
                        shifted = true;
                    }
                }
            }
            
            PostMessageW(icon.hwnd, icon.uCallbackMessage, icon.uID, uMsg);
            
            if (shifted) {
                Sleep(50);
                SetWindowPos(hShellTrayWnd, NULL, rcOriginal.left, rcOriginal.top, rcOriginal.right - rcOriginal.left, rcOriginal.bottom - rcOriginal.top, SWP_NOZORDER | SWP_NOACTIVATE);
                SendMessageW(hShellTrayWnd, WM_SETREDRAW, TRUE, 0);
                RedrawWindow(hShellTrayWnd, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
            }
        }
        break;
    }
    case WM_MOUSEMOVE: {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        int index = (int)SendMessageW(hWnd, TB_HITTEST, 0, (LPARAM)&pt);
        
        bool bTracking = GetTooltipTracking(hWnd);
        if (!bTracking) {
            TRACKMOUSEEVENT tme = {0};
            tme.cbSize = sizeof(tme);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hWnd;
            TrackMouseEvent(&tme);
            SetTooltipTracking(hWnd, true);
        }

        int lastIndex = GetTooltipLastIndex(hWnd);
        if (index != lastIndex) {
            SetTooltipLastIndex(hWnd, index);
            HWND hTip = GetOrCreateTrayTooltip(hWnd);
            if (index >= 0 && index < (int)g_CurrentTrayIcons.size()) {
                const auto& icon = g_CurrentTrayIcons[index];
                std::wstring tip = GetScrapedTrayTooltip(icon.hwnd, icon.uID);
                if (!tip.empty()) {
                    POINT ptScreen = pt;
                    ClientToScreen(hWnd, &ptScreen);
                    UpdateTooltipText(hTip, hWnd, tip, ptScreen);
                } else {
                    HideTooltip(hTip, hWnd);
                }
            } else {
                HideTooltip(hTip, hWnd);
            }
        } else if (index >= 0) {
            HWND hTip = GetOrCreateTrayTooltip(hWnd);
            POINT ptScreen = pt;
            ClientToScreen(hWnd, &ptScreen);
            SendMessageW(hTip, TTM_TRACKPOSITION, 0, MAKELPARAM(ptScreen.x, ptScreen.y - 24));
        }
        
        if (index >= 0 && index < (int)g_CurrentTrayIcons.size()) {
            const auto& icon = g_CurrentTrayIcons[index];
            PostMessageW(icon.hwnd, icon.uCallbackMessage, icon.uID, uMsg);
        }
        break;
    }
    case WM_MOUSELEAVE: {
        SetTooltipTracking(hWnd, false);
        SetTooltipLastIndex(hWnd, -1);
        HWND hTip = GetOrCreateTrayTooltip(hWnd);
        HideTooltip(hTip, hWnd);
        break;
    }
    case WM_DESTROY: {
        RemoveWindowSubclass(hWnd, TrayToolbarSubclassProc, uIdSubclass);
        break;
    }
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

std::wstring GetIndependentTrayTooltip(HWND hwndIcon, UINT uID) {
    for (const auto& icon : g_TrayIcons) {
        if (icon.hWnd == hwndIcon && icon.uID == uID) {
            return icon.szTip;
        }
    }
    return L"";
}

LRESULT CALLBACK SysPagerSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    if (uMsg == WM_NOTIFY) {
        LPNMHDR lpnmhdr = (LPNMHDR)lParam;
        if (lpnmhdr->code == TTN_GETDISPINFOW) {
            LPNMTTDISPINFOW lpnmt = (LPNMTTDISPINFOW)lParam;
            int idx = (int)lpnmt->hdr.idFrom;
            if (idx >= 0 && idx < (int)g_CurrentTrayIcons.size()) {
                const auto& icon = g_CurrentTrayIcons[idx];
                std::wstring tipText;
                if (g_Config.Mode == TaskbarMode::Independent) {
                    tipText = GetIndependentTrayTooltip(icon.hwnd, icon.uID);
                } else {
                    tipText = GetScrapedTrayTooltip(icon.hwnd, icon.uID);
                }
                if (!tipText.empty()) {
                    wcsncpy_s(lpnmt->szText, tipText.c_str(), _TRUNCATE);
                } else {
                    lpnmt->szText[0] = L'\0';
                }
                return 0;
            }
        }
    }
    if (uMsg == WM_ERASEBKGND) {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hWnd, &rc);
        DrawThemeParentBackground(hWnd, hdc, &rc);
        return TRUE;
    }
    if (uMsg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        if (hdc) {
            RECT rc;
            GetClientRect(hWnd, &rc);
            DrawThemeParentBackground(hWnd, hdc, &rc);
        }
        EndPaint(hWnd, &ps);
        return 0;
    }
    if (uMsg == WM_PRINTCLIENT) {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hWnd, &rc);
        DrawThemeParentBackground(hWnd, hdc, &rc);
        return 0;
    }
    if (uMsg == WM_DESTROY) {
        RemoveWindowSubclass(hWnd, SysPagerSubclassProc, uIdSubclass);
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

HICON GetWindowIconFix(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) return NULL;

    WCHAR szClass[256] = {0};
    GetClassNameW(hwnd, szClass, 256);
    if (wcscmp(szClass, L"ApplicationFrameWindow") == 0) {
        HMODULE hShell32 = GetModuleHandleW(L"shell32.dll");
        if (hShell32) {
            typedef HRESULT(STDAPICALLTYPE* SHGetPropertyStoreForWindowFn)(HWND, REFIID, void**);
            SHGetPropertyStoreForWindowFn fnGetProp = (SHGetPropertyStoreForWindowFn)GetProcAddress(hShell32, "SHGetPropertyStoreForWindow");
            if (fnGetProp) {
                IPropertyStore* pPropStore = NULL;
                HRESULT hr = fnGetProp(hwnd, MyIID_IPropertyStore, (void**)&pPropStore);
                if (SUCCEEDED(hr) && pPropStore) {
                    PROPVARIANT pv;
                    PropVariantInit(&pv);
                    hr = pPropStore->GetValue(MyPKEY_AppUserModel_ID, &pv);
                    if (SUCCEEDED(hr) && pv.vt == VT_LPWSTR && pv.pwszVal) {
                        typedef HRESULT(STDAPICALLTYPE* SHCreateItemFromParsingNameFn)(PCWSTR, IBindCtx*, REFIID, void**);
                        SHCreateItemFromParsingNameFn fnCreateItem = (SHCreateItemFromParsingNameFn)GetProcAddress(hShell32, "SHCreateItemFromParsingName");
                        if (fnCreateItem) {
                            std::wstring shellPath = L"shell:AppsFolder\\" + std::wstring(pv.pwszVal);
                            IShellItem* pShellItem = NULL;
                            hr = fnCreateItem(shellPath.c_str(), NULL, MyIID_IShellItem, (void**)&pShellItem);
                            if (SUCCEEDED(hr) && pShellItem) {
                                IShellItemImageFactory* pImgFactory = NULL;
                                hr = pShellItem->QueryInterface(MyIID_IShellItemImageFactory, (void**)&pImgFactory);
                                if (SUCCEEDED(hr) && pImgFactory) {
                                    SIZE szIcon = { 16, 16 };
                                    HBITMAP hBitmap = NULL;
                                    hr = pImgFactory->GetImage(szIcon, SIIGBF_ICONONLY, &hBitmap);
                                    if (SUCCEEDED(hr) && hBitmap) {
                                        HBITMAP hbmMask = CreateBitmap(16, 16, 1, 1, NULL);
                                        ICONINFO ii = {0};
                                        ii.fIcon = TRUE;
                                        ii.hbmMask = hbmMask;
                                        ii.hbmColor = hBitmap;
                                        HICON hIcon = CreateIconIndirect(&ii);
                                        DeleteObject(hbmMask);
                                        DeleteObject(hBitmap);
                                        pImgFactory->Release();
                                        pShellItem->Release();
                                        PropVariantClear(&pv);
                                        pPropStore->Release();
                                        if (hIcon) return hIcon;
                                    }
                                    pImgFactory->Release();
                                }
                                pShellItem->Release();
                            }
                        }
                    }
                    PropVariantClear(&pv);
                    pPropStore->Release();
                }
            }
        }
        
        struct EnumData {
            HWND hwndTarget;
            DWORD pid;
        };
        EnumData data = { NULL, 0 };
        EnumChildWindows(hwnd, [](HWND hChild, LPARAM lParam) -> BOOL {
            WCHAR szChildClass[256] = {0};
            GetClassNameW(hChild, szChildClass, 256);
            if (wcscmp(szChildClass, L"Windows.UI.Core.CoreWindow") == 0) {
                EnumData* pData = (EnumData*)lParam;
                pData->hwndTarget = hChild;
                GetWindowThreadProcessId(hChild, &pData->pid);
                return FALSE;
            }
            return TRUE;
        }, (LPARAM)&data);

        if (data.pid) {
            HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, data.pid);
            if (hProc) {
                WCHAR exePath[MAX_PATH] = {0};
                DWORD dwSize = MAX_PATH;
                if (QueryFullProcessImageNameW(hProc, 0, exePath, &dwSize)) {
                    SHFILEINFOW sfi = {0};
                    if (SHGetFileInfoW(exePath, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_SMALLICON)) {
                        CloseHandle(hProc);
                        if (sfi.hIcon) return sfi.hIcon;
                    }
                }
                CloseHandle(hProc);
            }
        }
    }

    DWORD_PTR dwRes = 0;
    if (SendMessageTimeoutW(hwnd, WM_GETICON, ICON_SMALL, 0, SMTO_ABORTIFHUNG, 500, &dwRes) && dwRes) {
        return (HICON)dwRes;
    }
    HICON hClassIcon = (HICON)GetClassLongPtrW(hwnd, GCLP_HICONSM);
    if (hClassIcon) return hClassIcon;

    return NULL;
}

void UpdateTaskbarLayout(TaskbarInstance* inst) {
    if (!inst || !inst->hTaskbar) return;

    RECT rcClient;
    GetClientRect(inst->hTaskbar, &rcClient);
    int taskbarWidth = rcClient.right - rcClient.left;
    int taskbarHeight = rcClient.bottom - rcClient.top;

    UINT dpi = 96;
    HMODULE hShcore = LoadLibraryW(L"shcore.dll");
    if (hShcore) {
        typedef HRESULT(STDAPICALLTYPE* GetDpiForMonitorFn)(HMONITOR, int, UINT*, UINT*);
        GetDpiForMonitorFn fn = (GetDpiForMonitorFn)GetProcAddress(hShcore, "GetDpiForMonitor");
        if (fn) {
            UINT dpiX = 96, dpiY = 96;
            fn(inst->hMonitor, 0, &dpiX, &dpiY);
            dpi = dpiX;
        }
        FreeLibrary(hShcore);
    } else {
        HDC hdc = GetDC(NULL);
        if (hdc) {
            dpi = GetDeviceCaps(hdc, LOGPIXELSX);
            ReleaseDC(NULL, hdc);
        }
    }

    int W_clock = MulDiv(85, dpi, 96);
    if (g_Config.DynamicClockWidth) { // - Draftsman-Dan
        SYSTEMTIME st;
        GetLocalTime(&st);
        wchar_t timeBuf[32];
        wchar_t dateBuf[32];
        GetTimeFormatW(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, timeBuf, 32);
        GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, dateBuf, 32);
        wchar_t clockText[128];
        swprintf_s(clockText, L"%s\n%s", timeBuf, dateBuf);

        HDC hdc = GetDC(inst->hTaskbar);
        if (hdc) {
            LOGFONTW lf = {0};
            lf.lfHeight = MulDiv(-11, dpi, 96);
            lf.lfWeight = FW_NORMAL;
            wcscpy_s(lf.lfFaceName, L"Segoe UI");
            HFONT hFont = CreateFontIndirectW(&lf);
            HFONT hOldFont = NULL;
            if (hFont) hOldFont = (HFONT)SelectObject(hdc, hFont);

            RECT rcCalc = {0};
            DrawTextW(hdc, clockText, -1, &rcCalc, DT_CALCRECT);
            int textWidth = rcCalc.right - rcCalc.left;
            W_clock = textWidth + MulDiv(12, dpi, 96); // tight padding 6px per side

            if (hOldFont) SelectObject(hdc, hOldFont);
            if (hFont) DeleteObject(hFont);
            ReleaseDC(inst->hTaskbar, hdc);
        }
    }
    int W_showDesktop = MulDiv(15, dpi, 96);

    HWND hShowDesktop = FindWindowExW(inst->hTaskbar, NULL, L"TrayShowDesktopButtonWClass", NULL);
    if (hShowDesktop) {
        SetWindowPos(hShowDesktop, NULL, taskbarWidth - W_showDesktop, 0, W_showDesktop, taskbarHeight, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    int W_tray = 100;
    int W_notify = 240;

    if (inst->hTrayNotify) {
        bool enableClock = (inst->hTrayClock != NULL);
        bool enableTray = (inst->hToolbar != NULL);

        int totalVisible = (int)g_CurrentTrayIcons.size();
        if (g_Config.Mode == TaskbarMode::Independent) {
            totalVisible = 0;
            for (const auto& icon : g_TrayIcons) {
                if (icon.hIcon && !(icon.dwState & NIS_HIDDEN)) totalVisible++;
            }
        }

        int limit = GetTrayVisibleLimit(inst->hTrayNotify, dpi, totalVisible);
        bool hasOverflow = (totalVisible > limit);

        if (g_Config.ManualTrayWidth > 0) {
            W_tray = MulDiv(g_Config.ManualTrayWidth, dpi, 96);
        } else if (inst->hToolbar && enableTray) { // Compute tray layout width dynamically
            int btnCount = (int)SendMessageW(inst->hToolbar, TB_BUTTONCOUNT, 0, 0);
            if (btnCount > 0) {
                // First, enforce the visibility limit on the buttons
                for (int idx = 0; idx < btnCount; ++idx) {
                    LRESULT state = SendMessageW(inst->hToolbar, TB_GETSTATE, idx, 0);
                    if (totalVisible > limit && idx < (totalVisible - limit)) {
                        state |= TBSTATE_HIDDEN;
                    } else {
                        state &= ~TBSTATE_HIDDEN;
                    }
                    SendMessageW(inst->hToolbar, TB_SETSTATE, idx, state);
                }

                if (g_Config.EnableTwoRowTray) {
                    int visCount = 0;
                    for (int idx = 0; idx < btnCount; ++idx) {
                        if (!(SendMessageW(inst->hToolbar, TB_GETSTATE, idx, 0) & TBSTATE_HIDDEN)) visCount++;
                    }
                    int colCount = (visCount + 1) / 2;
                    W_tray = colCount * MulDiv(15, dpi, 96) + 4; // 15px per column in two-row mode
                } else {
                    SendMessageW(inst->hToolbar, TB_AUTOSIZE, 0, 0);
                    int maxWidth = 0;
                    for (int idx = 0; idx < btnCount; ++idx) {
                        if (SendMessageW(inst->hToolbar, TB_GETSTATE, idx, 0) & TBSTATE_HIDDEN) continue;
                        RECT rcItem = { 0 };
                        if (SendMessageW(inst->hToolbar, TB_GETITEMRECT, idx, (LPARAM)&rcItem)) {
                            if (rcItem.right > maxWidth) {
                                maxWidth = rcItem.right;
                            }
                        }
                    }
                    if (maxWidth > 0) {
                        W_tray = maxWidth + 4;
                    } else {
                        W_tray = 0;
                    }
                }
            } else {
                W_tray = 0;
            }
        } else {
            W_tray = 0;
        }

        int W_overflowBtn = MulDiv(18, dpi, 96);
        W_notify = W_tray + (hasOverflow ? W_overflowBtn : 0) + (enableClock ? W_clock : 0);

        int xNotify = taskbarWidth - W_showDesktop - W_notify;
        SetWindowPos(inst->hTrayNotify, NULL, xNotify, 0, W_notify, taskbarHeight, SWP_NOZORDER | SWP_NOACTIVATE);

        if (enableTray && inst->hToolbar) {
            int toolbarX = hasOverflow ? W_overflowBtn : 0;
            if (g_Config.EnableTwoRowTray) {
                int tbHeight = MulDiv(26, dpi, 96);
                int tbY = (taskbarHeight - tbHeight) / 2;
                SetWindowPos(inst->hToolbar, NULL, toolbarX, tbY, W_tray, tbHeight, SWP_NOZORDER | SWP_NOACTIVATE);
            } else {
                SetWindowPos(inst->hToolbar, NULL, toolbarX, 0, W_tray, taskbarHeight, SWP_NOZORDER | SWP_NOACTIVATE);
            }
        }
        if (enableClock && inst->hTrayClock) {
            int clockX = (hasOverflow ? W_overflowBtn : 0) + W_tray;
            SetWindowPos(inst->hTrayClock, NULL, clockX, 0, W_clock, taskbarHeight, SWP_NOZORDER | SWP_NOACTIVATE);
        }
        
        InvalidateRect(inst->hTrayNotify, NULL, TRUE);
    } else {
        W_notify = 0;
    }

    if (inst->hTaskSwitch) {
        HWND hOrb = inst->startButton ? inst->startButton->GetHwnd() : NULL;
        int startButtonWidth = MulDiv(60, dpi, 96);
        if (hOrb && IsWindow(hOrb)) {
            RECT rcOrb;
            GetWindowRect(hOrb, &rcOrb);
            startButtonWidth = rcOrb.right - rcOrb.left;
        }
        
        int xTaskSwitch = startButtonWidth + MulDiv(6, dpi, 96);
        int xNotifyStart = taskbarWidth - W_showDesktop - W_notify - MulDiv(10, dpi, 96);
        int widthTaskSwitch = xNotifyStart - xTaskSwitch;
        if (widthTaskSwitch < 0) widthTaskSwitch = 0;

        // Position Rebar control - Builder-Bob
        if (inst->hRebar && SendMessageW(inst->hRebar, RB_GETBANDCOUNT, 0, 0) > 0) {
            int xRebar = startButtonWidth + MulDiv(6, dpi, 96);
            int wRebar = xNotifyStart - xRebar;
            int rebarWidth = MulDiv(150, dpi, 96);
            if (widthTaskSwitch > rebarWidth + MulDiv(20, dpi, 96)) {
                widthTaskSwitch -= rebarWidth;
                xRebar = xTaskSwitch + widthTaskSwitch + MulDiv(6, dpi, 96);
                wRebar = rebarWidth - MulDiv(6, dpi, 96);
            } else {
                wRebar = 0;
            }
            if (wRebar > 0) {
                SetWindowPos(inst->hRebar, NULL, xRebar, 0, wRebar, taskbarHeight, SWP_NOZORDER | SWP_NOACTIVATE);
                ShowWindow(inst->hRebar, SW_SHOW);
            } else {
                ShowWindow(inst->hRebar, SW_HIDE);
            }
        } else if (inst->hRebar) {
            ShowWindow(inst->hRebar, SW_HIDE);
        }

        int switchHeight = taskbarHeight;
        int switchY = 0;
        if (inst->hTaskSwitch) {
            DWORD dwBtnSize = (DWORD)SendMessageW(inst->hTaskSwitch, TB_GETBUTTONSIZE, 0, 0);
            int btnHeight = HIWORD(dwBtnSize);
            if (btnHeight > 0 && btnHeight < taskbarHeight) {
                switchHeight = btnHeight;
                switchY = (taskbarHeight - btnHeight) / 2;
            }
        } // - Draftsman-Dan
        SetWindowPos(inst->hTaskSwitch, NULL, xTaskSwitch, switchY, widthTaskSwitch, switchHeight, SWP_NOZORDER | SWP_NOACTIVATE);
        SendMessageW(inst->hTaskSwitch, TB_AUTOSIZE, 0, 0);
    } else {
        // If taskband is disabled, the Rebar can span the whole client space - Builder-Bob
        HWND hOrb = inst->startButton ? inst->startButton->GetHwnd() : NULL;
        int startButtonWidth = MulDiv(60, dpi, 96);
        if (hOrb && IsWindow(hOrb)) {
            RECT rcOrb;
            GetWindowRect(hOrb, &rcOrb);
            startButtonWidth = rcOrb.right - rcOrb.left;
        }
        int xNotifyStart = taskbarWidth - W_showDesktop - W_notify - MulDiv(10, dpi, 96);
        if (inst->hRebar && SendMessageW(inst->hRebar, RB_GETBANDCOUNT, 0, 0) > 0) {
            int xRebar = startButtonWidth + MulDiv(6, dpi, 96);
            int wRebar = xNotifyStart - xRebar;
            if (wRebar > 0) {
                SetWindowPos(inst->hRebar, NULL, xRebar, 0, wRebar, taskbarHeight, SWP_NOZORDER | SWP_NOACTIVATE);
                ShowWindow(inst->hRebar, SW_SHOW);
            } else {
                ShowWindow(inst->hRebar, SW_HIDE);
            }
        }
    }
}

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
bool g_IsSpoofingNativeTaskbar = false;
DWORD g_SpoofStartTime = 0;
std::vector<TaskbarInstance*> g_Taskbars;

// - Draftsman-Dan
bool IsTaskbarLocked() {
    bool isLocked = true;
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD dwValue = 0;
        DWORD cbData = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"TaskbarSizeMove", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
            isLocked = (dwValue == 0);
        }
        RegCloseKey(hKey);
    }
    return isLocked;
}

// - Draftsman-Dan
void TerminateProcessByName(const wchar_t* processName) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe = { sizeof(PROCESSENTRY32W) };
        if (Process32FirstW(hSnapshot, &pe)) {
            do {
                if (_wcsicmp(pe.szExeFile, processName) == 0) {
                    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                    if (hProcess) {
                        TerminateProcess(hProcess, 0);
                        CloseHandle(hProcess);
                    }
                }
            } while (Process32NextW(hSnapshot, &pe));
        }
        CloseHandle(hSnapshot);
    }
}

// - Draftsman-Dan
void RestartExplorerShell() {
    TerminateProcessByName(L"explorer.exe");
    Sleep(1000);
    wchar_t windir[MAX_PATH];
    GetWindowsDirectoryW(windir, MAX_PATH);
    std::wstring explorerPath = std::wstring(windir) + L"\\explorer.exe";
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = {0};
    wchar_t cmdLine[MAX_PATH];
    wcscpy_s(cmdLine, explorerPath.c_str());
    if (CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

// - Draftsman-Dan
void RestartEliteTaskbar() {
    wchar_t modulePath[MAX_PATH];
    GetModuleFileNameW(NULL, modulePath, MAX_PATH);
    DWORD currentPid = GetCurrentProcessId();
    wchar_t cmdLine[MAX_PATH * 2];
    wsprintfW(cmdLine, L"\"%s\" -restartPid %u", modulePath, currentPid);
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = {0};
    if (CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    for (auto* tb : g_Taskbars) {
        if (tb && IsWindow(tb->hTaskbar)) {
            PostMessageW(tb->hTaskbar, WM_CLOSE, 0, 0);
        }
    }
    PostQuitMessage(0);
}

void StartNativeTaskbarSpoof(HWND hClickedTaskbar) {
    if (g_hNativeTaskbar && IsWindow(g_hNativeTaskbar)) {
        g_IsSpoofingNativeTaskbar = true;
        g_SpoofStartTime = GetTickCount();
        RECT rc;
        GetWindowRect(hClickedTaskbar, &rc);
        POINT pt;
        GetCursorPos(&pt);
        int targetX = rc.left;
        HWND hNotify = FindWindowExW(g_hNativeTaskbar, NULL, L"TrayNotifyWnd", NULL);
        if (hNotify) {
            RECT rcNotify, rcNative;
            GetWindowRect(hNotify, &rcNotify);
            GetWindowRect(g_hNativeTaskbar, &rcNative);
            int offset = rcNotify.left - rcNative.left;
            targetX = pt.x - offset - ((rcNotify.right - rcNotify.left) / 2);
        } else {
            targetX = pt.x - (rc.right - rc.left);
        }
        SetWindowPos(g_hNativeTaskbar, HWND_TOPMOST, targetX, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOACTIVATE);
        ShowWindow(g_hNativeTaskbar, SW_SHOWNOACTIVATE);
    }
}

TaskbarInstance* GetTaskbarInstance(HWND hwnd) {
    if (!hwnd) return nullptr;
    // Check if hwnd is a taskbar
    for (auto* tb : g_Taskbars) {
        if (tb->hTaskbar == hwnd || tb->hTrayNotify == hwnd || tb->hTrayClock == hwnd || tb->hToolbar == hwnd) {
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
#define TRAY_CLASS_NAME L"Shell_SecondaryTrayWnd" // - Draftsman-Dan
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
#define IDM_RELOAD_TASKBAR          3015

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
                UINT dpi = 96;
                HMONITOR hMon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
                HMODULE hShcore = LoadLibraryW(L"shcore.dll");
                if (hShcore) {
                    typedef HRESULT(STDAPICALLTYPE* GetDpiForMonitorFn)(HMONITOR, int, UINT*, UINT*);
                    GetDpiForMonitorFn fn = (GetDpiForMonitorFn)GetProcAddress(hShcore, "GetDpiForMonitor");
                    if (fn) {
                        UINT dpiX = 96, dpiY = 96;
                        fn(hMon, 0, &dpiX, &dpiY);
                        dpi = dpiX;
                    }
                    FreeLibrary(hShcore);
                } else {
                    HDC hdcScr = GetDC(NULL);
                    if (hdcScr) {
                        dpi = GetDeviceCaps(hdcScr, LOGPIXELSX);
                        ReleaseDC(NULL, hdcScr);
                    }
                }
                lf.lfHeight = MulDiv(-12, dpi, 96);
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
                    info.hIcon = NULL;
                    
                    WCHAR szTitle[256] = {0};
                    GetWindowTextW(hwnd, szTitle, 256);
                    info.title = szTitle;
                    
                    bool found = false;
                    for (auto& tb : g_TaskButtons) {
                        if (tb.hwnd == hwnd) {
                            tb.title = szTitle;
                            tb.isActive = info.isActive;
                            AddTaskButton(tb);
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        info.hIcon = GetWindowIconFix(hwnd);
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
        HTHEME hTheme = OpenThemeData(hwnd, L"Taskbar");
        if (hTheme) {
            DrawThemeBackground(hTheme, hdc, 1 /*TBP_BACKGROUNDBOTTOM*/, 0, &rcClient, NULL);
            CloseThemeData(hTheme);
        } else {
            HBRUSH hbr = CreateSolidBrush(RGB(240, 240, 240));
            FillRect(hdc, &rcClient, hbr);
            DeleteObject(hbr);
        }
        
        // Draw 3D Edge
        DrawEdge(hdc, &rcClient, EDGE_RAISED, BF_RECT);
        int x = 4, y = 4;
        int totalVisible = 0;
        for (const auto& icon : g_TrayIcons) {
            if (icon.hIcon && !(icon.dwState & NIS_HIDDEN)) totalVisible++;
        }
        
        int drawn = 0;
        for (const auto& icon : g_TrayIcons) {
            if (icon.hIcon && !(icon.dwState & NIS_HIDDEN)) {
                if (drawn < totalVisible - TRAY_LIMIT) {
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
                if (drawn < totalVisible - TRAY_LIMIT) {
                    if (drawn == clickIndex) {
                        bool isClick = (uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP || uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONUP || uMsg == WM_LBUTTONDBLCLK || uMsg == WM_RBUTTONDBLCLK);
                        HWND hShellTrayWnd = NULL;
                        RECT rcOriginal = { 0 };
                        bool shifted = false;
                        
                        if (isClick) {
                            extern void StartNativeTaskbarSpoof(HWND hClickedTaskbar);
                            StartNativeTaskbarSpoof(GetParent(hwnd));
                            
                            hShellTrayWnd = FindWindowW(L"Shell_TrayWnd", NULL);
                            if (hShellTrayWnd) {
                                HMONITOR hMon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONULL);
                                HMONITOR hPrimaryMon = MonitorFromWindow(hShellTrayWnd, MONITOR_DEFAULTTOPRIMARY);
                                if (hMon) {
                                    SendMessageW(hShellTrayWnd, WM_SETREDRAW, FALSE, 0);
                                    GetWindowRect(hShellTrayWnd, &rcOriginal);
                                    POINT ptScreen = { xPos, yPos };
                                    ClientToScreen(hwnd, &ptScreen);
                                    SetWindowPos(hShellTrayWnd, NULL, ptScreen.x - 16, rcOriginal.top, rcOriginal.right - rcOriginal.left, rcOriginal.bottom - rcOriginal.top, SWP_NOZORDER | SWP_NOACTIVATE);
                                    shifted = true;
                                }
                            }
                        }
                        
                        PostMessageW(icon.hWnd, icon.uCallbackMessage, icon.uID, uMsg);
                        
                        if (shifted) {
                            Sleep(50);
                            SetWindowPos(hShellTrayWnd, NULL, rcOriginal.left, rcOriginal.top, rcOriginal.right - rcOriginal.left, rcOriginal.bottom - rcOriginal.top, SWP_NOZORDER | SWP_NOACTIVATE);
                            SendMessageW(hShellTrayWnd, WM_SETREDRAW, TRUE, 0);
                            RedrawWindow(hShellTrayWnd, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
                        }
                        
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
    static bool s_bResizingTray = false;
    static int s_dragStartScreenX = 0;
    static int s_dragStartWTray = 0;

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

        int dpi = GetDpiForWindowHelper(hwnd);
        int totalVisible = (int)g_CurrentTrayIcons.size();
        if (g_Config.Mode == TaskbarMode::Independent) {
            totalVisible = 0;
            for (const auto& icon : g_TrayIcons) {
                if (icon.hIcon && !(icon.dwState & NIS_HIDDEN)) totalVisible++;
            }
        }
        int limit = GetTrayVisibleLimit(hwnd, dpi, totalVisible);
        if (totalVisible > limit) {
            RECT rcBtn = { 0, 0, MulDiv(18, dpi, 96), rcClient.bottom };
            
            // Draw transparently - don't use solid DrawFrameControl
            DrawThemeParentBackground(hwnd, hdc, &rcBtn);
            
            // Optional: Draw a subtle toolbar-style hover effect if needed, but transparent is safer.

            bool bIsWin7Mode = (g_Config.OverflowMode == TrayOverflowMode::Win7Flyout);
            bool bIsExpanded = (GetPropW(hwnd, L"TrayExpanded") != NULL);
            LPCWSTR arrowText = L""; // - Draftsman-Dan
            if (g_Config.HorizontalTrayChevron) {
                arrowText = bIsExpanded ? L">" : L"<";
            } else if (!bIsWin7Mode) {
                arrowText = bIsExpanded ? L">" : L"<";
            }

            SetTextColor(hdc, RGB(255, 255, 255)); // White chevron on the dark/styled taskbar
            SetBkMode(hdc, TRANSPARENT);

            HFONT hFont = CreateFontW(
                -MulDiv(10, dpi, 96), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
            );
            HFONT hOldFont = NULL;
            if (hFont) hOldFont = (HFONT)SelectObject(hdc, hFont);

            DrawTextW(hdc, arrowText, -1, &rcBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            if (hOldFont) SelectObject(hdc, hOldFont);
            if (hFont) DeleteObject(hFont);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_COPYDATA: {
        COPYDATASTRUCT* pcds = (COPYDATASTRUCT*)lParam;
        if ((pcds->dwData == 1 || pcds->dwData == 0x34753423) && pcds->cbData >= 8) { // Tray notification
            TRAYDATA* pTrayData = (TRAYDATA*)pcds->lpData;
            if (pTrayData->dwSignature == 0x34753423) {
                DWORD dwMessage = pTrayData->dwMessage;
                NOTIFYICONDATAW* nid = (NOTIFYICONDATAW*)((BYTE*)pTrayData + 8);
                bool updated = false;

                if (dwMessage == NIM_ADD || dwMessage == NIM_MODIFY) {
                    bool found = false;
                    bool nidUseGUID = ((nid->uFlags & NIF_GUID) != 0);
                    for (auto& icon : g_TrayIcons) {
                        bool match = false;
                        if (nidUseGUID && icon.bUseGUID) {
                            match = (icon.guidItem == nid->guidItem);
                        } else {
                            match = (icon.hWnd == nid->hWnd && icon.uID == nid->uID);
                        }
                        if (match) {
                            if (nid->uFlags & NIF_ICON) {
                                if (icon.hIcon) DestroyIcon(icon.hIcon);
                                icon.hIcon = CopyIcon(nid->hIcon);
                            }
                            if (nid->uFlags & NIF_TIP) wcscpy_s(icon.szTip, nid->szTip);
                            if (nid->uFlags & NIF_MESSAGE) icon.uCallbackMessage = nid->uCallbackMessage;
                            if (nid->uFlags & NIF_STATE) icon.dwState = nid->dwState;
                            if (nidUseGUID) {
                                icon.guidItem = nid->guidItem;
                                icon.bUseGUID = true;
                            }
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
                        if (nidUseGUID) {
                            newIcon.guidItem = nid->guidItem;
                            newIcon.bUseGUID = true;
                        }
                        g_TrayIcons.push_back(newIcon);
                    }
                    updated = true;
                    InvalidateRect(hwnd, NULL, FALSE);
                }
                else if (dwMessage == NIM_DELETE) {
                    bool nidUseGUID = ((nid->uFlags & NIF_GUID) != 0);
                    for (auto it = g_TrayIcons.begin(); it != g_TrayIcons.end(); ++it) {
                        bool match = false;
                        if (nidUseGUID && it->bUseGUID) {
                            match = (it->guidItem == nid->guidItem);
                        } else {
                            match = (it->hWnd == nid->hWnd && it->uID == nid->uID);
                        }
                        if (match) {
                            if (it->hIcon) DestroyIcon(it->hIcon);
                            g_TrayIcons.erase(it);
                            break;
                        }
                    }
                    updated = true;
                    InvalidateRect(hwnd, NULL, FALSE);
                }

                if (updated && g_Config.Mode == TaskbarMode::Independent) { // Sync to hToolbar in independent mode - Builder-Bob
                    for (auto* inst : g_Taskbars) {
                        if (inst && inst->hToolbar) {
                            UpdateTrayToolbarFromIndependent(inst);
                            UpdateTaskbarLayout(inst);
                        }
                    }
                }
            }
        }
        return TRUE;
    }
    case WM_SETCURSOR: {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hwnd, &pt);
        if (pt.x >= 0 && pt.x <= 5 && !IsTaskbarLocked()) { // - Draftsman-Dan
            SetCursor(LoadCursor(NULL, IDC_SIZEWE));
            return TRUE;
        }
        break;
    }
    case WM_CAPTURECHANGED: {
        if (s_bResizingTray) {
            s_bResizingTray = false;
        }
        break;
    }
    case WM_MOUSEMOVE: {
        // Prune dead/ghost icons on mouse move - Builder-Bob
        bool anyPruned = false;
        for (auto it = g_TrayIcons.begin(); it != g_TrayIcons.end(); ) {
            if (it->hWnd && !IsWindow(it->hWnd)) {
                if (it->hIcon) DestroyIcon(it->hIcon);
                it = g_TrayIcons.erase(it);
                anyPruned = true;
            } else {
                ++it;
            }
        }
        if (anyPruned) {
            InvalidateRect(hwnd, NULL, FALSE);
            TaskbarInstance* inst = GetTaskbarInstance(hwnd);
            if (inst) {
                UpdateTaskbarLayout(inst);
                if (inst->hToolbar) {
                    if (g_Config.Mode == TaskbarMode::Independent) {
                        UpdateTrayToolbarFromIndependent(inst);
                    } else {
                        std::vector<ScrapedTrayIcon> icons = ScrapeTrayIcons();
                        UpdateTrayToolbar(inst->hToolbar, inst->hTrayImageList, icons);
                    }
                }
            }
        }

        int xPos = GET_X_LPARAM(lParam);
        int yPos = GET_Y_LPARAM(lParam);

        if (s_bResizingTray) {
            POINT ptScreen;
            GetCursorPos(&ptScreen);
            int deltaX = ptScreen.x - s_dragStartScreenX;
            int newW = s_dragStartWTray - deltaX;
            int minW = 24;
            if (newW < minW) newW = minW;

            g_Config.ManualTrayWidth = newW;
            HKEY hKey;
            if (RegCreateKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Settings", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) { // - Draftsman-Dan
                DWORD dwVal = newW;
                RegSetValueExW(hKey, L"ManualTrayWidth", 0, REG_DWORD, (const BYTE*)&dwVal, sizeof(DWORD));
                RegCloseKey(hKey);
            }

            TaskbarInstance* inst = GetTaskbarInstance(hwnd);
            if (inst) {
                UpdateTaskbarLayout(inst);
                if (inst->hToolbar) {
                    if (g_Config.Mode == TaskbarMode::Independent) {
                        UpdateTrayToolbarFromIndependent(inst);
                    } else {
                        std::vector<ScrapedTrayIcon> icons = ScrapeTrayIcons();
                        UpdateTrayToolbar(inst->hToolbar, inst->hTrayImageList, icons);
                    }
                }
            }
            SetCursor(LoadCursor(NULL, IDC_SIZEWE));
            return 0;
        }

        if (xPos >= 0 && xPos <= 5 && !IsTaskbarLocked()) { // - Draftsman-Dan
            SetCursor(LoadCursor(NULL, IDC_SIZEWE));
        }

        int dpi = GetDpiForWindowHelper(hwnd);
        int totalVisible = 0;
        for (const auto& icon : g_TrayIcons) {
            if (icon.hIcon && !(icon.dwState & NIS_HIDDEN)) totalVisible++;
        }
        int limit = GetTrayVisibleLimit(hwnd, dpi, totalVisible);
        int W_overflowBtn = MulDiv(18, dpi, 96);
        bool bIsWin7Mode = (g_Config.OverflowMode == TrayOverflowMode::Win7Flyout);
        bool bIsExpanded = (GetPropW(hwnd, L"TrayExpanded") != NULL);

        bool bTracking = GetTooltipTracking(hwnd);
        if (!bTracking) {
            TRACKMOUSEEVENT tme = {0};
            tme.cbSize = sizeof(tme);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hwnd;
            TrackMouseEvent(&tme);
            SetTooltipTracking(hwnd, true);
        }

        int iconIndex = -1;
        if (g_Config.EnableTwoRowTray) {
            int iconOffset = (totalVisible > limit) ? W_overflowBtn : 2;
            RECT rc;
            GetClientRect(hwnd, &rc);
            int height = rc.bottom - rc.top;
            int y = (height - 26) / 2;
            if (xPos >= iconOffset) {
                int col = (xPos - iconOffset) / 18;
                int row = (yPos - y) / 14;
                if (row >= 0 && row < 2) {
                    int relX = (xPos - iconOffset) % 18;
                    int relY = (yPos - y) % 14;
                    if (relX >= 0 && relX < 18 && relY >= 0 && relY < 14) { // - Draftsman-Dan
                        iconIndex = col * 2 + row;
                    }
                }
            }
        } else {
            if (!(totalVisible > limit && xPos < W_overflowBtn)) {
                int iconOffset = (totalVisible > limit) ? W_overflowBtn : 2;
                int tempIdx = (xPos - iconOffset) / 24;
                if (tempIdx >= 0) {
                    RECT rc;
                    GetClientRect(hwnd, &rc);
                    int height = rc.bottom - rc.top;
                    int relativeX = (xPos - iconOffset) % 24;
                    if (relativeX >= 0 && relativeX < 24 && yPos >= 0 && yPos < height) { // - Draftsman-Dan
                        iconIndex = tempIdx;
                    }
                }
            }
        }

        int visibleDrawnCount = totalVisible;
        if (totalVisible > limit) {
            if (bIsWin7Mode) {
                visibleDrawnCount = limit;
            } else {
                if (!bIsExpanded) {
                    visibleDrawnCount = limit;
                }
            }
        }
        if (iconIndex >= visibleDrawnCount) {
            iconIndex = -1;
        }

        int lastIndex = GetTooltipLastIndex(hwnd);
        if (iconIndex != lastIndex) {
            SetTooltipLastIndex(hwnd, iconIndex);
            HWND hTip = GetOrCreateTrayTooltip(hwnd);
            if (iconIndex >= 0) {
                int current = 0;
                int drawn = 0;
                for (const auto& icon : g_TrayIcons) {
                    if (icon.hIcon && !(icon.dwState & NIS_HIDDEN)) {
                        if (totalVisible > limit) {
                            if (bIsWin7Mode) {
                                if (current < totalVisible - limit) { current++; continue; }
                            } else {
                                if (!bIsExpanded && current < totalVisible - limit) { current++; continue; }
                            }
                        }

                        if (drawn == iconIndex) {
                            std::wstring tip = icon.szTip;
                            if (!tip.empty()) {
                                POINT ptScreen = { xPos, yPos };
                                ClientToScreen(hwnd, &ptScreen);
                                UpdateTooltipText(hTip, hwnd, tip, ptScreen);
                            } else {
                                HideTooltip(hTip, hwnd);
                            }
                            break;
                        }
                        drawn++;
                        current++;
                    }
                }
            } else {
                HideTooltip(hTip, hwnd);
            }
        } else if (iconIndex >= 0) {
            HWND hTip = GetOrCreateTrayTooltip(hwnd);
            POINT ptScreen = { xPos, yPos };
            ClientToScreen(hwnd, &ptScreen);
            SendMessageW(hTip, TTM_TRACKPOSITION, 0, MAKELPARAM(ptScreen.x, ptScreen.y - 24));
        }

        if (iconIndex >= 0) {
            int current = 0;
            int drawn = 0;
            for (const auto& icon : g_TrayIcons) {
                if (icon.hIcon && !(icon.dwState & NIS_HIDDEN)) {
                    if (totalVisible > limit) {
                        if (bIsWin7Mode) {
                            if (current < totalVisible - limit) { current++; continue; }
                        } else {
                            if (!bIsExpanded && current < totalVisible - limit) { current++; continue; }
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
        }
        return 0;
    }
    case WM_MOUSELEAVE: {
        SetTooltipTracking(hwnd, false);
        SetTooltipLastIndex(hwnd, -1);
        HWND hTip = GetOrCreateTrayTooltip(hwnd);
        HideTooltip(hTip, hwnd);
        break;
    }
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK: {
        int xPos = GET_X_LPARAM(lParam);
        int yPos = GET_Y_LPARAM(lParam);

        if (uMsg == WM_LBUTTONDOWN && xPos >= 0 && xPos <= 5 && !IsTaskbarLocked()) { // - Draftsman-Dan
            s_bResizingTray = true;
            SetCapture(hwnd);
            POINT ptScreen;
            GetCursorPos(&ptScreen);
            s_dragStartScreenX = ptScreen.x;

            TaskbarInstance* inst = GetTaskbarInstance(hwnd);
            int currentW = 100;
            if (inst && inst->hToolbar) {
                RECT rcToolbar;
                GetClientRect(inst->hToolbar, &rcToolbar);
                currentW = rcToolbar.right - rcToolbar.left;
            }
            s_dragStartWTray = currentW;
            return 0;
        }

        if (uMsg == WM_LBUTTONUP && s_bResizingTray) {
            s_bResizingTray = false;
            ReleaseCapture();
            return 0;
        }

        bool bIsWin7Mode = (g_Config.OverflowMode == TrayOverflowMode::Win7Flyout);
        bool bIsExpanded = (GetPropW(hwnd, L"TrayExpanded") != NULL);

        int totalVisible = 0;
        for (const auto& icon : g_TrayIcons) {
            if (icon.hIcon && !(icon.dwState & NIS_HIDDEN)) totalVisible++;
        }

        int dpi = GetDpiForWindowHelper(hwnd);
        int limit = GetTrayVisibleLimit(hwnd, dpi, totalVisible);
        int W_overflowBtn = MulDiv(18, dpi, 96);

        if (totalVisible > limit && xPos < W_overflowBtn) {
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

                    TaskbarInstance* inst = GetTaskbarInstance(hwnd);
                    if (inst) {
                        UpdateTaskbarLayout(inst);
                        if (inst->hToolbar) {
                            if (g_Config.Mode == TaskbarMode::Independent) {
                                UpdateTrayToolbarFromIndependent(inst);
                            } else {
                                std::vector<ScrapedTrayIcon> icons = ScrapeTrayIcons();
                                UpdateTrayToolbar(inst->hToolbar, inst->hTrayImageList, icons);
                            }
                        }
                    }
                }
            }
            return 0;
        }

        int iconIndex = -1;
        if (g_Config.EnableTwoRowTray) {
            int iconOffset = (totalVisible > limit) ? W_overflowBtn : 2;
            RECT rc;
            GetClientRect(hwnd, &rc);
            int height = rc.bottom - rc.top;
            int y = (height - 26) / 2;
            if (xPos >= iconOffset) {
                int col = (xPos - iconOffset) / 18;
                int row = (yPos - y) / 14;
                if (row >= 0 && row < 2) {
                    int relX = (xPos - iconOffset) % 18;
                    int relY = (yPos - y) % 14;
                    if (relX >= 0 && relX < 18 && relY >= 0 && relY < 14) { // - Draftsman-Dan
                        iconIndex = col * 2 + row;
                    }
                }
            }
        } else {
            int iconOffset = (totalVisible > limit) ? W_overflowBtn : 2;
            int tempIdx = (xPos - iconOffset) / 24;
            if (tempIdx >= 0) {
                RECT rc;
                GetClientRect(hwnd, &rc);
                int height = rc.bottom - rc.top;
                int relativeX = (xPos - iconOffset) % 24;
                if (relativeX >= 0 && relativeX < 24 && yPos >= 0 && yPos < height) { // - Draftsman-Dan
                    iconIndex = tempIdx;
                }
            }
        }

        int visibleDrawnCount = totalVisible;
        if (totalVisible > limit) {
            if (bIsWin7Mode) {
                visibleDrawnCount = limit;
            } else {
                if (!bIsExpanded) {
                    visibleDrawnCount = limit;
                }
            }
        }
        if (iconIndex < 0 || iconIndex >= visibleDrawnCount) {
            return 0;
        }

        int current = 0;
        int drawn = 0;
        for (const auto& icon : g_TrayIcons) {
            if (icon.hIcon && !(icon.dwState & NIS_HIDDEN)) {
                if (totalVisible > limit) {
                    if (bIsWin7Mode) {
                        if (current < totalVisible - limit) { current++; continue; }
                    } else {
                        if (!bIsExpanded && current < totalVisible - limit) { current++; continue; }
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
    // Forward mouse events in left 0-5px margin to parent hTrayNotify - Draftsman-Dan
    if (uMsg == WM_SETCURSOR) {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hwnd, &pt);
        if (pt.x >= 0 && pt.x <= 5) {
            HWND hParent = GetParent(hwnd);
            return SendMessageW(hParent, uMsg, wParam, lParam);
        }
    }
    if (uMsg == WM_MOUSEMOVE || uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP ||
        uMsg == WM_LBUTTONDBLCLK || uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONUP ||
        uMsg == WM_RBUTTONDBLCLK || uMsg == WM_MBUTTONDOWN || uMsg == WM_MBUTTONUP ||
        uMsg == WM_MBUTTONDBLCLK)
    {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        if (x >= 0 && x <= 5) {
            POINT pt = { x, y };
            ClientToScreen(hwnd, &pt);
            HWND hParent = GetParent(hwnd);
            ScreenToClient(hParent, &pt);
            return SendMessageW(hParent, uMsg, wParam, MAKELPARAM(pt.x, pt.y));
        }
    }

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
        UINT dpi = 96;
        HMONITOR hMon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        HMODULE hShcore = LoadLibraryW(L"shcore.dll");
        if (hShcore) {
            typedef HRESULT(STDAPICALLTYPE* GetDpiForMonitorFn)(HMONITOR, int, UINT*, UINT*);
            GetDpiForMonitorFn fn = (GetDpiForMonitorFn)GetProcAddress(hShcore, "GetDpiForMonitor");
            if (fn) {
                UINT dpiX = 96, dpiY = 96;
                fn(hMon, 0, &dpiX, &dpiY);
                dpi = dpiX;
            }
            FreeLibrary(hShcore);
        } else {
            HDC hdcScr = GetDC(NULL);
            if (hdcScr) {
                dpi = GetDeviceCaps(hdcScr, LOGPIXELSX);
                ReleaseDC(NULL, hdcScr);
            }
        }
        lf.lfHeight = MulDiv(-11, dpi, 96); // Smaller font to fit both
        lf.lfWeight = FW_NORMAL;
        wcscpy_s(lf.lfFaceName, L"Segoe UI");

        if (hBufferedPaint) {
            DrawThemeParentBackground(hwnd, hdcBuffer, &rcClient);
            
            HTHEME hTheme = OpenThemeData(hwnd, L"Taskbar");
            if (hTheme) {
                HFONT hFont = CreateFontIndirectW(&lf);
                HFONT hOldFont = (HFONT)SelectObject(hdcBuffer, hFont);

                RECT rcCalc = {0};
                DrawTextW(hdcBuffer, clockText, -1, &rcCalc, DT_CENTER | DT_CALCRECT);
                int textHeight = rcCalc.bottom - rcCalc.top;
                int clientHeight = rcClient.bottom - rcClient.top;
                if (textHeight < clientHeight) {
                    int yOffset = (clientHeight - textHeight) / 2;
                    rcClient.top += yOffset;
                    rcClient.bottom = rcClient.top + textHeight;
                }

                DTTOPTS dttOpts = { sizeof(DTTOPTS) };
                dttOpts.dwFlags = DTT_COMPOSITED | DTT_GLOWSIZE | DTT_TEXTCOLOR;
                dttOpts.iGlowSize = 0; // Neutralize glow but keep alpha channel intact
                dttOpts.crText = RGB(255, 255, 255);
                
                HRESULT hr = DrawThemeTextEx(hTheme, hdcBuffer, 0, 0, clockText, -1, DT_CENTER | DT_VCENTER, &rcClient, &dttOpts);
                if (FAILED(hr)) {
                    SetTextColor(hdcBuffer, RGB(255, 255, 255));
                    SetBkMode(hdcBuffer, TRANSPARENT);
                    DrawTextW(hdcBuffer, clockText, -1, &rcClient, DT_CENTER | DT_VCENTER | DT_NOCLIP);
                }
                
                SelectObject(hdcBuffer, hOldFont);
                DeleteObject(hFont);
                CloseThemeData(hTheme);
            } else {
                // Fallback if Theme fails inside BufferedPaint
                HFONT hFont = CreateFontIndirectW(&lf);
                HFONT hOldFont = (HFONT)SelectObject(hdcBuffer, hFont);

                RECT rcCalc = {0};
                DrawTextW(hdcBuffer, clockText, -1, &rcCalc, DT_CENTER | DT_CALCRECT);
                int textHeight = rcCalc.bottom - rcCalc.top;
                int clientHeight = rcClient.bottom - rcClient.top;
                if (textHeight < clientHeight) {
                    int yOffset = (clientHeight - textHeight) / 2;
                    rcClient.top += yOffset;
                    rcClient.bottom = rcClient.top + textHeight;
                }

                SetTextColor(hdcBuffer, RGB(255, 255, 255));
                SetBkMode(hdcBuffer, TRANSPARENT);
                DrawTextW(hdcBuffer, clockText, -1, &rcClient, DT_CENTER | DT_VCENTER | DT_NOCLIP);
                SelectObject(hdcBuffer, hOldFont);
                DeleteObject(hFont);
            }
            EndBufferedPaint(hBufferedPaint, TRUE);
        } else {
            // Absolute Fallback if BufferedPaint fails completely for Standard User
            HFONT hFont = CreateFontIndirectW(&lf);
            HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

            RECT rcCalc = {0};
            DrawTextW(hdc, clockText, -1, &rcCalc, DT_CENTER | DT_CALCRECT);
            int textHeight = rcCalc.bottom - rcCalc.top;
            int clientHeight = rcClient.bottom - rcClient.top;
            if (textHeight < clientHeight) {
                int yOffset = (clientHeight - textHeight) / 2;
                rcClient.top += yOffset;
                rcClient.bottom = rcClient.top + textHeight;
            }

            SetTextColor(hdc, RGB(255, 255, 255));
            SetBkMode(hdc, TRANSPARENT);
            DrawTextW(hdc, clockText, -1, &rcClient, DT_CENTER | DT_VCENTER | DT_NOCLIP);
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
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\EliteSoftware\\Win32Explorer\\Settings", 0, KEY_READ, &hKey) == ERROR_SUCCESS) { // - Draftsman-Dan
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
    static WCHAR s_szSettingChangeParam[256] = {0};
    if (uMsg == WM_WINDOWPOSCHANGING) {
        WINDOWPOS* lpw = (WINDOWPOS*)lParam;
        if (lpw) {
            if (g_Config.Mode == TaskbarMode::Replace) {
                lpw->hwndInsertAfter = HWND_TOPMOST;
            } else {
                if (g_hNativeTaskbar && IsWindow(g_hNativeTaskbar)) {
                    lpw->hwndInsertAfter = g_hNativeTaskbar;
                } else {
                    lpw->hwndInsertAfter = HWND_TOPMOST;
                }
            }
            lpw->flags &= ~SWP_NOZORDER;
        }
        return 0;
    }
    if (uMsg == WM_DPICHANGED) {
        UINT newDpi = HIWORD(wParam);
        LPRECT lprcSuggested = (LPRECT)lParam;
        TaskbarInstance* inst = GetTaskbarInstance(hwnd);
        if (inst) {
            int baseHeight = 40;
            HKEY hKeySmall;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 0, KEY_READ, &hKeySmall) == ERROR_SUCCESS) {
                DWORD dwVal = 0, cbData = sizeof(DWORD);
                if (RegQueryValueExW(hKeySmall, L"TaskbarSmallIcons", NULL, NULL, (LPBYTE)&dwVal, &cbData) == ERROR_SUCCESS) {
                    baseHeight = (dwVal == 1) ? 30 : 40;
                }
                RegCloseKey(hKeySmall);
            }
            inst->taskbarHeight = MulDiv(baseHeight, newDpi, 96);
            SetWindowPos(hwnd, HWND_TOPMOST, lprcSuggested->left, lprcSuggested->top,
                         lprcSuggested->right - lprcSuggested->left,
                         inst->taskbarHeight, SWP_NOACTIVATE | SWP_NOZORDER);
            UpdateTaskbarLayout(inst);
            InvalidateRect(hwnd, NULL, TRUE);
        }
        return 0;
    }
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
                
                info.hIcon = GetWindowIconFix(hwndShell);
                
                info.cmdId = g_NextCmdId++;
                g_TaskButtons.push_back(info);
                AddTaskButton(g_TaskButtons.back());
            }
        }
        else if (nCode == HSHELL_WINDOWDESTROYED) {
            for (auto it = g_TaskButtons.begin(); it != g_TaskButtons.end(); ++it) {
                if (it->hwnd == hwndShell) {
                    if (it->hIcon) {
                        DestroyIcon(it->hIcon);
                    }
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
                    
                    if (btn.hIcon) {
                        DestroyIcon(btn.hIcon);
                    }
                    btn.hIcon = GetWindowIconFix(hwndShell);
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
    case WM_HOTKEY: {
        if (wParam == 1) { // Win+E
            WCHAR exePath[MAX_PATH];
            GetModuleFileNameW(NULL, exePath, MAX_PATH);
            WCHAR* lastSlash = wcsrchr(exePath, L'\\');
            if (lastSlash) {
                wcscpy_s(lastSlash + 1, MAX_PATH - (lastSlash + 1 - exePath), L"Win32Explorer.exe");
                ShellExecuteW(NULL, L"open", exePath, NULL, NULL, SW_SHOWNORMAL);
            } else {
                ShellExecuteW(NULL, L"open", L"Win32Explorer.exe", NULL, NULL, SW_SHOWNORMAL);
            }
        } else if (wParam == 2) { // Win+R
            InvokeNativeRunDialog(hwnd);
        } else if (wParam == 3) { // Win+D
            SendMessageW(hwnd, WM_COMMAND, IDM_TASKBAR_SHOWDESKTOP, 0);
        }
        return 0;
    }
    case WM_TRAY_CALLBACK_WIN32EXPLORER: {
        if (lParam == WM_LBUTTONUP) {
            extern void ShowAboutDialog(HWND hwndOwner);
            ShowAboutDialog(hwnd);
        } else if (lParam == WM_LBUTTONDBLCLK) {
            wchar_t exePath[MAX_PATH];
            GetModuleFileNameW(NULL, exePath, MAX_PATH);
            PathRemoveFileSpecW(exePath);
            std::wstring path = std::wstring(exePath) + L"\\Win32Explorer.exe";
            ShellExecuteW(NULL, L"open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
        }
        return 0;
    }
    case WM_TRAY_CALLBACK_TASKBAR: {
        if (lParam == WM_LBUTTONUP) {
            extern void ShowAboutDialog(HWND hwndOwner);
            ShowAboutDialog(hwnd);
        } else if (lParam == WM_LBUTTONDBLCLK) {
            wchar_t modulePath[MAX_PATH];
            if (GetModuleFileNameW(NULL, modulePath, MAX_PATH)) {
                std::wstring pathStr(modulePath);
                size_t lastSlash = pathStr.find_last_of(L"\\/");
                if (lastSlash != std::wstring::npos) {
                    std::wstring settingsCpl = pathStr.substr(0, lastSlash) + L"\\EliteSettings.cpl";
                    std::wstring parameters = L"\"" + settingsCpl + L"\"";
                    ShellExecuteW(hwnd, L"open", L"control.exe", parameters.c_str(), NULL, SW_SHOWNORMAL);
                }
            }        }
        return 0;
    }
    case WM_TRAY_CALLBACK_DESKTOP: {
        if (lParam == WM_LBUTTONDBLCLK) {
            HWND hProgman = FindWindowW(L"Progman", NULL);
            HWND hDefView = FindWindowExW(hProgman, NULL, L"SHELLDLL_DefView", NULL);
            if (!hDefView) {
                HWND hWorkerW = NULL;
                while ((hWorkerW = FindWindowExW(NULL, hWorkerW, L"WorkerW", NULL)) != NULL) {
                    hDefView = FindWindowExW(hWorkerW, NULL, L"SHELLDLL_DefView", NULL);
                    if (hDefView) {
                        hProgman = hWorkerW;
                        break;
                    }
                }
            }
            HWND hListView = FindWindowExW(hDefView, NULL, L"SysListView32", NULL);
            if (hListView && IsWindow(hListView)) {
                bool isVisible = IsWindowVisible(hListView);
                ShowWindow(hListView, isVisible ? SW_HIDE : SW_SHOW);
                HKEY hKey;
                if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
                    DWORD dwHide = isVisible ? 1 : 0;
                    RegSetValueExW(hKey, L"HideIcons", 0, REG_DWORD, (const BYTE*)&dwHide, sizeof(DWORD));
                    RegCloseKey(hKey);
                }
                PostMessageW(HWND_BROADCAST, WM_COMMAND, 28607, 0);
                SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
            }
        } else if (lParam == WM_RBUTTONUP) {
            HMENU hMenu = CreatePopupMenu();
            bool desktopReplaceEnabled = true;
            HKEY hKey;
            if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Settings", 0, KEY_READ, &hKey) == ERROR_SUCCESS) { // - Draftsman-Dan
                DWORD dwVal = 1;
                DWORD cbData = sizeof(DWORD);
                if (RegQueryValueExW(hKey, L"DesktopReplacementEnabled", NULL, NULL, (LPBYTE)&dwVal, &cbData) == ERROR_SUCCESS) {
                    desktopReplaceEnabled = (dwVal == 1);
                }
                RegCloseKey(hKey);
            }
            AppendMenuW(hMenu, MF_STRING | (desktopReplaceEnabled ? MF_CHECKED : MF_UNCHECKED), 10001, L"Toggle Desktop Replacement");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hMenu, MF_STRING, 10002, L"Restart Taskbar");
            AppendMenuW(hMenu, MF_STRING, 10003, L"Restart Explorer");
            
            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hwnd);
            int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
            DestroyMenu(hMenu);
            
            if (cmd == 10001) {
                desktopReplaceEnabled = !desktopReplaceEnabled;
                HKEY hKeyWrite;
                if (RegCreateKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Settings", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKeyWrite, NULL) == ERROR_SUCCESS) { // - Draftsman-Dan
                    DWORD dwVal = desktopReplaceEnabled ? 1 : 0;
                    RegSetValueExW(hKeyWrite, L"DesktopReplacementEnabled", 0, REG_DWORD, (const BYTE*)&dwVal, sizeof(DWORD));
                    RegCloseKey(hKeyWrite);
                }
                if (desktopReplaceEnabled) {
                    DesktopWindow::Initialize();
                } else {
                    DesktopWindow::Cleanup();
                }
            } else if (cmd == 10002) {
                RestartEliteTaskbar(); // - Draftsman-Dan
            } else if (cmd == 10003) {
                RestartExplorerShell(); // - Draftsman-Dan
            }
        }
        return 0;
    }
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
                    return CDRF_DODEFAULT; // - Draftsman-Dan
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
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
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
        
        if (wmId >= 3001 && wmId <= 3999 && wmId != IDM_EXIT_ELITETASKBAR && wmId != IDM_EXIT_ALL_ELITETASKBAR) { // - Draftsman-Dan
            // Find target path in g_FolderBands buttons dwData - Builder-Bob
            for (auto* band : g_FolderBands) {
                if (band && band->hToolbar) {
                    TBBUTTON btn = {0};
                    int index = (int)SendMessageW(band->hToolbar, TB_COMMANDTOINDEX, wmId, 0);
                    if (index >= 0) {
                        if (SendMessageW(band->hToolbar, TB_GETBUTTON, index, (LPARAM)&btn)) {
                            if (btn.dwData) {
                                std::wstring* pPath = (std::wstring*)btn.dwData;
                                ExecuteTarget(*pPath);
                            }
                        }
                        break;
                    }
                }
            }
            return 0;
        }
        
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
                    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS) {
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
            case IDM_TASKBAR_SETTINGS: {
                wchar_t modulePath[MAX_PATH];
                if (GetModuleFileNameW(NULL, modulePath, MAX_PATH)) {
                    std::wstring pathStr(modulePath);
                    size_t lastSlash = pathStr.find_last_of(L"\\/");
                    if (lastSlash != std::wstring::npos) {
                        std::wstring settingsCpl = pathStr.substr(0, lastSlash) + L"\\EliteSettings.cpl";
                        std::wstring parameters = L"\"" + settingsCpl + L"\"";
                        ShellExecuteW(hwnd, L"open", L"control.exe", parameters.c_str(), NULL, SW_SHOWNORMAL);
                    }
                }
                break;
            }

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
                RestartExplorerShell(); // - Draftsman-Dan
                break;
            case IDM_RELOAD_TASKBAR: {
                RestartEliteTaskbar(); // - Draftsman-Dan
                break;
            }
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
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
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
            AppendMenuW(hMenu, MF_STRING, IDM_RELOAD_TASKBAR, L"Reload Taskbar");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
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
        if (wParam == 555) {
            KillTimer(hwnd, 555);
            // Execute debounced teardown and re-initialization - Builder-Bob
            bool requiresRestart = false;
            HKEY hKey;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\EliteSoftware\\Win32Explorer\\Settings", 0, KEY_READ, &hKey) == ERROR_SUCCESS) { // - Draftsman-Dan
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
                WCHAR exePath[MAX_PATH] = {0};
                GetModuleFileNameW(NULL, exePath, MAX_PATH);

                ::g_IsRestarting = true;
                RestartEliteTaskbar(); // - Draftsman-Dan
            } else {
                // Teardown: Safely call DestroyIcon on scraped handles to prevent GDI leaks - Builder-Bob
                for (auto& icon : g_TrayIcons) {
                    if (icon.hIcon) {
                        DestroyIcon(icon.hIcon);
                        icon.hIcon = NULL;
                    }
                }
                g_TrayIcons.clear();

                // Teardown folder bands - Builder-Bob
                for (auto* band : g_FolderBands) {
                    delete band;
                }
                g_FolderBands.clear();

                QueryOperationalMode();
                
                for (auto* inst : g_Taskbars) {
                    if (inst->hRebar) {
                        int bandCount = (int)SendMessageW(inst->hRebar, RB_GETBANDCOUNT, 0, 0);
                        for (int i = bandCount - 1; i >= 0; i--) {
                            SendMessageW(inst->hRebar, RB_DELETEBAND, i, 0);
                        }
                        LoadFolderToolbars(inst->hRebar);
                    }
                    if (inst->hTaskSwitch) {
                        int count = (int)SendMessageW(inst->hTaskSwitch, TB_BUTTONCOUNT, 0, 0);
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
                
                if (g_uTaskbarCreatedMsg != 0) {
                    PostMessageW(HWND_BROADCAST, g_uTaskbarCreatedMsg, 0, 0);
                    SendNotifyMessageW(HWND_BROADCAST, g_uTaskbarCreatedMsg, 0, 0);
                }
                
                if (s_szSettingChangeParam[0] != L'\0' && wcscmp(s_szSettingChangeParam, L"EliteTaskbarSettings") == 0) {
                    for (auto* inst : g_Taskbars) {
                        if (inst->hTaskbar == hwnd && inst->startButton && inst->startButton->GetHwnd()) {
                            inst->startButton->ReloadOrbImage(GetModuleHandleW(NULL), inst->monitorIndex);
                            
                            RECT rcWindow;
                            GetWindowRect(hwnd, &rcWindow);
                            inst->startButton->Show(rcWindow.left, rcWindow.top, rcWindow.bottom - rcWindow.top);
                            break;
                        }
                    }
                }
            }
            return 0;
        }
        if (wParam == 1001) {
            TaskbarInstance* inst = GetTaskbarInstance(hwnd);
            if (inst && inst->hToolbar) {
                std::vector<ScrapedTrayIcon> icons = ScrapeTrayIcons();
                UpdateTrayToolbar(inst->hToolbar, inst->hTrayImageList, icons);
                UpdateTaskbarLayout(inst);
            }
            return 0;
        }
        if (wParam == 9998) {
            SyncWindowsAcrossMonitors();
            return 0;
        }
        if (wParam == 9999) {
            extern bool g_IsSpoofingNativeTaskbar;
            extern DWORD g_SpoofStartTime;
            if (g_IsSpoofingNativeTaskbar) {
                if (GetTickCount() - g_SpoofStartTime > 2000) {
                    g_IsSpoofingNativeTaskbar = false;
                }
            }
            if (g_Config.Mode == TaskbarMode::Replace && g_hNativeTaskbar) {
                if (!g_IsSpoofingNativeTaskbar) {
                    if (IsWindowVisible(g_hNativeTaskbar)) {
                        ShowWindow(g_hNativeTaskbar, SW_HIDE);
                        SetWindowPos(g_hNativeTaskbar, NULL, -10000, -10000, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
                    }
                }
                SyncTaskbarButtonsAcrossMonitors();
            } else {
                SyncTaskbarButtonsAcrossMonitors();
            }
        } else if (wParam == 2000) { // Preview timer
            KillTimer(hwnd, 2000);
            g_PreviewTimer = 0;
            if (g_Config.ShowPreviews && IsWindow(g_PreviewTargetHwnd)) {
                if (!g_hPreviewWindow) {
                    g_hPreviewWindow = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED, L"TaskbarPreview", L"", WS_POPUP | WS_CLIPCHILDREN, 0, 0, 200, 150, hwnd, NULL, GetModuleHandle(NULL), NULL); // - Draftsman-Dan
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
        // Debounce Settings Change (1000ms delay) to prevent rapid-fire multi-spawns - Builder-Bob
        s_szSettingChangeParam[0] = L'\0';
        if (lParam) {
            wcscpy_s(s_szSettingChangeParam, (LPCWSTR)lParam);
        }
        KillTimer(hwnd, 555);
        SetTimer(hwnd, 555, 1000, NULL);
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

// Rename callback to TaskbarMonitorEnumProc to resolve ODR violation - Builder-Bob
BOOL CALLBACK TaskbarMonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
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
    if (!g_Config.EnableEliteTaskbar) {
        Logger::Log(L"Elite Taskbar is disabled by setting. Skipping taskbar window creation, initializing Desktop only. - Builder-Bob");
        DesktopWindow::Initialize();
        return true;
    }

    g_uTaskbarCreatedMsg = RegisterWindowMessageW(L"TaskbarCreated");
    g_uShellHookMsg = RegisterWindowMessageW(L"SHELLHOOK");

    WNDCLASSW wcPrev = {0};
    wcPrev.lpfnWndProc = PreviewWndProc;
    wcPrev.hInstance = hInstance;
    wcPrev.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcPrev.lpszClassName = L"TaskbarPreview"; // - Draftsman-Dan
    RegisterClassW(&wcPrev);

    int taskbarHeight = 40; // Default fallback
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
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
    
    bool bRegistered = false;
    if (g_Config.Mode == TaskbarMode::Replace) {
        if (RegisterClassExW(&wc)) {
            bRegistered = true;
        } else {
            Logger::Log(L"Failed to register Shell_TrayWnd, falling back to Shell_SecondaryTrayWnd.");
            s_UseSecondaryTrayWndAsFallback = true;
            wc.lpszClassName = SEC_CLASS_NAME;
            if (RegisterClassExW(&wc)) {
                bRegistered = true;
            }
        }
    } else {
        if (RegisterClassExW(&wc)) {
            bRegistered = true;
        }
    }

    if (!bRegistered) {
        Logger::Log(L"Failed to register taskbar window class.");
        return false;
    }

    if (g_Config.Mode == TaskbarMode::Replace && !s_UseSecondaryTrayWndAsFallback) {
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
    // Update reference to TaskbarMonitorEnumProc - Builder-Bob
    EnumDisplayMonitors(NULL, NULL, TaskbarMonitorEnumProc, (LPARAM)&monData);
    
    bool bHookRegistered = false;
    for (size_t i = 0; i < monData.monitors.size(); i++) {
        if (g_Config.Mode == TaskbarMode::SecondaryOnly && monData.isPrimary[i]) {
            HWND hNativeTray = FindWindowW(L"Shell_TrayWnd", NULL);
            if (hNativeTray != NULL && IsWindowVisible(hNativeTray)) {
                continue; // Skip primary monitor in SecondaryOnly mode only if native taskbar is running and visible - Draftsman-Dan
            }
        }
        
        TaskbarInstance* inst = new TaskbarInstance();
        inst->hMonitor = monData.monitors[i];
        inst->monitorIndex = i;
        inst->monitorRect = monData.rects[i];
        inst->hNativeTrayNotify = NULL;
        inst->bStolenSysPager = false;
        
        int baseHeight = 40;
        {
            HKEY hKeySmall;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 0, KEY_READ, &hKeySmall) == ERROR_SUCCESS) {
                DWORD dwValue = 0;
                DWORD cbData = sizeof(DWORD);
                if (RegQueryValueExW(hKeySmall, L"TaskbarSmallIcons", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                    baseHeight = (dwValue == 1) ? 30 : 40;
                }
                RegCloseKey(hKeySmall);
            }
            if (g_hNativeTaskbar) {
                RECT nativeRect = {0};
                GetWindowRect(g_hNativeTaskbar, &nativeRect);
                int nativeHeight = nativeRect.bottom - nativeRect.top;
                if (nativeHeight > 0 && nativeHeight < 100) {
                    HMONITOR hPrimaryMon = MonitorFromWindow(g_hNativeTaskbar, MONITOR_DEFAULTTOPRIMARY);
                    UINT primaryDpi = 96;
                    HMODULE hShcoreTemp = LoadLibraryW(L"shcore.dll");
                    if (hShcoreTemp) {
                        typedef HRESULT(STDAPICALLTYPE* GetDpiForMonitorFn)(HMONITOR, int, UINT*, UINT*);
                        GetDpiForMonitorFn fn = (GetDpiForMonitorFn)GetProcAddress(hShcoreTemp, "GetDpiForMonitor");
                        if (fn) {
                            UINT dx = 96, dy = 96;
                            fn(hPrimaryMon, 0, &dx, &dy);
                            primaryDpi = dx;
                        }
                        FreeLibrary(hShcoreTemp);
                    }
                    baseHeight = MulDiv(nativeHeight, 96, primaryDpi);
                }
            }
        }

        UINT dpi = 96;
        HMODULE hShcore = LoadLibraryW(L"shcore.dll");
        if (hShcore) {
            typedef HRESULT(STDAPICALLTYPE* GetDpiForMonitorFn)(HMONITOR, int, UINT*, UINT*);
            GetDpiForMonitorFn fn = (GetDpiForMonitorFn)GetProcAddress(hShcore, "GetDpiForMonitor");
            if (fn) {
                UINT dpiX = 96, dpiY = 96;
                fn(inst->hMonitor, 0, &dpiX, &dpiY);
                dpi = dpiX;
            }
            FreeLibrary(hShcore);
        } else {
            HDC hdcScr = GetDC(NULL);
            if (hdcScr) {
                dpi = GetDeviceCaps(hdcScr, LOGPIXELSX);
                ReleaseDC(NULL, hdcScr);
            }
        }
        inst->taskbarHeight = MulDiv(baseHeight, dpi, 96);
        
        int screenWidth = inst->monitorRect.right - inst->monitorRect.left;
        int screenHeight = inst->monitorRect.bottom - inst->monitorRect.top;
        int xPos = inst->monitorRect.left;
        int yPos = inst->monitorRect.bottom - inst->taskbarHeight;

        LPCWSTR szClassName = TRAY_CLASS_NAME;
        if (g_Config.Mode == TaskbarMode::Replace) {
            szClassName = (monData.isPrimary[i] && !s_UseSecondaryTrayWndAsFallback) ? CLASS_NAME : SEC_CLASS_NAME;
        }

        inst->hTaskbar = CreateWindowExW(
            0,
            szClassName, L"", WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
            xPos, yPos, screenWidth, inst->taskbarHeight,
            NULL, NULL, hInstance, NULL
        );
        
        if (!inst->hTaskbar) {
            delete inst;
            continue;
        }

        // Hide window from native taskbar using ITaskbarList - Builder-Bob
        ITaskbarList* pTaskList = NULL;
        HRESULT hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_ITaskbarList, (void**)&pTaskList);
        if (SUCCEEDED(hr) && pTaskList) {
            pTaskList->HrInit();
            pTaskList->DeleteTab(inst->hTaskbar);
            pTaskList->Release();
        }

        if (!bHookRegistered) {
            RegisterShellHookWindow(inst->hTaskbar);
            bHookRegistered = true;
        }

        DWORD enableTray = 1;
        DWORD enableClock = 1;
        DWORD enableTaskBtns = 1;
        HKEY hKeyAdv;
        if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Settings", 0, KEY_READ, &hKeyAdv) == ERROR_SUCCESS) { // - Draftsman-Dan
            DWORD cbData = sizeof(DWORD);
            WCHAR valName[64];
            wsprintfW(valName, L"EnableTray_Mon%d", (int)i);
            if (RegQueryValueExW(hKeyAdv, valName, NULL, NULL, (LPBYTE)&enableTray, &cbData) != ERROR_SUCCESS) {
                enableTray = 1;
            }
            cbData = sizeof(DWORD);
            wsprintfW(valName, L"EnableClock_Mon%d", (int)i);
            if (RegQueryValueExW(hKeyAdv, valName, NULL, NULL, (LPBYTE)&enableClock, &cbData) != ERROR_SUCCESS) {
                enableClock = 1;
            }
            cbData = sizeof(DWORD);
            wsprintfW(valName, L"EnableTaskBtns_Mon%d", (int)i);
            if (RegQueryValueExW(hKeyAdv, valName, NULL, NULL, (LPBYTE)&enableTaskBtns, &cbData) != ERROR_SUCCESS) {
                enableTaskBtns = 1;
            }
            RegCloseKey(hKeyAdv);
        }

        inst->hTaskSwitch = NULL;
        if (enableTaskBtns) {
            inst->hTaskSwitch = CreateWindowExW(0, TOOLBARCLASSNAMEW, L"", 
                WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TBSTYLE_LIST | TBSTYLE_FLAT | TBSTYLE_WRAPABLE | CCS_NODIVIDER | CCS_NORESIZE | TBSTYLE_TRANSPARENT, 
                MulDiv(60, dpi, 96), 0, screenWidth - MulDiv(315, dpi, 96), inst->taskbarHeight, inst->hTaskbar, (HMENU)2000, hInstance, NULL);
            SetWindowSubclass(inst->hTaskSwitch, TaskSwitchSubclassProc, 1, 0);
            SendMessageW(inst->hTaskSwitch, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
            // SendMessageW(inst->hTaskSwitch, TB_SETBUTTONSIZE, 0, MAKELPARAM(160, inst->taskbarHeight)); // Removed - Draftsman-Dan
            SendMessageW(inst->hTaskSwitch, TB_SETPADDING, 0, MAKELPARAM(10, 4));
            if (g_Config.ButtonWidth == ButtonWidthMode::Fixed) {
                int width = 160;
                if (g_Config.FixedWidthSize == 0) width = 100;
                else if (g_Config.FixedWidthSize == 2) width = 220;
                SendMessageW(inst->hTaskSwitch, TB_SETBUTTONWIDTH, 0, MAKELPARAM(width, width));
            } else if (g_Config.ButtonWidth == ButtonWidthMode::IconsOnly) {
                SendMessageW(inst->hTaskSwitch, TB_SETBUTTONWIDTH, 0, MAKELPARAM(40, 40));
            }
            
            SetWindowTheme(inst->hTaskSwitch, L"TaskBand", NULL);
            inst->hImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 10);
            SendMessageW(inst->hTaskSwitch, TB_SETIMAGELIST, 0, (LPARAM)inst->hImageList);
        }

        inst->hRebar = NULL;
        // Create Rebar control if toolbars are enabled - Builder-Bob
        INITCOMMONCONTROLSEX iccex;
        iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        iccex.dwICC = ICC_COOL_CLASSES;
        InitCommonControlsEx(&iccex);

        inst->hRebar = CreateWindowExW(
            WS_EX_TOOLWINDOW,
            REBARCLASSNAMEW, NULL,
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | RBS_VARHEIGHT | CCS_NODIVIDER | CCS_NORESIZE | RBS_BANDBORDERS,
            0, 0, 0, 0,
            inst->hTaskbar, NULL, hInstance, NULL
        );
        if (inst->hRebar) {
            LoadFolderToolbars(inst->hRebar);
        }

        inst->hTrayNotify = NULL;
        inst->hToolbar = NULL;
        inst->hTrayClock = NULL;

        if (enableTray || enableClock) {
            inst->hTrayNotify = CreateWindowExW(0, L"TrayNotifyWnd", L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, screenWidth - MulDiv(255, dpi, 96), 0, MulDiv(240, dpi, 96), inst->taskbarHeight, inst->hTaskbar, NULL, hInstance, NULL);
            
            if (enableTray) {
                DWORD toolbarStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | CCS_NODIVIDER | CCS_NORESIZE;
                if (g_Config.EnableTwoRowTray) {
                    toolbarStyle |= TBSTYLE_WRAPABLE;
                }
                inst->hToolbar = CreateWindowExW(0, L"ToolbarWindow32", L"", toolbarStyle, 0, 0, MulDiv(100, dpi, 96), inst->taskbarHeight, inst->hTrayNotify, NULL, hInstance, NULL);
                SetWindowSubclass(inst->hToolbar, TrayToolbarSubclassProc, 2, (DWORD_PTR)inst);
                SendMessageW(inst->hToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
                int iconDim = g_Config.EnableTwoRowTray ? 12 : 16;
                inst->hTrayImageList = ImageList_Create(iconDim, iconDim, ILC_COLOR32 | ILC_MASK, 0, 20);
                SendMessageW(inst->hToolbar, TB_SETIMAGELIST, 0, (LPARAM)inst->hTrayImageList);
            }
            
            int W_clock = MulDiv(85, dpi, 96);
            if (enableClock) {
                inst->hTrayClock = CreateWindowExW(0, L"TrayClockWClass", L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, MulDiv(100, dpi, 96), 0, W_clock, inst->taskbarHeight, inst->hTrayNotify, NULL, hInstance, NULL);
            }
            
            CreateWindowExW(0, L"TrayShowDesktopButtonWClass", L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, screenWidth - MulDiv(15, dpi, 96), 0, MulDiv(15, dpi, 96), inst->taskbarHeight, inst->hTaskbar, NULL, hInstance, NULL);
        }

        SetWindowPos(inst->hTaskbar, HWND_TOPMOST, xPos, yPos, screenWidth, inst->taskbarHeight, SWP_SHOWWINDOW);
        
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
            abd.uEdge = ABE_BOTTOM;
            abd.rc.left = xPos;
            abd.rc.right = xPos + screenWidth;
            abd.rc.top = yPos;
            abd.rc.bottom = yPos + inst->taskbarHeight;
            SHAppBarMessage(ABM_NEW, &abd);
            SHAppBarMessage(ABM_QUERYPOS, &abd);
            SHAppBarMessage(ABM_SETPOS, &abd);
        }

        ShowWindow(inst->hTaskbar, SW_SHOW);
        UpdateWindow(inst->hTaskbar);

        inst->startButton = new StartButton();
        if (inst->startButton->Initialize(hInstance, inst->hTaskbar, inst->monitorIndex)) {
            inst->startButton->ReloadOrbImage(hInstance, inst->monitorIndex);
            inst->startButton->Show(xPos, yPos, inst->taskbarHeight);
        }
        
        UpdateTaskbarLayout(inst);
        g_Taskbars.push_back(inst);

        if (monData.isPrimary[i]) {
            NOTIFYICONDATAW nid1 = { sizeof(NOTIFYICONDATAW) };
            nid1.hWnd = inst->hTaskbar;
            nid1.uID = 5001;
            nid1.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
            nid1.uCallbackMessage = WM_TRAY_CALLBACK_WIN32EXPLORER;
            nid1.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_MAIN_PROGRAM));
            wcscpy_s(nid1.szTip, L"Win32Explorer");
            Shell_NotifyIconW(NIM_ADD, &nid1);

            NOTIFYICONDATAW nid2 = { sizeof(NOTIFYICONDATAW) };
            nid2.hWnd = inst->hTaskbar;
            nid2.uID = 5002;
            nid2.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
            nid2.uCallbackMessage = WM_TRAY_CALLBACK_TASKBAR;
            nid2.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_PREFERENCES));
            wcscpy_s(nid2.szTip, L"Elite Taskbar Preferences");
            Shell_NotifyIconW(NIM_ADD, &nid2);

            NOTIFYICONDATAW nid3 = { sizeof(NOTIFYICONDATAW) };
            nid3.hWnd = inst->hTaskbar;
            nid3.uID = 5003;
            nid3.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
            nid3.uCallbackMessage = WM_TRAY_CALLBACK_DESKTOP;
            nid3.hIcon = ExtractIconW(GetModuleHandleW(NULL), L"shell32.dll", 34);
            wcscpy_s(nid3.szTip, L"Desktop Replacement");
            Shell_NotifyIconW(NIM_ADD, &nid3);
        }
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
                    
                    info.hIcon = GetWindowIconFix(hwnd);
                    
                    info.cmdId = g_NextCmdId++;
                    g_TaskButtons.push_back(info);
                    AddTaskButton(g_TaskButtons.back());
                }
            }
        }
        return TRUE;
    }, 0);

    bool desktopReplaceEnabled = false;
    HKEY hKeyDesktop;
    if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Settings", 0, KEY_READ, &hKeyDesktop) == ERROR_SUCCESS) { // - Draftsman-Dan
        DWORD dwVal = 0;
        DWORD cbData = sizeof(DWORD);
        if (RegQueryValueExW(hKeyDesktop, L"DesktopReplacementEnabled", NULL, NULL, (LPBYTE)&dwVal, &cbData) == ERROR_SUCCESS) {
            desktopReplaceEnabled = (dwVal == 1);
        }
        RegCloseKey(hKeyDesktop);
    }

    bool forceProgman = false;
    HKEY hKeyForce;
    if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Settings", 0, KEY_READ, &hKeyForce) == ERROR_SUCCESS) { // - Draftsman-Dan
        DWORD dwVal = 0;
        DWORD cbData = sizeof(DWORD);
        if (RegQueryValueExW(hKeyForce, L"ForceProgmanAllDisplays", NULL, NULL, (LPBYTE)&dwVal, &cbData) == ERROR_SUCCESS) {
            forceProgman = (dwVal == 1);
        }
        RegCloseKey(hKeyForce);
    }

    if ((g_Config.Mode == TaskbarMode::Replace || forceProgman) && desktopReplaceEnabled) {
        DesktopWindow::Initialize();
    }

    if (g_uTaskbarCreatedMsg != 0) {
        PostMessageW(HWND_BROADCAST, g_uTaskbarCreatedMsg, 0, 0);
        SendNotifyMessageW(HWND_BROADCAST, g_uTaskbarCreatedMsg, 0, 0);
    }

    return true;
}


void TaskbarWindow::RunMessageLoop() {
    MSG msg = {0};
    while (GetMessageW(&msg, NULL, 0, 0)) {
        if (DesktopWindow::TranslateAccelerator(&msg)) {
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void TaskbarWindow::Cleanup() {
    for (auto* band : g_FolderBands) {
        delete band;
    }
    g_FolderBands.clear();

    DesktopWindow::Cleanup();
    for (auto* inst : g_Taskbars) {
        APPBARDATA abd = {0};
        abd.cbSize = sizeof(APPBARDATA);
        abd.hWnd = inst->hTaskbar;
        SHAppBarMessage(ABM_REMOVE, &abd);
        if (inst->startButton) {
            delete inst->startButton;
        }
        
        if (inst->bStolenSysPager && inst->hToolbar && inst->hNativeTrayNotify) {
            SetParent(inst->hToolbar, inst->hNativeTrayNotify);
            SetWindowPos(inst->hToolbar, NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE);
        }
        
        HMONITOR hPrimaryMon = MonitorFromWindow(FindWindowW(L"Shell_TrayWnd", NULL), MONITOR_DEFAULTTOPRIMARY);
        if (inst->hMonitor == hPrimaryMon) {
            NOTIFYICONDATAW nid1 = { sizeof(NOTIFYICONDATAW) };
            nid1.hWnd = inst->hTaskbar;
            nid1.uID = 5001;
            Shell_NotifyIconW(NIM_DELETE, &nid1);

            NOTIFYICONDATAW nid2 = { sizeof(NOTIFYICONDATAW) };
            nid2.hWnd = inst->hTaskbar;
            nid2.uID = 5002;
            Shell_NotifyIconW(NIM_DELETE, &nid2);

            NOTIFYICONDATAW nid3 = { sizeof(NOTIFYICONDATAW) };
            nid3.hWnd = inst->hTaskbar;
            nid3.uID = 5003;
            Shell_NotifyIconW(NIM_DELETE, &nid3);
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
        if (!::g_IsRestarting) {
            ShellExecuteW(NULL, L"open", L"explorer.exe", NULL, NULL, SW_SHOWNORMAL);
        }
    }
}





bool IsEliteTaskbarRunning() {
    return !g_Taskbars.empty();
}

