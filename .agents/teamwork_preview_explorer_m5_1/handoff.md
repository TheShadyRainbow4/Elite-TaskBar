# Handoff Report - Milestone 5 Analysis

## 1. Observation

Direct observations made in the codebase:

### Start Menu Invocation
In `SourceFiles/StartButton.cpp` under `OrbWndProc`, clicks are handled via `WM_LBUTTONUP` (lines 275-370):
```cpp
275:         case WM_LBUTTONUP: {
...
314:                 HWND hNativeTarget = NULL;
315:                 HMONITOR hMon = MonitorFromWindow(pThis->GetParentTaskbar(), MONITOR_DEFAULTTONULL);
...
330:                 if (!hNativeTarget) hNativeTarget = FindWindowW(L"Shell_TrayWnd", NULL);
...
344:                 } else if (mode == 1 || (mode == 3 && !isShiftDown) || (mode == 2 && isShiftDown)) {
345:                     // Open Native Menu (Requires Shift for Open-Shell users)
346:                     bool injectShift = !isShiftDown;
347:                     if (injectShift) keybd_event(VK_SHIFT, 0, 0, 0);
348:                     SendMessageW(hNativeTarget, WM_SYSCOMMAND, SC_TASKLIST, lCursorParam);
349:                     if (injectShift) keybd_event(VK_SHIFT, 0, KEYEVENTF_KEYUP, 0);
```

### Volume/Sound Tray Icon Clicks
In `SourceFiles/TaskbarWindow.cpp` inside `TrayToolbarSubclassProc` (lines 100-118), tray icon clicks are forwarded to the native owner:
```cpp
100: LRESULT CALLBACK TrayToolbarSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
101:     switch (uMsg) {
102:     case WM_LBUTTONDOWN:
...
110:     case WM_MBUTTONDBLCLK: {
111:         POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
112:         int index = (int)SendMessageW(hWnd, TB_HITTEST, 0, (LPARAM)&pt);
113:         if (index >= 0 && index < (int)g_CurrentTrayIcons.size()) {
114:             const auto& icon = g_CurrentTrayIcons[index];
115:             PostMessageW(icon.hwnd, icon.uCallbackMessage, icon.uID, uMsg);
116:         }
```

### Native Taskbar Teleport Loop (UIPI Defense)
In `SourceFiles/TaskbarWindow.cpp` inside `WindowProc` (lines 2103-2108), the background timer repeatedly hides and repositions the native taskbar to prevent it from overlaying:
```cpp
2103:         if (wParam == 9999 && g_Config.Mode == TaskbarMode::Replace && g_hNativeTaskbar) {
2104:             if (IsWindowVisible(g_hNativeTaskbar)) {
2105:                 ShowWindow(g_hNativeTaskbar, SW_HIDE);
2106:                 SetWindowPos(g_hNativeTaskbar, NULL, -10000, -10000, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
2107:             }
2108:             SyncTaskbarButtonsAcrossMonitors();
2109:         }
```

### About Dialog and Settings Activation
- In `SourceFiles/TaskbarProperties.cpp`, the About dialog is shown using `ShowAboutDialog(HWND hwndOwner)` at line 266.
- The Custom settings panel is triggered via `ShowTaskbarProperties(HWND hwnd)` (line 1988 of `TaskbarWindow.cpp`).

### Custom Desktop Window Lifecycle
- In `SourceFiles/DesktopWindow.cpp`, custom desktop initialization and cleanup are controlled by `DesktopWindow::Initialize()` (line 34) and `DesktopWindow::Cleanup()` (line 110).
- The desktop visibility check utilizes registry key `DesktopReplacementEnabled` queried at startup in `TaskbarWindow::Initialize` (lines 2600-2609).

---

## 2. Logic Chain

1. **Start Menu & Volume Flyouts rendering context**: The OS shell experienced flyouts (like Start Menu or Volume/Network flyouts) decide which monitor they will show on based on the position of the native taskbar `Shell_TrayWnd` (and secondary `Shell_SecondaryTrayWnd`). 
2. **Defect on secondary screens**: Since standard Replace mode hides the native taskbars and pushes them off-screen to `(-10000, -10000)` (Observation Section 3), any click on the Start button or volume tray item of a secondary taskbar delegates to an off-screen/hidden native target. As a result, the OS-level rendering engine defaults the flyout display position to the primary display.
3. **Spoofing resolution**: To fix this, we must temporarily teleport the native `Shell_TrayWnd` to the secondary monitor's taskbar area (covering the exact size of our secondary taskbar instance) and make it visible. 
4. **Visual seamlessness**: Since our custom taskbar is an `HWND_TOPMOST` window, it stays directly on top of the native taskbar, keeping the spoofed native taskbar visually concealed from the user.
5. **Inhibiting the teleport timer**: The 100ms background timer (ID `9999`) will immediately move the native taskbar back off-screen if it runs during the spoof. We must introduce a flag `g_IsSpoofingNativeTaskbar` to bypass the timer's relocation logic while a spoof is active.
6. **Registration of custom tray items**: Since there are no custom notification area icons defined in the project, we should register three tray items for Win32Explorer, Taskbar, and Desktop Replacement during primary taskbar creation, routing their callback events to `WindowProc`.
7. **Control flows**:
   - Double-clicking the Desktop Replacement tray icon toggles the visibility of the internal ListView grid window (`SysListView32` child of `SHELLDLL_DefView` under the custom `Progman` window).
   - The right-click menu toggles the `DesktopReplacementEnabled` registry key and switches the custom desktop replacement state on or off dynamically.

---

## 3. Caveats

- **Explorer Restarts**: If the Windows Explorer shell crashes or restarts, the tray notification icons must be re-registered on our window. The `WM_CREATE` or the custom taskbar message (`g_uTaskbarCreatedMsg`) should trigger re-registration.
- **DPI Scaling**: The teleport position and sizing of the native taskbar must match the target secondary monitor's specific DPI configuration to prevent misalignment of flyouts.
- **Multiprocess Coordination**: Settings changes might require settings stub execution permissions or registry updates.

---

## 4. Conclusion

To implement Milestone 5 requirements, we recommend the following code additions and modifications:

### A. Dynamic Display Spoofing Implementation

1. Declare global state variables at the top of `SourceFiles/TaskbarWindow.cpp`:
```cpp
bool g_IsSpoofingNativeTaskbar = false;
DWORD g_SpoofStartTime = 0;
```

2. Add helper functions to start and stop the spoofing in `TaskbarWindow.cpp`:
```cpp
void StartNativeTaskbarSpoof(HWND hClickedTaskbar) {
    if (!g_hNativeTaskbar || !IsWindow(g_hNativeTaskbar)) return;
    
    RECT rcTaskbar;
    GetWindowRect(hClickedTaskbar, &rcTaskbar);
    
    g_IsSpoofingNativeTaskbar = true;
    g_SpoofStartTime = GetTickCount();
    
    // Teleport native taskbar to the target taskbar and show it
    SetWindowPos(g_hNativeTaskbar, HWND_BOTTOM, rcTaskbar.left, rcTaskbar.top,
                 rcTaskbar.right - rcTaskbar.left, rcTaskbar.bottom - rcTaskbar.top,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
}
```

3. Update the 100ms timer (Timer ID `9999`) in `WindowProc` to avoid resetting the spoofed taskbar prematurely:
```cpp
        if (wParam == 9999 && g_Config.Mode == TaskbarMode::Replace && g_hNativeTaskbar) {
            if (g_IsSpoofingNativeTaskbar) {
                if (GetTickCount() - g_SpoofStartTime > 2000) { // Safety timeout: 2 seconds
                    g_IsSpoofingNativeTaskbar = false;
                } else {
                    return 0; // Inhibit relocation while spoofing is active
                }
            }
            if (IsWindowVisible(g_hNativeTaskbar)) {
                ShowWindow(g_hNativeTaskbar, SW_HIDE);
                SetWindowPos(g_hNativeTaskbar, NULL, -10000, -10000, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
            }
            SyncTaskbarButtonsAcrossMonitors();
        }
```

4. Modify `StartButton.cpp` under `WM_LBUTTONUP` to trigger spoofing on secondary screens before sending click/SYSCOMMAND:
```cpp
                // Before SendMessageW to native target:
                if (pThis->GetMonitorIndex() > 0) {
                    extern void StartNativeTaskbarSpoof(HWND hClickedTaskbar);
                    StartNativeTaskbarSpoof(pThis->GetParentTaskbar());
                }
```

5. Modify `TrayToolbarSubclassProc` to pass `inst` to `dwRefData` and trigger spoofing:
   - In `TaskbarWindow::Initialize` at line 2514, change `SetWindowSubclass(inst->hToolbar, TrayToolbarSubclassProc, 2, 0);` to `SetWindowSubclass(inst->hToolbar, TrayToolbarSubclassProc, 2, (DWORD_PTR)inst);`.
   - Update `TrayToolbarSubclassProc` to read `dwRefData` and spoof before posting tray callbacks on secondary monitors:
```cpp
        if (index >= 0 && index < (int)g_CurrentTrayIcons.size()) {
            const auto& icon = g_CurrentTrayIcons[index];
            TaskbarInstance* inst = (TaskbarInstance*)dwRefData;
            
            if (inst && inst->monitorIndex > 0) {
                StartNativeTaskbarSpoof(inst->hTaskbar);
            }
            
            PostMessageW(icon.hwnd, icon.uCallbackMessage, icon.uID, uMsg);
        }
```

### B. Custom Tray Icons Registration

1. Define message IDs and icon constants in `TaskbarWindow.cpp`:
```cpp
#define WM_TRAY_CALLBACK_WIN32EXPLORER (WM_USER + 500)
#define WM_TRAY_CALLBACK_TASKBAR       (WM_USER + 501)
#define WM_TRAY_CALLBACK_DESKTOP       (WM_USER + 502)

#define TRAY_ID_WIN32EXPLORER 1
#define TRAY_ID_TASKBAR       2
#define TRAY_ID_DESKTOP       3
```

2. Add custom tray registration and cleanup methods:
```cpp
void RegisterCustomTrayIcons(HWND hwndParent, HINSTANCE hInst) {
    NOTIFYICONDATAW nid = { sizeof(nid) };
    nid.hWnd = hwndParent;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    
    // 1. Win32Explorer Tray
    nid.uID = TRAY_ID_WIN32EXPLORER;
    nid.uCallbackMessage = WM_TRAY_CALLBACK_WIN32EXPLORER;
    nid.hIcon = LoadIconW(hInst, MAKEINTRESOURCEW(IDI_MAIN_PROGRAM));
    wcscpy_s(nid.szTip, L"Win32Explorer Shell");
    Shell_NotifyIconW(NIM_ADD, &nid);
    
    // 2. Taskbar Tray
    nid.uID = TRAY_ID_TASKBAR;
    nid.uCallbackMessage = WM_TRAY_CALLBACK_TASKBAR;
    nid.hIcon = LoadIconW(hInst, MAKEINTRESOURCEW(IDI_PREFERENCES));
    wcscpy_s(nid.szTip, L"EliteTaskbar Settings");
    Shell_NotifyIconW(NIM_ADD, &nid);
    
    // 3. Desktop Replacement Tray
    nid.uID = TRAY_ID_DESKTOP;
    nid.uCallbackMessage = WM_TRAY_CALLBACK_DESKTOP;
    // Extract a generic monitor/desktop icon from shell32.dll as fallback
    nid.hIcon = ExtractIconW(hInst, L"shell32.dll", 34); 
    if (!nid.hIcon) nid.hIcon = LoadIconW(hInst, MAKEINTRESOURCEW(IDI_MAIN_PROGRAM));
    wcscpy_s(nid.szTip, L"Desktop Replacement Component");
    Shell_NotifyIconW(NIM_ADD, &nid);
}

void UnregisterCustomTrayIcons(HWND hwndParent) {
    NOTIFYICONDATAW nid = { sizeof(nid) };
    nid.hWnd = hwndParent;
    nid.uID = TRAY_ID_WIN32EXPLORER;
    Shell_NotifyIconW(NIM_DELETE, &nid);
    
    nid.uID = TRAY_ID_TASKBAR;
    Shell_NotifyIconW(NIM_DELETE, &nid);
    
    nid.uID = TRAY_ID_DESKTOP;
    Shell_NotifyIconW(NIM_DELETE, &nid);
}
```

3. Call `RegisterCustomTrayIcons` in `TaskbarWindow::Initialize` if the monitor is the primary monitor:
```cpp
    if (monData.isPrimary[i]) {
        RegisterCustomTrayIcons(inst->hTaskbar, hInstance);
    }
```
And call `UnregisterCustomTrayIcons` in `TaskbarWindow::Cleanup`:
```cpp
    // For the primary taskbar HWND
    if (!g_Taskbars.empty() && g_Taskbars[0]->hTaskbar) {
        UnregisterCustomTrayIcons(g_Taskbars[0]->hTaskbar);
    }
```

### C. Processing Tray Message Callbacks in `WindowProc`

Add the custom cases inside `WindowProc`:
```cpp
    case WM_TRAY_CALLBACK_WIN32EXPLORER: {
        UINT uMouseMsg = (UINT)lParam;
        if (uMouseMsg == WM_LBUTTONUP) {
            extern void ShowAboutDialog(HWND hwndOwner);
            ShowAboutDialog(hwnd);
        } else if (uMouseMsg == WM_LBUTTONDBLCLK) {
            ShellExecuteW(NULL, L"open", L"explorer.exe", L"shell:::{20D04FE0-3AEA-1069-A2D8-08002B30309D}", NULL, SW_SHOWNORMAL);
        }
        return 0;
    }
    case WM_TRAY_CALLBACK_TASKBAR: {
        UINT uMouseMsg = (UINT)lParam;
        if (uMouseMsg == WM_LBUTTONUP) {
            extern void ShowAboutDialog(HWND hwndOwner);
            ShowAboutDialog(hwnd);
        } else if (uMouseMsg == WM_LBUTTONDBLCLK) {
            extern void ShowTaskbarProperties(HWND hwnd);
            ShowTaskbarProperties(hwnd);
        }
        return 0;
    }
    case WM_TRAY_CALLBACK_DESKTOP: {
        UINT uMouseMsg = (UINT)lParam;
        if (uMouseMsg == WM_LBUTTONDBLCLK) {
            // Toggle desktop icons
            HWND hProgman = DesktopWindow::GetHWND();
            if (hProgman) {
                HWND hDefView = FindWindowExW(hProgman, NULL, L"SHELLDLL_DefView", NULL);
                if (hDefView) {
                    HWND hListView = FindWindowExW(hDefView, NULL, L"SysListView32", NULL);
                    if (hListView) {
                        bool isVisible = IsWindowVisible(hListView);
                        ShowWindow(hListView, isVisible ? SW_HIDE : SW_SHOW);
                        
                        // Toggle registry value
                        HKEY hKey;
                        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
                            DWORD dwHide = isVisible ? 1 : 0;
                            RegSetValueExW(hKey, L"HideIcons", 0, REG_DWORD, (const BYTE*)&dwHide, sizeof(DWORD));
                            RegCloseKey(hKey);
                        }
                    }
                }
            }
        } else if (uMouseMsg == WM_RBUTTONUP) {
            // Show custom context menu
            HMENU hMenu = CreatePopupMenu();
            if (hMenu) {
                bool desktopReplaceEnabled = false;
                HKEY hKeyDesktop;
                if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKeyDesktop) == ERROR_SUCCESS) {
                    DWORD dwVal = 0;
                    DWORD cbData = sizeof(DWORD);
                    if (RegQueryValueExW(hKeyDesktop, L"DesktopReplacementEnabled", NULL, NULL, (LPBYTE)&dwVal, &cbData) == ERROR_SUCCESS) {
                        desktopReplaceEnabled = (dwVal == 1);
                    }
                    RegCloseKey(hKeyDesktop);
                }
                
                AppendMenuW(hMenu, MF_STRING | (desktopReplaceEnabled ? MF_CHECKED : MF_UNCHECKED), 5001, L"Toggle Desktop Replacement");
                AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
                AppendMenuW(hMenu, MF_STRING, 5002, L"Restart Taskbar");
                AppendMenuW(hMenu, MF_STRING, 5003, L"Restart Explorer");
                
                POINT pt;
                GetCursorPos(&pt);
                SetForegroundWindow(hwnd);
                
                int cmd = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, NULL);
                DestroyMenu(hMenu);
                
                if (cmd == 5001) {
                    // Update registry
                    HKEY hKey;
                    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
                        DWORD dwVal = desktopReplaceEnabled ? 0 : 1;
                        RegSetValueExW(hKey, L"DesktopReplacementEnabled", 0, REG_DWORD, (const BYTE*)&dwVal, sizeof(DWORD));
                        RegCloseKey(hKey);
                    }
                    
                    if (desktopReplaceEnabled) {
                        DesktopWindow::Cleanup();
                    } else {
                        DesktopWindow::Initialize();
                    }
                } else if (cmd == 5002) {
                    // Restart Taskbar shell
                    wchar_t exePath[MAX_PATH];
                    GetModuleFileNameW(NULL, exePath, MAX_PATH);
                    wchar_t psCmd[512];
                    swprintf_s(psCmd, L"-NoProfile -WindowStyle Hidden -Command \"Stop-Process -Name EliteTaskbar -Force; Start-Sleep -Milliseconds 500; Start-Process -FilePath '%s'\"", exePath);
                    ShellExecuteW(NULL, NULL, L"powershell.exe", psCmd, NULL, SW_HIDE);
                } else if (cmd == 5003) {
                    // Restart explorer
                    ShellExecuteW(NULL, NULL, L"powershell.exe", L"-NoProfile -WindowStyle Hidden -Command \"Stop-Process -Name explorer -Force; Start-Process explorer.exe\"", NULL, SW_HIDE);
                }
            }
        }
        return 0;
    }
```

---

## 5. Verification Method

To verify these changes:

1. **Build Compilation Check**:
   Run `build.ps1` in a powershell console to verify the codebase compiles successfully without warnings or errors.
2. **Dynamic Display Spoofing Verification**:
   - Set taskbar mode to `Replace` and open on secondary monitor.
   - Click the Volume/Sound icon in the replicated taskbar tray on the secondary monitor.
   - Verify that the Windows volume flyout opens correctly positioned directly above the clicked icon on the secondary screen.
   - Click the Start button on the secondary taskbar. Verify the Start Menu opens on the secondary monitor.
3. **Custom Tray Icons Verification**:
   - Verify three new tray icons appear in the primary system tray.
   - Hover over each to verify tooltips: "Win32Explorer Shell", "EliteTaskbar Settings", "Desktop Replacement Component".
   - Test Win32Explorer tray icon:
     - Single-click -> Verify About dialog pops up.
     - Double-click -> Verify a new Explorer window opens.
   - Test Taskbar tray icon:
     - Single-click -> Verify About dialog pops up.
     - Double-click -> Verify Elite Taskbar Settings window opens.
   - Test Desktop Replacement tray icon:
     - Double-click -> Verify desktop icons hide/show.
     - Right-click -> Verify popup menu displays with "Toggle Desktop Replacement", "Restart Taskbar", and "Restart Explorer".
     - Click "Toggle Desktop Replacement" -> Verify the desktop replacements switches on/off.
