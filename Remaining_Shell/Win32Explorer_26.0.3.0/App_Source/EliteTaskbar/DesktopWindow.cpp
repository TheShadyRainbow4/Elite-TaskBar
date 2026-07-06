#include "DesktopWindow.h"
#include "Config.h"
#include "Logger.h"
#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <gdiplus.h>
#include <vector>
#include <string>
#include <algorithm>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "uxtheme.lib")
#include <uxtheme.h>
#include <commoncontrols.h>

#define WM_SHELLCHANGE (WM_USER + 101)
#define WM_POPULATE_GRID (WM_USER + 102)
#define TIMER_DEBOUNCE_REFRESH 1001

static HWND s_hProgman = NULL;
static HWND s_hNativeProgman = NULL;
static HWND s_hNativeWorkerW = NULL;
static Gdiplus::Bitmap* s_pCachedWallpaper = nullptr;
static std::wstring s_cachedWallpaperPath = L"";
static int s_cachedStyle = -1;
static bool s_cachedTile = false;
static bool s_cachedDrawWallpaper = true;
static HIMAGELIST s_hCustomImageList = nullptr;

#define TIMER_SLIDESHOW 1002

static std::wstring GetThemeDirectory() {
    HKEY hKey;
    wchar_t themePathVal[MAX_PATH] = {0};
    DWORD cbData = sizeof(themePathVal);
    if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExW(hKey, L"DesktopThemePath", NULL, NULL, (LPBYTE)themePathVal, &cbData);
        RegCloseKey(hKey);
    }
    
    if (wcslen(themePathVal) > 0) {
        std::wstring pathStr(themePathVal);
        if (PathIsDirectoryW(pathStr.c_str())) {
            return pathStr;
        } else if (PathFileExistsW(pathStr.c_str())) {
            wchar_t wallpaperVal[MAX_PATH] = {0};
            GetPrivateProfileStringW(L"Control Panel\\Desktop", L"Wallpaper", L"", wallpaperVal, MAX_PATH, pathStr.c_str());
            if (wcslen(wallpaperVal) > 0) {
                wchar_t expandedWallpaper[MAX_PATH] = {0};
                ExpandEnvironmentStringsW(wallpaperVal, expandedWallpaper, MAX_PATH);
                std::wstring wallPath(expandedWallpaper);
                size_t lastSlash = wallPath.find_last_of(L'\\');
                if (lastSlash != std::wstring::npos) {
                    std::wstring parentDir = wallPath.substr(0, lastSlash);
                    if (PathIsDirectoryW(parentDir.c_str())) {
                        return parentDir;
                    }
                }
            }
            size_t lastSlash = pathStr.find_last_of(L'\\');
            if (lastSlash != std::wstring::npos) {
                return pathStr.substr(0, lastSlash);
            }
        } else {
            size_t lastSlash = pathStr.find_last_of(L'\\');
            if (lastSlash != std::wstring::npos) {
                return pathStr.substr(0, lastSlash);
            }
        }
    }
    
    wchar_t winDir[MAX_PATH];
    if (GetWindowsDirectoryW(winDir, MAX_PATH) > 0) {
        std::wstring fallback = std::wstring(winDir) + L"\\Web\\Wallpaper";
        if (PathFileExistsW(fallback.c_str())) {
            return fallback;
        }
    }
    return L"";
}

static void AdvanceSlideshow(HWND hwnd) {
    std::wstring dir = GetThemeDirectory();
    if (dir.empty() || !PathFileExistsW(dir.c_str())) return;
    
    std::vector<std::wstring> images;
    const wchar_t* extensions[] = { L"\\*.jpg", L"\\*.png", L"\\*.bmp", L"\\*.jpeg" };
    for (const auto& ext : extensions) {
        std::wstring query = dir + ext;
        WIN32_FIND_DATAW fd;
        HANDLE hFind = FindFirstFileW(query.c_str(), &fd);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                images.push_back(dir + L"\\" + fd.cFileName);
            } while (FindNextFileW(hFind, &fd));
            FindClose(hFind);
        }
    }
    
    if (images.empty()) return;
    
    std::sort(images.begin(), images.end());
    
    int currentIndex = -1;
    for (size_t i = 0; i < images.size(); i++) {
        if (_wcsicmp(images[i].c_str(), s_cachedWallpaperPath.c_str()) == 0) {
            currentIndex = (int)i;
            break;
        }
    }
    
    int nextIndex = (currentIndex + 1) % images.size();
    std::wstring nextWallpaper = images[nextIndex];
    
    if (s_pCachedWallpaper) {
        delete s_pCachedWallpaper;
        s_pCachedWallpaper = nullptr;
    }
    s_cachedWallpaperPath = nextWallpaper;
    s_pCachedWallpaper = new Gdiplus::Bitmap(s_cachedWallpaperPath.c_str());
    if (s_pCachedWallpaper->GetLastStatus() != Gdiplus::Ok) {
        delete s_pCachedWallpaper;
        s_pCachedWallpaper = nullptr;
    }
    
    InvalidateRect(hwnd, NULL, TRUE);
}

LRESULT CALLBACK ProgmanWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK DefViewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void PopulateDesktopGrid(HWND hwndListView);
void SaveIconPositions(HWND hwndListView);
ULONG RegisterDesktopChangeWatcher(HWND hwndTarget);
void DrawWallpaper(HWND hwnd, HDC hdc, int scrW, int scrH);

namespace DesktopWindow {
    bool Initialize() {
        Logger::Log(L"DesktopWindow::Initialize starting.");
        
        bool forceProgman = false;
        HKEY hKeyForce;
        if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKeyForce) == ERROR_SUCCESS) {
            DWORD dwVal = 0;
            DWORD cbData = sizeof(DWORD);
            if (RegQueryValueExW(hKeyForce, L"ForceProgmanAllDisplays", NULL, NULL, (LPBYTE)&dwVal, &cbData) == ERROR_SUCCESS) {
                forceProgman = (dwVal == 1);
            }
            RegCloseKey(hKeyForce);
        }
        Logger::Log(forceProgman ? L"ForceProgmanAllDisplays is active." : L"ForceProgmanAllDisplays is inactive.");
        
        HINSTANCE hInst = GetModuleHandleW(NULL);
        
        // Coexistence: find and hide native desktop windows
        s_hNativeProgman = FindWindowW(L"Progman", NULL);
        if (s_hNativeProgman) {
            Logger::Log(L"Native Progman window detected. Hiding.");
            ShowWindow(s_hNativeProgman, SW_HIDE);
        }
        
        HWND hWorker = NULL;
        while ((hWorker = FindWindowExW(NULL, hWorker, L"WorkerW", NULL)) != NULL) {
            HWND hDefView = FindWindowExW(hWorker, NULL, L"SHELLDLL_DefView", NULL);
            if (hDefView) {
                Logger::Log(L"Native WorkerW desktop window detected. Hiding.");
                s_hNativeWorkerW = hWorker;
                ShowWindow(hWorker, SW_HIDE);
            }
        }

        // Register custom Progman class
        WNDCLASSEXW wcProgman = { 0 };
        wcProgman.cbSize = sizeof(WNDCLASSEXW);
        wcProgman.style = CS_DBLCLKS;
        wcProgman.lpfnWndProc = ProgmanWndProc;
        wcProgman.hInstance = hInst;
        wcProgman.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
        wcProgman.hbrBackground = NULL; // We paint our own background
        wcProgman.lpszClassName = L"Progman";
        
        if (!RegisterClassExW(&wcProgman)) {
            Logger::Log(L"Warning: Progman class registration failed or already exists.");
        }

        // Register custom DefView class
        WNDCLASSEXW wcDefView = { 0 };
        wcDefView.cbSize = sizeof(WNDCLASSEXW);
        wcDefView.style = CS_DBLCLKS;
        wcDefView.lpfnWndProc = DefViewWndProc;
        wcDefView.hInstance = hInst;
        wcDefView.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
        wcDefView.hbrBackground = NULL;
        wcDefView.lpszClassName = L"SHELLDLL_DefView";
        
        if (!RegisterClassExW(&wcDefView)) {
            Logger::Log(L"Warning: SHELLDLL_DefView class registration failed or already exists.");
        }

        // Get full virtual screen dimensions to span all monitors
        int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
        int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
        int cx = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        int cy = GetSystemMetrics(SM_CYVIRTUALSCREEN);

        s_hProgman = CreateWindowExW(
            WS_EX_TOOLWINDOW,
            L"Progman",
            L"Program Manager",
            WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
            x, y, cx, cy,
            NULL, NULL, hInst, NULL
        );

        if (!s_hProgman) {
            Logger::LogError(L"Failed to create custom Progman window.", GetLastError());
            return false;
        }

        // Force bottom Z-order positioning
        SetWindowPos(s_hProgman, HWND_BOTTOM, x, y, cx, cy, SWP_SHOWWINDOW | SWP_NOACTIVATE);
        Logger::Log(L"Custom Progman window initialized successfully.");
        return true;
    }

    void Cleanup() {
        Logger::Log(L"DesktopWindow::Cleanup starting.");
        
        if (s_pCachedWallpaper) {
            delete s_pCachedWallpaper;
            s_pCachedWallpaper = nullptr;
        }
        s_cachedWallpaperPath.clear();
        s_cachedStyle = -1;
        s_cachedTile = false;
        s_cachedDrawWallpaper = true;

        if (s_hProgman) {
            DestroyWindow(s_hProgman);
            s_hProgman = NULL;
        }

        HINSTANCE hInst = GetModuleHandleW(NULL);
        UnregisterClassW(L"Progman", hInst);
        UnregisterClassW(L"SHELLDLL_DefView", hInst);

        // Restore native desktop windows if hidden
        if (s_hNativeProgman && IsWindow(s_hNativeProgman)) {
            Logger::Log(L"Restoring native Progman window.");
            ShowWindow(s_hNativeProgman, SW_SHOW);
        }
        if (s_hNativeWorkerW && IsWindow(s_hNativeWorkerW)) {
            Logger::Log(L"Restoring native WorkerW window.");
            ShowWindow(s_hNativeWorkerW, SW_SHOW);
        }
    }

    HWND GetHWND() {
        return s_hProgman;
    }
}

LRESULT CALLBACK ProgmanWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND hwndDefView = NULL;
    switch (uMsg) {
    case WM_CREATE: {
        CREATESTRUCTW* pcs = (CREATESTRUCTW*)lParam;
        hwndDefView = CreateWindowExW(
            0, L"SHELLDLL_DefView", L"",
            WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
            0, 0, pcs->cx, pcs->cy, hwnd, (HMENU)100, pcs->hInstance, NULL
        );
        return 0;
    }
    case WM_SIZE: {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        if (hwndDefView) {
            SetWindowPos(hwndDefView, NULL, 0, 0, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
        }
        return 0;
    }
    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;

    case WM_WINDOWPOSCHANGING: {
        WINDOWPOS* lpw = (WINDOWPOS*)lParam;
        lpw->hwndInsertAfter = HWND_BOTTOM;
        lpw->flags &= ~SWP_NOZORDER;
        break;
    }
    case WM_DISPLAYCHANGE: {
        int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
        int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
        int cx = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        int cy = GetSystemMetrics(SM_CYVIRTUALSCREEN);
        SetWindowPos(hwnd, HWND_BOTTOM, x, y, cx, cy, SWP_NOACTIVATE | SWP_SHOWWINDOW);
        if (hwndDefView) {
            SetWindowPos(hwndDefView, NULL, 0, 0, cx, cy, SWP_NOZORDER | SWP_NOACTIVATE);
        }
        return 0;
    }
    case WM_TIMER: {
        if (wParam == TIMER_SLIDESHOW) {
            AdvanceSlideshow(hwnd);
            return 0;
        }
        break;
    }
    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hwnd, &rc);
        DrawWallpaper(hwnd, hdc, rc.right - rc.left, rc.bottom - rc.top);
        return TRUE;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        DrawWallpaper(hwnd, hdc, rc.right - rc.left, rc.bottom - rc.top);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_SETTINGCHANGE: {
        HKEY hKey;
        DWORD useNativeWallpaperVal = 1;
        DWORD cbData = sizeof(DWORD);
        if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            RegQueryValueExW(hKey, L"UseNativeWallpaperEngine", NULL, NULL, (LPBYTE)&useNativeWallpaperVal, &cbData);
            RegCloseKey(hKey);
        }
        if (useNativeWallpaperVal == 1) {
            if (s_pCachedWallpaper) {
                delete s_pCachedWallpaper;
                s_pCachedWallpaper = nullptr;
            }
            s_cachedWallpaperPath = L"";
            s_cachedStyle = -1;
            s_cachedTile = false;
            KillTimer(hwnd, TIMER_SLIDESHOW);
            InvalidateRect(hwnd, NULL, TRUE);
        }
        break;
    }
    case WM_DESTROY:
        KillTimer(hwnd, TIMER_SLIDESHOW);
        s_hProgman = NULL;
        break;
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK DefViewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND hwndListView = NULL;
    static ULONG uNotifyId = 0;

    switch (uMsg) {
    case WM_CREATE: {
        CREATESTRUCTW* pcs = (CREATESTRUCTW*)lParam;
        
        hwndListView = CreateWindowExW(
            WS_EX_TRANSPARENT, WC_LISTVIEWW, L"",
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
            LVS_ICON | LVS_ALIGNLEFT | LVS_SHAREIMAGELISTS | LVS_EDITLABELS,
            0, 0, pcs->cx, pcs->cy, hwnd, (HMENU)200, pcs->hInstance, NULL
        );
        if (!hwndListView) return -1;

        ListView_SetExtendedListViewStyle(hwndListView, LVS_EX_DOUBLEBUFFER | LVS_EX_BORDERSELECT);
        ListView_SetBkColor(hwndListView, CLR_NONE);
        ListView_SetTextBkColor(hwndListView, CLR_NONE);
        ListView_SetTextColor(hwndListView, RGB(255, 255, 255));
        SetWindowTheme(hwndListView, L"Explorer", NULL);

        // Retrieve and bind system image list (extralarge 48x48)
        HIMAGELIST hSysIL = NULL;
        IImageList* pSysIL = NULL;
        if (SUCCEEDED(SHGetImageList(SHIL_EXTRALARGE, IID_IImageList, (void**)&pSysIL))) {
            hSysIL = (HIMAGELIST)pSysIL;
        } else {
            SHFILEINFOW sfiNormal = { 0 };
            hSysIL = (HIMAGELIST)SHGetFileInfoW(L"", 0, &sfiNormal, sizeof(sfiNormal), SHGFI_SYSICONINDEX | SHGFI_LARGEICON);
        }
        if (hSysIL) {
            ImageList_SetBkColor(hSysIL, CLR_NONE);
            ListView_SetImageList(hwndListView, hSysIL, LVSIL_NORMAL);
        }
        if (pSysIL) {
            pSysIL->Release();
        }

        PostMessageW(hwnd, WM_POPULATE_GRID, 0, 0);
        uNotifyId = RegisterDesktopChangeWatcher(hwnd);
        return 0;
    }
    case WM_POPULATE_GRID:
        PopulateDesktopGrid(hwndListView);
        return 0;
    case WM_SIZE: {
        if (hwndListView) {
            SetWindowPos(hwndListView, NULL, 0, 0, LOWORD(lParam), HIWORD(lParam), SWP_NOZORDER | SWP_NOACTIVATE);
        }
        return 0;
    }
    case WM_ERASEBKGND: {
        // Forward erasing to parent Progman for wallpaper continuity
        HWND hwndParent = GetParent(hwnd);
        HDC hdc = (HDC)wParam;
        POINT pt = { 0, 0 };
        MapWindowPoints(hwnd, hwndParent, &pt, 1);
        OffsetWindowOrgEx(hdc, pt.x, pt.y, &pt);
        LRESULT ret = SendMessageW(hwndParent, WM_ERASEBKGND, wParam, lParam);
        SetWindowOrgEx(hdc, pt.x, pt.y, NULL);
        return ret;
    }
    case WM_SHELLCHANGE: {
        HANDLE hLock = (HANDLE)wParam;
        DWORD dwProcId = (DWORD)lParam;
        if (hLock) {
            PIDLIST_ABSOLUTE* ppidl = nullptr;
            LONG lEvent = 0;
            HANDLE hNotifyLock = SHChangeNotification_Lock(hLock, dwProcId, &ppidl, &lEvent);
            if (hNotifyLock) {
                SHChangeNotification_Unlock(hNotifyLock);
            }
        }
        SetTimer(hwnd, TIMER_DEBOUNCE_REFRESH, 100, NULL);
        return 0;
    }
    case WM_TIMER:
        if (wParam == TIMER_DEBOUNCE_REFRESH) {
            KillTimer(hwnd, TIMER_DEBOUNCE_REFRESH);
            PopulateDesktopGrid(hwndListView);
        }
        return 0;
    case WM_NOTIFY: {
        LPNMHDR pnmh = (LPNMHDR)lParam;
        if (pnmh->hwndFrom == hwndListView) {
            switch (pnmh->code) {
            case NM_DBLCLK: {
                LPNMLISTVIEW pnmlv = (LPNMLISTVIEW)lParam;
                if (pnmlv->iItem != -1) {
                    LVITEMW lvi = { 0 };
                    lvi.mask = LVIF_PARAM;
                    lvi.iItem = pnmlv->iItem;
                     if (SendMessageW(hwndListView, LVM_GETITEMW, 0, (LPARAM)&lvi)) {
                        PITEMID_CHILD pidl = (PITEMID_CHILD)lvi.lParam;
                        if (pidl) {
                            SHELLEXECUTEINFOW sei = { 0 };
                            sei.cbSize = sizeof(sei);
                            sei.fMask = SEE_MASK_IDLIST;
                            sei.hwnd = hwnd;
                            sei.nShow = SW_SHOWNORMAL;
                            sei.lpIDList = (LPVOID)pidl;
                            ShellExecuteExW(&sei);
                        }
                    }
                }
                break;
            }
            case LVN_ENDLABELEDITW: {
                NMLVDISPINFOW* pdi = (NMLVDISPINFOW*)lParam;
                if (pdi->item.pszText != NULL && pdi->item.iItem != -1) {
                    PITEMID_CHILD pidlOld = (PITEMID_CHILD)pdi->item.lParam;
                    if (pidlOld) {
                        IShellFolder* pDesktopFolder = NULL;
                        if (SUCCEEDED(SHGetDesktopFolder(&pDesktopFolder))) {
                            PITEMID_CHILD pidlNew = nullptr;
                            HRESULT hr = pDesktopFolder->SetNameOf(hwndListView, pidlOld, pdi->item.pszText, SHGDN_INFOLDER, &pidlNew);
                            pDesktopFolder->Release();
                            if (SUCCEEDED(hr)) {
                                CoTaskMemFree(pidlOld);
                                pdi->item.lParam = (LPARAM)pidlNew;
                                return TRUE;
                            }
                        }
                    }
                }
                return FALSE;
            }
            }
        }
        return 0;
    }
    case WM_DESTROY: {
        if (uNotifyId) {
            SHChangeNotifyDeregister(uNotifyId);
            uNotifyId = 0;
        }
        if (hwndListView) {
            SaveIconPositions(hwndListView);
            int count = ListView_GetItemCount(hwndListView);
            for (int i = 0; i < count; ++i) {
                LVITEMW lvi = { 0 };
                lvi.mask = LVIF_PARAM;
                lvi.iItem = i;
                 if (SendMessageW(hwndListView, LVM_GETITEMW, 0, (LPARAM)&lvi)) {
                    PITEMID_CHILD pidl = (PITEMID_CHILD)lvi.lParam;
                    if (pidl) CoTaskMemFree(pidl);
                }
            }
        }
        if (s_hCustomImageList) {
            ImageList_Destroy(s_hCustomImageList);
            s_hCustomImageList = nullptr;
        }
        break;
    }
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

struct SavedPos {
    std::wstring name;
    POINT pt;
};

void SaveIconPositions(HWND hwndListView) {
    int itemCount = ListView_GetItemCount(hwndListView);
    if (itemCount <= 0) return;
    std::vector<SavedPos> savedPositions;
    for (int i = 0; i < itemCount; ++i) {
        POINT pt;
        if (ListView_GetItemPosition(hwndListView, i, &pt)) {
            wchar_t szItemName[MAX_PATH] = { 0 };
            LVITEMW lvi = { 0 };
            lvi.mask = LVIF_TEXT;
            lvi.iItem = i;
            lvi.pszText = szItemName;
            lvi.cchTextMax = MAX_PATH;
             if (SendMessageW(hwndListView, LVM_GETITEMW, 0, (LPARAM)&lvi)) {
                SavedPos sp;
                sp.name = szItemName;
                sp.pt = pt;
                savedPositions.push_back(sp);
            }
        }
    }

    HKEY hKeyPos;
    if (RegCreateKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced\\DesktopIconPositions", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKeyPos, NULL) == ERROR_SUCCESS) {
        for (const auto& sp : savedPositions) {
            DWORD val = (sp.pt.x << 16) | (sp.pt.y & 0xFFFF);
            RegSetValueExW(hKeyPos, sp.name.c_str(), 0, REG_DWORD, (const BYTE*)&val, sizeof(DWORD));
        }
        RegCloseKey(hKeyPos);
    }
}

void PopulateDesktopGrid(HWND hwndListView) {
    // Determine if desktop icons and thumbnails are enabled
    bool desktopIconsEnabled = true;
    bool desktopThumbnailsEnabled = true;
    HKEY hKeySettings;
    if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKeySettings) == ERROR_SUCCESS) {
        DWORD dwVal = 1;
        DWORD cbData = sizeof(DWORD);
        if (RegQueryValueExW(hKeySettings, L"DesktopIconsEnabled", NULL, NULL, (LPBYTE)&dwVal, &cbData) == ERROR_SUCCESS) {
            desktopIconsEnabled = (dwVal == 1);
        }
        dwVal = 1;
        cbData = sizeof(DWORD);
        if (RegQueryValueExW(hKeySettings, L"DesktopThumbnailsEnabled", NULL, NULL, (LPBYTE)&dwVal, &cbData) == ERROR_SUCCESS) {
            desktopThumbnailsEnabled = (dwVal == 1);
        }
        RegCloseKey(hKeySettings);
    }

    if (!desktopIconsEnabled) {
        ShowWindow(hwndListView, SW_HIDE);
        return;
    } else {
        ShowWindow(hwndListView, SW_SHOW);
    }

    // Save positions before clearing
    SaveIconPositions(hwndListView);

    // Load saved positions from registry
    std::vector<SavedPos> savedPositions;
    HKEY hKeyPos;
    if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced\\DesktopIconPositions", 0, KEY_READ, &hKeyPos) == ERROR_SUCCESS) {
        WCHAR valName[MAX_PATH];
        DWORD cbValName = MAX_PATH;
        DWORD dwType = 0;
        DWORD dwData = 0;
        DWORD cbData = sizeof(DWORD);
        DWORD dwIndex = 0;
        while (RegEnumValueW(hKeyPos, dwIndex, valName, &cbValName, NULL, &dwType, (LPBYTE)&dwData, &cbData) == ERROR_SUCCESS) {
            SavedPos sp;
            sp.name = valName;
            sp.pt.x = (dwData >> 16);
            sp.pt.y = (dwData & 0xFFFF);
            savedPositions.push_back(sp);
            
            dwIndex++;
            cbValName = MAX_PATH;
            cbData = sizeof(DWORD);
        }
        RegCloseKey(hKeyPos);
    }

    // Clean up existing item parameters (PIDLs)
    int count = ListView_GetItemCount(hwndListView);
    for (int i = 0; i < count; ++i) {
        LVITEMW lvi = { 0 };
        lvi.mask = LVIF_PARAM;
        lvi.iItem = i;
        if (SendMessageW(hwndListView, LVM_GETITEMW, 0, (LPARAM)&lvi)) {
            PITEMID_CHILD pidl = (PITEMID_CHILD)lvi.lParam;
            if (pidl) CoTaskMemFree(pidl);
        }
    }
    ListView_DeleteAllItems(hwndListView);

    // Recreate custom image list if thumbnails are enabled
    if (s_hCustomImageList) {
        ListView_SetImageList(hwndListView, NULL, LVSIL_NORMAL);
        ImageList_Destroy(s_hCustomImageList);
        s_hCustomImageList = nullptr;
    }

    if (desktopThumbnailsEnabled) {
        s_hCustomImageList = ImageList_Create(48, 48, ILC_COLOR32 | ILC_MASK, 0, 10);
        ImageList_SetBkColor(s_hCustomImageList, CLR_NONE);
        ListView_SetImageList(hwndListView, s_hCustomImageList, LVSIL_NORMAL);
    } else {
        // Just bind the standard system image list
        HIMAGELIST hSysIL = NULL;
        IImageList* pSysIL = nullptr;
        if (SUCCEEDED(SHGetImageList(SHIL_EXTRALARGE, IID_IImageList, (void**)&pSysIL))) {
            hSysIL = (HIMAGELIST)pSysIL;
        } else {
            SHFILEINFOW sfiNormal = { 0 };
            hSysIL = (HIMAGELIST)SHGetFileInfoW(L"", 0, &sfiNormal, sizeof(sfiNormal), SHGFI_SYSICONINDEX | SHGFI_LARGEICON);
        }
        if (hSysIL) {
            ImageList_SetBkColor(hSysIL, CLR_NONE);
            ListView_SetImageList(hwndListView, hSysIL, LVSIL_NORMAL);
        }
        if (pSysIL) {
            pSysIL->Release();
        }
    }

    IShellFolder* pDesktopFolder = nullptr;
    if (FAILED(SHGetDesktopFolder(&pDesktopFolder))) {
        Logger::Log(L"PopulateDesktopGrid: Failed to get desktop folder.");
        return;
    }

    IEnumIDList* pEnumIDList = nullptr;
    // Query both directories and files, including hidden ones
    HRESULT hrEnum = pDesktopFolder->EnumObjects(
        hwndListView, 
        SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN, 
        &pEnumIDList
    );

    if (SUCCEEDED(hrEnum) && pEnumIDList) {
        ULONG uFetched = 0;
        PITEMID_CHILD pidlChild = nullptr;
        while (pEnumIDList->Next(1, &pidlChild, &uFetched) == S_OK && uFetched == 1) {
            STRRET strRet;
            wchar_t szName[MAX_PATH] = L"";
            if (SUCCEEDED(pDesktopFolder->GetDisplayNameOf(pidlChild, SHGDN_INFOLDER, &strRet))) {
                StrRetToBufW(&strRet, pidlChild, szName, MAX_PATH);
            }

            int itemImageIndex = -1;

            if (desktopThumbnailsEnabled && s_hCustomImageList) {
                bool addedThumbnail = false;
                IShellItem* pShellItem = nullptr;
                PIDLIST_ABSOLUTE pidlAbsolute = nullptr;
                PIDLIST_ABSOLUTE pidlDesktop = nullptr;
                if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &pidlDesktop))) {
                    pidlAbsolute = ILCombine(pidlDesktop, pidlChild);
                    CoTaskMemFree(pidlDesktop);
                }

                if (pidlAbsolute) {
                    if (SUCCEEDED(SHCreateItemFromIDList(pidlAbsolute, IID_PPV_ARGS(&pShellItem))) && pShellItem) {
                        IShellItemImageFactory* pFactory = nullptr;
                        if (SUCCEEDED(pShellItem->QueryInterface(IID_PPV_ARGS(&pFactory))) && pFactory) {
                            SIZE size = { 48, 48 };
                            HBITMAP hbmp = nullptr;
                            if (SUCCEEDED(pFactory->GetImage(size, SIIGBF_THUMBNAILONLY, &hbmp)) && hbmp) {
                                int thumbIdx = ImageList_Add(s_hCustomImageList, hbmp, NULL);
                                if (thumbIdx != -1) {
                                    itemImageIndex = thumbIdx;
                                    addedThumbnail = true;
                                }
                                DeleteObject(hbmp);
                            }
                            pFactory->Release();
                        }
                        pShellItem->Release();
                    }
                    ILFree(pidlAbsolute);
                }

                if (!addedThumbnail) {
                    // Fetch default icon from system image list and add to our custom image list
                    HIMAGELIST hSysIL = NULL;
                    IImageList* pSysIL = nullptr;
                    SHFILEINFOW sfi = { 0 };
                    SHGetFileInfoW((LPCWSTR)pidlChild, 0, &sfi, sizeof(sfi), SHGFI_PIDL | SHGFI_SYSICONINDEX);
                    if (SUCCEEDED(SHGetImageList(SHIL_EXTRALARGE, IID_IImageList, (void**)&pSysIL))) {
                        hSysIL = (HIMAGELIST)pSysIL;
                    } else {
                        SHFILEINFOW sfiNormal = { 0 };
                        hSysIL = (HIMAGELIST)SHGetFileInfoW(L"", 0, &sfiNormal, sizeof(sfiNormal), SHGFI_SYSICONINDEX | SHGFI_LARGEICON);
                    }
                    
                    if (hSysIL) {
                        HICON hIcon = ImageList_GetIcon(hSysIL, sfi.iIcon, ILD_NORMAL);
                        if (hIcon) {
                            itemImageIndex = ImageList_AddIcon(s_hCustomImageList, hIcon);
                            DestroyIcon(hIcon);
                        }
                    }
                    if (pSysIL) {
                        pSysIL->Release();
                    }
                }
            } else {
                SHFILEINFOW sfi = { 0 };
                SHGetFileInfoW((LPCWSTR)pidlChild, 0, &sfi, sizeof(sfi), SHGFI_PIDL | SHGFI_SYSICONINDEX);
                itemImageIndex = sfi.iIcon;
            }

            LVITEMW lvi = { 0 };
            lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
            lvi.iItem = ListView_GetItemCount(hwndListView);
            lvi.iSubItem = 0;
            lvi.pszText = szName;
            lvi.iImage = itemImageIndex;
            lvi.lParam = (LPARAM)pidlChild;
             SendMessageW(hwndListView, LVM_INSERTITEMW, 0, (LPARAM)&lvi);
        }
        pEnumIDList->Release();
    }
    pDesktopFolder->Release();

    // Restore saved positions
    int newCount = ListView_GetItemCount(hwndListView);
    bool restoredAny = false;
    for (int i = 0; i < newCount; ++i) {
        wchar_t szItemName[MAX_PATH] = { 0 };
        LVITEMW lvi = { 0 };
        lvi.mask = LVIF_TEXT;
        lvi.iItem = i;
        lvi.pszText = szItemName;
        lvi.cchTextMax = MAX_PATH;
        if (SendMessageW(hwndListView, LVM_GETITEMW, 0, (LPARAM)&lvi)) {
            for (const auto& sp : savedPositions) {
                if (sp.name == szItemName) {
                    ListView_SetItemPosition(hwndListView, i, sp.pt.x, sp.pt.y);
                    restoredAny = true;
                    break;
                }
            }
        }
    }

    if (!restoredAny) {
        ListView_Arrange(hwndListView, LVA_DEFAULT);
    }
}

ULONG RegisterDesktopChangeWatcher(HWND hwndTarget) {
    PIDLIST_ABSOLUTE pidlDesktop = nullptr;
    if (FAILED(SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &pidlDesktop))) return 0;

    SHChangeNotifyEntry entries[1];
    entries[0].pidl = pidlDesktop;
    entries[0].fRecursive = FALSE;

    ULONG uRegisterId = SHChangeNotifyRegister(
        hwndTarget,
        SHCNRF_ShellLevel | SHCNRF_InterruptLevel | SHCNRF_NewDelivery,
        SHCNE_CREATE | SHCNE_DELETE | SHCNE_RENAMEITEM | SHCNE_RENAMEFOLDER | SHCNE_UPDATEITEM | SHCNE_UPDATEDIR,
        WM_SHELLCHANGE,
        1,
        entries
    );

    CoTaskMemFree(pidlDesktop);
    return uRegisterId;
}

struct MonitorWallpaperData {
    HDC hdc;
    Gdiplus::Bitmap* pBitmap;
    int style;
    bool tile;
    COLORREF bgColor;
};

static BOOL CALLBACK DrawWallpaperMonitorProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    MonitorWallpaperData* pData = (MonitorWallpaperData*)dwData;
    
    int virtualX = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int virtualY = GetSystemMetrics(SM_YVIRTUALSCREEN);
    
    int monLeft = lprcMonitor->left - virtualX;
    int monTop = lprcMonitor->top - virtualY;
    int monWidth = lprcMonitor->right - lprcMonitor->left;
    int monHeight = lprcMonitor->bottom - lprcMonitor->top;
    
    Gdiplus::Graphics graphics(pData->hdc);
    graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
    
    if (pData->tile) {
        Gdiplus::TextureBrush brush(pData->pBitmap);
        brush.SetWrapMode(Gdiplus::WrapModeTile);
        brush.TranslateTransform((Gdiplus::REAL)monLeft, (Gdiplus::REAL)monTop);
        graphics.FillRectangle(&brush, monLeft, monTop, monWidth, monHeight);
    } else {
        int imgW = pData->pBitmap->GetWidth();
        int imgH = pData->pBitmap->GetHeight();
        
        int destX = monLeft, destY = monTop, destW = monWidth, destH = monHeight;
        int srcX = 0, srcY = 0, srcW = imgW, srcH = imgH;
        
        double imgAspect = (double)imgW / imgH;
        double scrAspect = (double)monWidth / monHeight;
        
        switch (pData->style) {
            case 0: // Center
                if (imgW <= monWidth) {
                    destX = monLeft + (monWidth - imgW) / 2;
                    destW = imgW;
                } else {
                    destX = monLeft;
                    destW = monWidth;
                    srcX = (imgW - monWidth) / 2;
                    srcW = monWidth;
                }
                if (imgH <= monHeight) {
                    destY = monTop + (monHeight - imgH) / 2;
                    destH = imgH;
                } else {
                    destY = monTop;
                    destH = monHeight;
                    srcY = (imgH - monHeight) / 2;
                    srcH = monHeight;
                }
                {
                    Gdiplus::SolidBrush bgBrush(Gdiplus::Color(GetRValue(pData->bgColor), GetGValue(pData->bgColor), GetBValue(pData->bgColor)));
                    graphics.FillRectangle(&bgBrush, monLeft, monTop, monWidth, monHeight);
                }
                break;
                
            case 2: // Stretch
                destX = monLeft; destY = monTop;
                destW = monWidth; destH = monHeight;
                break;
                
            case 6: // Fit
                if (imgAspect > scrAspect) {
                    destW = monWidth;
                    destH = (int)(monWidth / imgAspect);
                    destX = monLeft;
                    destY = monTop + (monHeight - destH) / 2;
                } else {
                    destH = monHeight;
                    destW = (int)(monHeight * imgAspect);
                    destY = monTop;
                    destX = monLeft + (monWidth - destW) / 2;
                }
                {
                    Gdiplus::SolidBrush bgBrush(Gdiplus::Color(GetRValue(pData->bgColor), GetGValue(pData->bgColor), GetBValue(pData->bgColor)));
                    graphics.FillRectangle(&bgBrush, monLeft, monTop, monWidth, monHeight);
                }
                break;
                
            case 10: // Fill
            default:
                if (imgAspect > scrAspect) {
                    destH = monHeight;
                    destW = (int)(monHeight * imgAspect);
                    destY = monTop;
                    destX = monLeft + (monWidth - destW) / 2;
                } else {
                    destW = monWidth;
                    destH = (int)(monWidth / imgAspect);
                    destX = monLeft;
                    destY = monTop + (monHeight - destH) / 2;
                }
                break;
        }
        
        graphics.DrawImage(pData->pBitmap, 
            Gdiplus::Rect(destX, destY, destW, destH),
            srcX, srcY, srcW, srcH, 
            Gdiplus::UnitPixel);
    }
    
    return TRUE;
}

void DrawWallpaper(HWND hwnd, HDC hdc, int scrW, int scrH) {
    HKEY hKey;
    DWORD drawWallpaper = 1;
    DWORD cbData = sizeof(DWORD);
    if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExW(hKey, L"DesktopWallpaperEnabled", NULL, NULL, (LPBYTE)&drawWallpaper, &cbData);
        RegCloseKey(hKey);
    }

    // Retrieve UseNativeWallpaperEngine
    DWORD useNativeWallpaperVal = 1;
    cbData = sizeof(DWORD);
    if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExW(hKey, L"UseNativeWallpaperEngine", NULL, NULL, (LPBYTE)&useNativeWallpaperVal, &cbData);
        RegCloseKey(hKey);
    }

    std::wstring wallpaperPath = L"";
    int style = 22; // default Span
    bool tile = false;
    DWORD slideshowEnabled = 0;
    DWORD slideshowInterval = 300;
    bool slideshowJustEnabled = false;

    if (useNativeWallpaperVal == 1) {
        // Kill custom slideshow timer
        KillTimer(hwnd, TIMER_SLIDESHOW);

        // Native wallpaper engine reads directly from HKCU\Control Panel\Desktop
        if (drawWallpaper) {
            if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                wchar_t szBuffer[MAX_PATH] = { 0 };
                DWORD dwSize = sizeof(szBuffer);
                if (RegQueryValueExW(hKey, L"Wallpaper", NULL, NULL, (LPBYTE)szBuffer, &dwSize) == ERROR_SUCCESS) {
                    wallpaperPath = szBuffer;
                }
                dwSize = sizeof(szBuffer);
                if (RegQueryValueExW(hKey, L"WallpaperStyle", NULL, NULL, (LPBYTE)szBuffer, &dwSize) == ERROR_SUCCESS) {
                    style = _wtoi(szBuffer);
                }
                dwSize = sizeof(szBuffer);
                if (RegQueryValueExW(hKey, L"TileWallpaper", NULL, NULL, (LPBYTE)szBuffer, &dwSize) == ERROR_SUCCESS) {
                    tile = (_wtoi(szBuffer) == 1);
                }
                RegCloseKey(hKey);
            }
        }

        // Determine if settings have changed for native engine
        bool settingsChanged = (drawWallpaper != (s_cachedDrawWallpaper ? 1 : 0)) ||
                               (wallpaperPath != s_cachedWallpaperPath) ||
                               (style != s_cachedStyle) ||
                               (tile != s_cachedTile);

        if (settingsChanged || !s_pCachedWallpaper) {
            if (s_pCachedWallpaper) {
                delete s_pCachedWallpaper;
                s_pCachedWallpaper = nullptr;
            }

            s_cachedDrawWallpaper = (drawWallpaper == 1);
            s_cachedStyle = style;
            s_cachedTile = tile;
            s_cachedWallpaperPath = wallpaperPath;

            if (s_cachedDrawWallpaper && !s_cachedWallpaperPath.empty() && PathFileExistsW(s_cachedWallpaperPath.c_str())) {
                s_pCachedWallpaper = new Gdiplus::Bitmap(s_cachedWallpaperPath.c_str());
                if (s_pCachedWallpaper->GetLastStatus() != Gdiplus::Ok) {
                    delete s_pCachedWallpaper;
                    s_pCachedWallpaper = nullptr;
                }
            }
        }
    } else {
        // Custom wallpaper engine (original legacy code)
        if (drawWallpaper) {
            if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                wchar_t szBuffer[MAX_PATH] = { 0 };
                DWORD dwSize = sizeof(szBuffer);
                if (RegQueryValueExW(hKey, L"Wallpaper", NULL, NULL, (LPBYTE)szBuffer, &dwSize) == ERROR_SUCCESS) {
                    wallpaperPath = szBuffer;
                }
                dwSize = sizeof(szBuffer);
                if (RegQueryValueExW(hKey, L"WallpaperStyle", NULL, NULL, (LPBYTE)szBuffer, &dwSize) == ERROR_SUCCESS) {
                    style = _wtoi(szBuffer);
                }
                dwSize = sizeof(szBuffer);
                if (RegQueryValueExW(hKey, L"TileWallpaper", NULL, NULL, (LPBYTE)szBuffer, &dwSize) == ERROR_SUCCESS) {
                    tile = (_wtoi(szBuffer) == 1);
                }
                RegCloseKey(hKey);
            }
        }

        // Check slideshow settings and start/stop timer dynamically
        HKEY hKeySlide;
        if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKeySlide) == ERROR_SUCCESS) {
            DWORD cb = sizeof(DWORD);
            RegQueryValueExW(hKeySlide, L"DesktopSlideshowEnabled", NULL, NULL, (LPBYTE)&slideshowEnabled, &cb);
            cb = sizeof(DWORD);
            RegQueryValueExW(hKeySlide, L"DesktopSlideshowInterval", NULL, NULL, (LPBYTE)&slideshowInterval, &cb);
            RegCloseKey(hKeySlide);
        }
        
        static DWORD s_lastSlideshowEnabled = 0;
        static DWORD s_lastSlideshowInterval = 0;
        
        slideshowJustEnabled = (s_lastSlideshowEnabled != slideshowEnabled) && slideshowEnabled;

        if (slideshowEnabled) {
            if (slideshowInterval < 3) slideshowInterval = 3;
            if (s_lastSlideshowEnabled != slideshowEnabled || s_lastSlideshowInterval != slideshowInterval) {
                SetTimer(hwnd, TIMER_SLIDESHOW, slideshowInterval * 1000, NULL);
                s_lastSlideshowEnabled = slideshowEnabled;
                s_lastSlideshowInterval = slideshowInterval;
            }
        } else {
            if (s_lastSlideshowEnabled != slideshowEnabled) {
                KillTimer(hwnd, TIMER_SLIDESHOW);
                s_lastSlideshowEnabled = slideshowEnabled;
            }
        }

        // Startup Slideshow Rendering Delay Fix
        if (slideshowEnabled) {
            std::wstring activeThemeDir = GetThemeDirectory();
            bool isOutside = true;
            if (!s_cachedWallpaperPath.empty() && !activeThemeDir.empty()) {
                if (s_cachedWallpaperPath.length() > activeThemeDir.length() &&
                    _wcsnicmp(s_cachedWallpaperPath.c_str(), activeThemeDir.c_str(), activeThemeDir.length()) == 0 &&
                    (s_cachedWallpaperPath[activeThemeDir.length()] == L'\\' || s_cachedWallpaperPath[activeThemeDir.length()] == L'/')) {
                    isOutside = false;
                }
            }
            
            if (s_cachedWallpaperPath.empty() || slideshowJustEnabled || isOutside) {
                std::vector<std::wstring> images;
                if (!activeThemeDir.empty() && PathFileExistsW(activeThemeDir.c_str())) {
                    const wchar_t* extensions[] = { L"\\*.jpg", L"\\*.png", L"\\*.bmp", L"\\*.jpeg" };
                    for (const auto& ext : extensions) {
                        std::wstring query = activeThemeDir + ext;
                        WIN32_FIND_DATAW fd;
                        HANDLE hFind = FindFirstFileW(query.c_str(), &fd);
                        if (hFind != INVALID_HANDLE_VALUE) {
                            do {
                                images.push_back(activeThemeDir + L"\\" + fd.cFileName);
                            } while (FindNextFileW(hFind, &fd));
                            FindClose(hFind);
                        }
                    }
                }
                if (!images.empty()) {
                    std::sort(images.begin(), images.end());
                    s_cachedWallpaperPath = images[0];
                } else {
                    s_cachedWallpaperPath = wallpaperPath;
                }
            }
        }

        // Determine if settings have changed
        static std::wstring s_lastLoadedWallpaperPath = L"";
        bool settingsChanged = (drawWallpaper != (s_cachedDrawWallpaper ? 1 : 0)) ||
                               (wallpaperPath != s_cachedWallpaperPath && !slideshowEnabled) ||
                               (style != s_cachedStyle) ||
                               (tile != s_cachedTile);

        if (settingsChanged || !s_pCachedWallpaper || (slideshowEnabled && s_cachedWallpaperPath != s_lastLoadedWallpaperPath)) {
            if (s_pCachedWallpaper) {
                delete s_pCachedWallpaper;
                s_pCachedWallpaper = nullptr;
            }

            s_cachedDrawWallpaper = (drawWallpaper == 1);
            s_cachedStyle = style;
            s_cachedTile = tile;

            if (!slideshowEnabled) {
                s_cachedWallpaperPath = wallpaperPath;
            }

            if (s_cachedDrawWallpaper && !s_cachedWallpaperPath.empty() && PathFileExistsW(s_cachedWallpaperPath.c_str())) {
                s_pCachedWallpaper = new Gdiplus::Bitmap(s_cachedWallpaperPath.c_str());
                if (s_pCachedWallpaper->GetLastStatus() != Gdiplus::Ok) {
                    delete s_pCachedWallpaper;
                    s_pCachedWallpaper = nullptr;
                } else {
                    s_lastLoadedWallpaperPath = s_cachedWallpaperPath;
                }
            }
        }
    }

    if (!s_cachedDrawWallpaper || !s_pCachedWallpaper) {
        HBRUSH hBrush = CreateSolidBrush(GetSysColor(COLOR_BACKGROUND));
        RECT rc = { 0, 0, scrW, scrH };
        FillRect(hdc, &rc, hBrush);
        DeleteObject(hBrush);
        return;
    }

    // Determine wallpaper mode: native style 22 is Span.
    // If UseNativeWallpaperEngine is enabled, style == 22 -> Span mode (draw over virtual screen client coords),
    // otherwise style != 22 -> Per-monitor.
    // If UseNativeWallpaperEngine is disabled, use the custom "DesktopWallpaperMode" registry value.
    DWORD wallpaperModeVal = 0;
    if (useNativeWallpaperVal == 1) {
        wallpaperModeVal = (s_cachedStyle == 22) ? 0 : 1;
    } else {
        HKEY hKeyMode;
        if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKeyMode) == ERROR_SUCCESS) {
            DWORD cbMode = sizeof(DWORD);
            RegQueryValueExW(hKeyMode, L"DesktopWallpaperMode", NULL, NULL, (LPBYTE)&wallpaperModeVal, &cbMode);
            RegCloseKey(hKeyMode);
        }
    }

    if (wallpaperModeVal == 1) {
        MonitorWallpaperData data;
        data.hdc = hdc;
        data.pBitmap = s_pCachedWallpaper;
        data.style = s_cachedStyle;
        data.tile = s_cachedTile;
        data.bgColor = GetSysColor(COLOR_BACKGROUND);
        
        EnumDisplayMonitors(NULL, NULL, DrawWallpaperMonitorProc, (LPARAM)&data);
    } else {
        int imgW = s_pCachedWallpaper->GetWidth();
        int imgH = s_pCachedWallpaper->GetHeight();

        Gdiplus::Graphics graphics(hdc);
        graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);

        if (s_cachedTile) {
            Gdiplus::TextureBrush brush(s_pCachedWallpaper);
            brush.SetWrapMode(Gdiplus::WrapModeTile);
            graphics.FillRectangle(&brush, 0, 0, scrW, scrH);
        } else {
            int destX = 0, destY = 0, destW = scrW, destH = scrH;
            int srcX = 0, srcY = 0, srcW = imgW, srcH = imgH;

            double imgAspect = (double)imgW / imgH;
            double scrAspect = (double)scrW / scrH;

            // If UseNativeWallpaperEngine is enabled and we are in Span mode (style == 22), stretch it.
            // Under style == 22, it behaves like Stretch (destW = scrW, destH = scrH).
            if (useNativeWallpaperVal == 1 && s_cachedStyle == 22) {
                destX = 0; destY = 0; destW = scrW; destH = scrH;
            } else {
                switch (s_cachedStyle) {
                    case 0: // Center
                        if (imgW <= scrW) {
                            destX = (scrW - imgW) / 2;
                            destW = imgW;
                        } else {
                            destX = 0;
                            destW = scrW;
                            srcX = (imgW - scrW) / 2;
                            srcW = scrW;
                        }
                        if (imgH <= scrH) {
                            destY = (scrH - imgH) / 2;
                            destH = imgH;
                        } else {
                            destY = 0;
                            destH = scrH;
                            srcY = (imgH - scrH) / 2;
                            srcH = scrH;
                        }
                        graphics.Clear(Gdiplus::Color(GetRValue(GetSysColor(COLOR_BACKGROUND)), 
                                                      GetGValue(GetSysColor(COLOR_BACKGROUND)), 
                                                      GetBValue(GetSysColor(COLOR_BACKGROUND))));
                        break;
                    case 2: // Stretch
                        destX = 0; destY = 0;
                        destW = scrW; destH = scrH;
                        break;
                    case 6: // Fit
                        if (imgAspect > scrAspect) {
                            destW = scrW;
                            destH = (int)(scrW / imgAspect);
                            destX = 0;
                            destY = (scrH - destH) / 2;
                        } else {
                            destH = scrH;
                            destW = (int)(scrH * imgAspect);
                            destY = 0;
                            destX = (scrW - destW) / 2;
                        }
                        graphics.Clear(Gdiplus::Color(GetRValue(GetSysColor(COLOR_BACKGROUND)), 
                                                      GetGValue(GetSysColor(COLOR_BACKGROUND)), 
                                                      GetBValue(GetSysColor(COLOR_BACKGROUND))));
                        break;
                    case 10: // Fill
                    default:
                        if (imgAspect > scrAspect) {
                            destH = scrH;
                            destW = (int)(scrH * imgAspect);
                            destY = 0;
                            destX = (scrW - destW) / 2;
                        } else {
                            destW = scrW;
                            destH = (int)(scrW / imgAspect);
                            destX = 0;
                            destY = (scrH - destH) / 2;
                        }
                        break;
                    case 22: // Span
                        destX = 0; destY = 0;
                        destW = scrW; destH = scrH;
                        break;
                }
            }

            graphics.DrawImage(s_pCachedWallpaper, 
                Gdiplus::Rect(destX, destY, destW, destH),
                srcX, srcY, srcW, srcH, 
                Gdiplus::UnitPixel);
        }
    }
}
