# Handoff Report: Desktop Icon Grid & Shell Namespace Binding (Milestone 1, Task 3)

## 1. Observation
The following source files and design assets were reviewed:
- `ReactOS-Source-Shell/explorer/desktop.cpp` (lines 143–177): Implements a separate thread (`CDesktopThread`) to initialize and host the desktop workspace via `_SHCreateDesktop` (Ordinal 200 of `shell32.dll`) and `_SHDesktopMessageLoop` (Ordinal 201).
- `ReactOS-Source-Shell/explorer/rshell.cpp` (lines 50–77): Resolves `_SHCreateDesktop` dynamically via ordinal indexing from `shell32.dll`.
- `Documentation/BuildGuide-FeatureRequirement_CheckList.md` (lines 328–336): Outlines requirements for Section XI: Desktop Window Routing & Icon Grid (PROGMAN), specifically creating a child window of "Progman" using class name "SHELLDLL_DefView" and a list view of class name "SysListView32", binding to desktop folders using `IShellFolder`, and utilizing `SHChangeNotifyRegister` for refreshing.
- `GEMINI.md` (Rule 1, lines 3–10): Mandates preserving existing code paths and exposing registry switches for behavior configuration.

## 2. Logic Chain
1. **Window Hierarchy for Native Replication**: 
   - Standard Windows applications and desktop widget customizers (e.g., Fences, Wallpaper Engine) query the desktop window hierarchy using `FindWindow`/`FindWindowEx` with hardcoded class names.
   - To achieve flawless native replication (as requested by Rule 5 in `GEMINI.md`), the custom shell desktop window must exactly match the OS layout: `Progman` (parent) -> `SHELLDLL_DefView` (child) -> `SysListView32` (grandchild).
   - This layout can be achieved by registering custom classes named `"Progman"` and `"SHELLDLL_DefView"`, and creating a child `WC_LISTVIEWW` control with class `"SysListView32"`.
2. **Directory Aggregation using IShellFolder**:
   - The desktop consists of two distinct folder locations on disk: the current user's desktop (`CSIDL_DESKTOPDIRECTORY`) and the public/common desktop (`CSIDL_COMMON_DESKTOPDIRECTORY`).
   - Using `SHGetDesktopFolder` retrieves the virtual root `IShellFolder`. Enumerating this folder's items via `IEnumIDList::EnumObjects` natively aggregates both directories into a unified view.
   - Retreiving items requires calling `IEnumIDList::Next`, fetching display names via `IShellFolder::GetDisplayNameOf`, and matching system icon indices using `SHGetFileInfoW` on child PIDLs.
3. **Change Monitoring and Debouncing**:
   - To reflect additions, deletions, or updates in real time, `SHChangeNotifyRegister` must watch the virtual desktop PIDL (`CSIDL_DESKTOP`).
   - Using the `SHCNRF_NewDelivery` flag delivers granular notifications. However, processing file-by-file updates directly inside the notification handler can cause layout flickering and performance locks during batch file operations.
   - Introducing a 100ms debounced refresh (setting a timer in `WM_SHELLCHANGE` and executing a full repopulation when it fires) merges rapid updates into a single UI paint, ensuring smooth and performant desktop interactions.

## 3. Caveats
- **Wallpaper Drawing Coordination**: This task does not implement wallpaper drawing. The `WM_PAINT` and `WM_ERASEBKGND` handling on the `Progman` window must align with findings from `explorer_m1_2` to ensure proper blending underneath the list-view grid.
- **Icon Positioning Persistence**: Custom positions (unaligned to grid) require writing/reading coordinates from `HKCU\Software\EliteSoftware\Win32Explorer\Desktop\IconPositions` during shell shutdown and refresh. If not implemented, `LVS_AUTOARRANGE` should be enforced by default.
- **COM Apartment State**: The thread managing the desktop grid must be initialized as `COINIT_APARTMENTTHREADED` (STA) via `CoInitializeEx` for proper COM shell shell-extensions and shortcut resolution.

## 4. Conclusion
To build a highly compatible, performant desktop grid:
1. Register custom window classes for `"Progman"` and `"SHELLDLL_DefView"`.
2. Handle `WM_WINDOWPOSCHANGING` in `Progman` to clamp it at `HWND_BOTTOM` Z-order.
3. Create the list view with styles `WS_CHILD | WS_VISIBLE | LVS_ICON | LVS_SHAREIMAGELISTS | LVS_AUTOARRANGE | LVS_EDITLABELS`.
4. Bind to `SHGetDesktopFolder`, populating the grid using child PIDLs, and execute them on double click (`NM_DBLCLK`) via `ShellExecuteExW` with `SEE_MASK_IDLIST`.
5. Use `SHChangeNotifyRegister` on `CSIDL_DESKTOP` with a 100ms timer debounce to trigger repopulation.

### Proposed Code Structure
```cpp
// Target Location: SourceFiles/DesktopWindow.cpp (New File) or integration in TaskbarWindow.cpp

#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>

#define WM_SHELLCHANGE (WM_USER + 101)
#define TIMER_DEBOUNCE_REFRESH 1001

LRESULT CALLBACK ProgmanWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK DefViewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void PopulateDesktopGrid(HWND hwndListView);
ULONG RegisterDesktopChangeWatcher(HWND hwndTarget);

// Initializer called during shell boot (Replace Mode)
HWND CreateDesktopWindowGrid(HINSTANCE hInstance)
{
    WNDCLASSEXW wcProgman = { 0 };
    wcProgman.cbSize = sizeof(WNDCLASSEXW);
    wcProgman.style = CS_DBLCLKS;
    wcProgman.lpfnWndProc = ProgmanWndProc;
    wcProgman.hInstance = hInstance;
    wcProgman.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcProgman.hbrBackground = (HBRUSH)(COLOR_DESKTOP + 1);
    wcProgman.lpszClassName = L"Progman";
    RegisterClassExW(&wcProgman);

    WNDCLASSEXW wcDefView = { 0 };
    wcDefView.cbSize = sizeof(WNDCLASSEXW);
    wcDefView.style = CS_DBLCLKS;
    wcDefView.lpfnWndProc = DefViewWndProc;
    wcDefView.hInstance = hInstance;
    wcDefView.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcDefView.hbrBackground = NULL;
    wcDefView.lpszClassName = L"SHELLDLL_DefView";
    RegisterClassExW(&wcDefView);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    HWND hwndProgman = CreateWindowExW(
        WS_EX_TOOLWINDOW,
        L"Progman",
        L"Program Manager",
        WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN,
        0, 0, screenWidth, screenHeight,
        NULL, NULL, hInstance, NULL
    );

    if (hwndProgman)
    {
        SetWindowPos(hwndProgman, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
    return hwndProgman;
}

LRESULT CALLBACK ProgmanWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HWND hwndDefView = NULL;
    switch (uMsg)
    {
    case WM_CREATE:
    {
        CREATESTRUCTW* pcs = (CREATESTRUCTW*)lParam;
        hwndDefView = CreateWindowExW(
            0, L"SHELLDLL_DefView", L"",
            WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
            0, 0, pcs->cx, pcs->cy, hwnd, (HMENU)100, pcs->hInstance, NULL
        );
        return 0;
    }
    case WM_SIZE:
    {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        if (hwndDefView)
        {
            SetWindowPos(hwndDefView, NULL, 0, 0, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
        }
        return 0;
    }
    case WM_WINDOWPOSCHANGING:
    {
        LPWINDOWPOS lpwp = (LPWINDOWPOS)lParam;
        lpwp->hwndInsertAfter = HWND_BOTTOM;
        lpwp->flags &= ~SWP_NOZORDER;
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK DefViewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HWND hwndListView = NULL;
    static ULONG uNotifyId = 0;

    switch (uMsg)
    {
    case WM_CREATE:
    {
        CREATESTRUCTW* pcs = (CREATESTRUCTW*)lParam;
        hwndListView = CreateWindowExW(
            WS_EX_TRANSPARENT, WC_LISTVIEWW, L"",
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
            LVS_ICON | LVS_ALIGNLEFT | LVS_SHAREIMAGELISTS | LVS_AUTOARRANGE | LVS_EDITLABELS,
            0, 0, pcs->cx, pcs->cy, hwnd, (HMENU)200, pcs->hInstance, NULL
        );
        if (!hwndListView) return -1;

        ListView_SetExtendedListViewStyle(hwndListView, LVS_EX_DOUBLEBUFFER | LVS_EX_BORDERSELECT);
        ListView_SetTextBkColor(hwndListView, CLR_NONE);
        ListView_SetTextColor(hwndListView, RGB(255, 255, 255));

        SHFILEINFOW sfiNormal = { 0 };
        HIMAGELIST hSysIL = (HIMAGELIST)SHGetFileInfoW(L"", 0, &sfiNormal, sizeof(sfiNormal), SHGFI_SYSICONINDEX | SHGFI_LARGEICON);
        if (hSysIL)
        {
            ListView_SetImageList(hwndListView, hSysIL, LVSIL_NORMAL);
        }

        PopulateDesktopGrid(hwndListView);
        uNotifyId = RegisterDesktopChangeWatcher(hwnd);
        return 0;
    }
    case WM_SIZE:
    {
        if (hwndListView)
        {
            SetWindowPos(hwndListView, NULL, 0, 0, LOWORD(lParam), HIWORD(lParam), SWP_NOZORDER | SWP_NOACTIVATE);
        }
        return 0;
    }
    case WM_SHELLCHANGE:
        SetTimer(hwnd, TIMER_DEBOUNCE_REFRESH, 100, NULL);
        return 0;
    case WM_TIMER:
        if (wParam == TIMER_DEBOUNCE_REFRESH)
        {
            KillTimer(hwnd, TIMER_DEBOUNCE_REFRESH);
            PopulateDesktopGrid(hwndListView);
        }
        return 0;
    case WM_NOTIFY:
    {
        LPNMHDR pnmh = (LPNMHDR)lParam;
        if (pnmh->hwndFrom == hwndListView)
        {
            switch (pnmh->code)
            {
            case NM_DBLCLK:
            {
                LPNMLISTVIEW pnmlv = (LPNMLISTVIEW)lParam;
                if (pnmlv->iItem != -1)
                {
                    LVITEMW lvi = { 0 };
                    lvi.mask = LVIF_PARAM;
                    lvi.iItem = pnmlv->iItem;
                    if (ListView_GetItem(hwndListView, &lvi))
                    {
                        PITEMID_CHILD pidl = (PITEMID_CHILD)lvi.lParam;
                        if (pidl)
                        {
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
            case LVN_ENDLABELEDITW:
            {
                NMLVDISPINFOW* pdi = (NMLVDISPINFOW*)lParam;
                if (pdi->item.pszText != NULL && pdi->item.iItem != -1)
                {
                    PITEMID_CHILD pidlOld = (PITEMID_CHILD)pdi->item.lParam;
                    if (pidlOld)
                    {
                        IShellFolder* pDesktopFolder = NULL;
                        if (SUCCEEDED(SHGetDesktopFolder(&pDesktopFolder)))
                        {
                            PITEMID_CHILD pidlNew = nullptr;
                            HRESULT hr = pDesktopFolder->SetNameOf(hwndListView, pidlOld, pdi->item.pszText, SHGDN_INFOLDER, &pidlNew);
                            pDesktopFolder->Release();
                            if (SUCCEEDED(hr))
                            {
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
    case WM_DESTROY:
    {
        if (uNotifyId) SHChangeNotifyDeregister(uNotifyId);
        int count = ListView_GetItemCount(hwndListView);
        for (int i = 0; i < count; ++i)
        {
            LVITEMW lvi = { 0 };
            lvi.mask = LVIF_PARAM;
            lvi.iItem = i;
            if (ListView_GetItem(hwndListView, &lvi))
            {
                PITEMID_CHILD pidl = (PITEMID_CHILD)lvi.lParam;
                if (pidl) CoTaskMemFree(pidl);
            }
        }
        return 0;
    }
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

void PopulateDesktopGrid(HWND hwndListView)
{
    int count = ListView_GetItemCount(hwndListView);
    for (int i = 0; i < count; ++i)
    {
        LVITEMW lvi = { 0 };
        lvi.mask = LVIF_PARAM;
        lvi.iItem = i;
        if (ListView_GetItem(hwndListView, &lvi))
        {
            PITEMID_CHILD pidl = (PITEMID_CHILD)lvi.lParam;
            if (pidl) CoTaskMemFree(pidl);
        }
    }
    ListView_DeleteAllItems(hwndListView);

    IShellFolder* pDesktopFolder = nullptr;
    if (FAILED(SHGetDesktopFolder(&pDesktopFolder))) return;

    IEnumIDList* pEnumIDList = nullptr;
    if (SUCCEEDED(pDesktopFolder->EnumObjects(hwndListView, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN, &pEnumIDList)) && pEnumIDList)
    {
        ULONG uFetched = 0;
        PITEMID_CHILD pidlChild = nullptr;
        while (pEnumIDList->Next(1, &pidlChild, &uFetched) == S_OK && uFetched == 1)
        {
            STRRET strRet;
            wchar_t szName[MAX_PATH] = L"";
            if (SUCCEEDED(pDesktopFolder->GetDisplayNameOf(pidlChild, SHGDN_INFOLDER, &strRet)))
            {
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

ULONG RegisterDesktopChangeWatcher(HWND hwndTarget)
{
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
```

## 5. Verification Method
1. **Compilation Check**: Combine the proposed source with `main.cpp` entry point structure, and compile the target via `build.ps1`. Verify that no syntax errors arise from shell/list view declarations or `IShellFolder`/`SHChangeNotifyRegister` invocations.
2. **Spy++ Hierarchy Verification**: Once running in Replace mode, launch Spy++ (or `spyxx.exe`) and verify that:
   - Window with class `"Progman"` exists as a top-level window.
   - Window with class `"SHELLDLL_DefView"` exists as its child.
   - Control with class `"SysListView32"` exists as a child of `"SHELLDLL_DefView"`.
3. **Registry Check**: Verify that `HKCU\Software\EliteSoftware\Win32Explorer\Advanced` settings enable toggling the Progman creation correctly.
4. **Invalidation Conditions**: If standard Windows `explorer.exe` is running, registering `"Progman"` will fail because class registration is global and Explorer already owns it. Thus, verification of registering `"Progman"` must be done either in replacing shell mode or by using a secondary class name (e.g., `"Elite_Progman"`) under testing conditions, as mandated by the mode toggle.
