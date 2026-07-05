#include "DesktopWindow.h"
#include "Config.h"
#include "Logger.h"
#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <gdiplus.h>
#include <vector>
#include <string>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shlwapi.lib")

#define WM_SHELLCHANGE (WM_USER + 101)
#define TIMER_DEBOUNCE_REFRESH 1001

static HWND s_hProgman = NULL;
static HWND s_hNativeProgman = NULL;
static HWND s_hNativeWorkerW = NULL;
static Gdiplus::Bitmap* s_pCachedWallpaper = nullptr;
static std::wstring s_cachedWallpaperPath = L"";
static int s_cachedStyle = -1;
static bool s_cachedTile = false;
static bool s_cachedDrawWallpaper = true;


LRESULT CALLBACK ProgmanWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK DefViewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void PopulateDesktopGrid(HWND hwndListView);
ULONG RegisterDesktopChangeWatcher(HWND hwndTarget);
void DrawWallpaper(HDC hdc, int scrW, int scrH);

namespace DesktopWindow {
    bool Initialize() {
        Logger::Log(L"DesktopWindow::Initialize starting.");
        
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
    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hwnd, &rc);
        DrawWallpaper(hdc, rc.right - rc.left, rc.bottom - rc.top);
        return TRUE;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        DrawWallpaper(hdc, rc.right - rc.left, rc.bottom - rc.top);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY:
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
            LVS_ICON | LVS_ALIGNLEFT | LVS_SHAREIMAGELISTS | LVS_AUTOARRANGE | LVS_EDITLABELS,
            0, 0, pcs->cx, pcs->cy, hwnd, (HMENU)200, pcs->hInstance, NULL
        );
        if (!hwndListView) return -1;

        ListView_SetExtendedListViewStyle(hwndListView, LVS_EX_DOUBLEBUFFER | LVS_EX_BORDERSELECT);
        ListView_SetBkColor(hwndListView, CLR_NONE);
        ListView_SetTextBkColor(hwndListView, CLR_NONE);
        ListView_SetTextColor(hwndListView, RGB(255, 255, 255));

        // Retrieve and bind system image list
        SHFILEINFOW sfiNormal = { 0 };
        HIMAGELIST hSysIL = (HIMAGELIST)SHGetFileInfoW(L"", 0, &sfiNormal, sizeof(sfiNormal), SHGFI_SYSICONINDEX | SHGFI_LARGEICON);
        if (hSysIL) {
            ListView_SetImageList(hwndListView, hSysIL, LVSIL_NORMAL);
        }

        PopulateDesktopGrid(hwndListView);
        uNotifyId = RegisterDesktopChangeWatcher(hwnd);
        return 0;
    }
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
    case WM_SHELLCHANGE:
        SetTimer(hwnd, TIMER_DEBOUNCE_REFRESH, 100, NULL);
        return 0;
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
                    if (ListView_GetItem(hwndListView, &lvi)) {
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
            int count = ListView_GetItemCount(hwndListView);
            for (int i = 0; i < count; ++i) {
                LVITEMW lvi = { 0 };
                lvi.mask = LVIF_PARAM;
                lvi.iItem = i;
                if (ListView_GetItem(hwndListView, &lvi)) {
                    PITEMID_CHILD pidl = (PITEMID_CHILD)lvi.lParam;
                    if (pidl) CoTaskMemFree(pidl);
                }
            }
        }
        break;
    }
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

void PopulateDesktopGrid(HWND hwndListView) {
    // Determine if desktop icons are enabled
    bool desktopIconsEnabled = true;
    HKEY hKeyIcons;
    if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKeyIcons) == ERROR_SUCCESS) {
        DWORD dwVal = 1;
        DWORD cbData = sizeof(DWORD);
        if (RegQueryValueExW(hKeyIcons, L"DesktopIconsEnabled", NULL, NULL, (LPBYTE)&dwVal, &cbData) == ERROR_SUCCESS) {
            desktopIconsEnabled = (dwVal == 1);
        }
        RegCloseKey(hKeyIcons);
    }

    if (!desktopIconsEnabled) {
        ShowWindow(hwndListView, SW_HIDE);
        return;
    } else {
        ShowWindow(hwndListView, SW_SHOW);
    }

    // Clean up existing item parameters (PIDLs)
    int count = ListView_GetItemCount(hwndListView);
    for (int i = 0; i < count; ++i) {
        LVITEMW lvi = { 0 };
        lvi.mask = LVIF_PARAM;
        lvi.iItem = i;
        if (ListView_GetItem(hwndListView, &lvi)) {
            PITEMID_CHILD pidl = (PITEMID_CHILD)lvi.lParam;
            if (pidl) CoTaskMemFree(pidl);
        }
    }
    ListView_DeleteAllItems(hwndListView);

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

            SHFILEINFOW sfi = { 0 };
            SHGetFileInfoW((LPCWSTR)pidlChild, 0, &sfi, sizeof(sfi), SHGFI_PIDL | SHGFI_SYSICONINDEX);

            LVITEMW lvi = { 0 };
            lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
            lvi.iItem = ListView_GetItemCount(hwndListView);
            lvi.iSubItem = 0;
            lvi.pszText = szName;
            lvi.iImage = sfi.iIcon;
            lvi.lParam = (LPARAM)pidlChild;
            ListView_InsertItem(hwndListView, &lvi);
        }
        pEnumIDList->Release();
    }
    pDesktopFolder->Release();
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

void DrawWallpaper(HDC hdc, int scrW, int scrH) {
    HKEY hKey;
    DWORD drawWallpaper = 1;
    DWORD cbData = sizeof(DWORD);
    if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExW(hKey, L"DesktopWallpaperEnabled", NULL, NULL, (LPBYTE)&drawWallpaper, &cbData);
        RegCloseKey(hKey);
    }

    std::wstring wallpaperPath = L"";
    int style = 10; // default Fill
    bool tile = false;

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

    // Determine if settings have changed
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
        s_cachedWallpaperPath = wallpaperPath;
        s_cachedStyle = style;
        s_cachedTile = tile;

        if (s_cachedDrawWallpaper && !s_cachedWallpaperPath.empty() && PathFileExistsW(s_cachedWallpaperPath.c_str())) {
            s_pCachedWallpaper = new Gdiplus::Bitmap(s_cachedWallpaperPath.c_str());
            if (s_pCachedWallpaper->GetLastStatus() != Gdiplus::Ok) {
                delete s_pCachedWallpaper;
                s_pCachedWallpaper = nullptr;
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

            case 6: // Fit (Letterbox / Pillarbox)
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

        graphics.DrawImage(s_pCachedWallpaper, 
            Gdiplus::Rect(destX, destY, destW, destH),
            srcX, srcY, srcW, srcH, 
            Gdiplus::UnitPixel);
    }
}
