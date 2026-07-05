# Handoff Report: Codebase File Drift & Advanced Features Analysis

## 1. Observation

Direct comparison between the standalone source files (`SourceFiles/`) and the embedded Win32Explorer copies (`Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/`) revealed extensive file drift across five key files: `TaskbarWindow.cpp`, `TrayIconScraper.cpp`, `TaskbarProperties.cpp`, `resources.rc`, and `resource.h`.

### 1.1. resource.h & resources.rc
- **Header Changes (`resource.h`)**:
  - Win32Explorer copy adds `#pragma warning(disable: 4715)` at the top and metadata comments at the bottom.
- **Resource Changes (`resources.rc`)**:
  - **Include Paths**: Win32Explorer includes `"EliteTaskbar\resource.h"` and specifies assets under `"EliteTaskbar\Resources\"` instead of relative paths `"..\"` in `SourceFiles/`.
  - **Clip Children Style**: In `resources.rc`, the dialog styles (like `IDD_TASKBAR_PROPS`, `IDD_NATIVE_PROPS`, `IDD_MULTIMON_PROPS`) have the `WS_CLIPCHILDREN` style removed in the Win32Explorer copy:
    ```rc
    // SourceFiles copy:
    STYLE DS_SETFONT | WS_CHILD | WS_DISABLED | WS_CAPTION | DS_CONTROL | WS_CLIPCHILDREN
    // Win32Explorer copy:
    STYLE DS_SETFONT | WS_CHILD | WS_DISABLED | WS_CAPTION | DS_CONTROL
    ```
  - **UI Controls Drift**: In `IDD_TASKBAR_PROPS`, the "Taskbar Appearance" groupbox (and its Width controls, Fixed Size ComboBox, and Hover Previews/Portable settings) is entirely replaced in the Win32Explorer copy with "Custom Icon Theme Folder" controls:
    ```rc
    GROUPBOX        "Custom Icon Theme Folder", IDC_STATIC, 7, 58, 238, 77
    LTEXT           "Theme Path:", IDC_STATIC, 14, 73, 45, 8
    EDITTEXT        IDC_THEME_FOLDER_PATH, 65, 71, 120, 12, ES_AUTOHSCROLL | WS_TABSTOP
    PUSHBUTTON      "Browse...", IDC_THEME_FOLDER_BROWSE, 190, 70, 50, 14, WS_TABSTOP
    CONTROL         "Enable Dark Mode", IDC_ENABLE_DARK_MODE, "Button", BS_AUTOCHECKBOX | WS_DISABLED | WS_TABSTOP, 14, 95, 150, 10
    ```
  - **Explorer Replacement Removal**: In `IDD_NATIVE_PROPS`, the "Explorer Replacement" controls groupbox is completely removed in the Win32Explorer copy.
  - **About Dialog Spacing**: In `IDD_ABOUT_DIALOG`, the coordinates for expansion and buttons are shifted vertically:
    - Expand Button Y: `118` (Win32Explorer) vs `120` (`SourceFiles/`).
    - Okay Button Y: `118` (Win32Explorer) vs `120` (`SourceFiles/`).

### 1.2. TrayIconScraper.cpp
- **Double Scraping and Tooltips**: The Win32Explorer copy splits scraping into a helper `ScrapeTrayIconsFromToolbar(HWND hToolbar, std::vector<ScrapedTrayIcon>& icons)`:
  - Scrapes both the native taskbar tray toolbar (`Shell_TrayWnd` -> `TrayNotifyWnd` -> `SysPager` -> `ToolbarWindow32`) and the overflow pane (`NotifyIconOverflowWindow` -> `ToolbarWindow32`).
  - Calls `SendMessageW(hToolbar, TB_GETBUTTONTEXTW, ...)` to copy tooltip strings from the target toolbar processes to a mapped storage `g_TrayTooltipsMap` using remote memory allocation (`VirtualAllocEx` and `ReadProcessMemory`).
  - The `SourceFiles/` copy only scrapes `Shell_TrayWnd` and does not scrape tooltips or the overflow window.

### 1.3. TaskbarProperties.cpp
- **About Dialog Layout Spacing Fix**: Win32Explorer fixes button/chin coordinates in `AboutDlgProc` to support expanded sizing without overlap:
  - Expanded dialog height is `245` (vs `210` in `SourceFiles/`).
  - Chin is `{ 0, 215, 250, 245 }` (vs `{ 0, 185, 250, 210 }` in `SourceFiles/`).
  - Buttons (Expand/Okay) are positioned at Y=`223` (vs Y=`190` in `SourceFiles/`), separating them cleanly from the `IDC_ABOUT_MOREINFO` edit box which resides at Y=`145` with a height of `60`.
- **Apply Button Hang / Settings Broadcast**: Win32Explorer spawns a background thread to broadcast settings changes asynchronously:
  ```cpp
  DWORD WINAPI BroadcastSettingsChangeThread(LPVOID lpParam) {
      SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"TraySettings", SMTO_ABORTIFHUNG, 5000, NULL);
      SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"EliteTaskbarSettings", SMTO_ABORTIFHUNG, 500, NULL);
      return 0;
  }
  void NotifySettingsChange() {
      HANDLE hThread = CreateThread(NULL, 0, BroadcastSettingsChangeThread, NULL, 0, NULL);
      if (hThread) { CloseHandle(hThread); }
  }
  ```
  The `SourceFiles/` copy performs `SendMessageTimeoutW` synchronously on the main UI thread.
- **UI Logic Drift**: `TaskbarProperties.cpp` in Win32Explorer handles Custom Theme browse/path edit controls but strips out Button Width, Fixed Size, Previews, Portable registry settings, and Explorer Replacement sync methods. It also uses raw `HKEY_CURRENT_USER` registry writes instead of the Portable Mirror `GetEliteRegistryRoot()` function.

### 1.4. TaskbarWindow.cpp
- **Advanced Scraping & Event Routing**: Win32Explorer implements `TrayToolbarSubclassProc` to subclass `inst->hToolbar`. This procedure intercepts mouse events (`WM_LBUTTONDOWN`, `WM_RBUTTONUP`, etc.), resolves the targeted scraped icon via `TB_HITTEST`, and routes messages to the original windows using `PostMessageW(icon.hwnd, icon.uCallbackMessage, icon.uID, uMsg)`. It tracks hover tooltips using `TrackMouseEvent` and queries tooltips from `g_TrayTooltipsMap`.
- **UWP App Icons Fix**: Implements `GetWindowIconFix(HWND hwnd)` to resolve UWP Application Frame icons:
  1. Calls `SHGetPropertyStoreForWindow` to fetch `PKEY_AppUserModel_ID`.
  2. Resolves path `shell:AppsFolder\<AUMID>` and calls `IShellItemImageFactory::GetImage` to extract a clean 16x16 icon.
  3. Falls back to child window enumeration to locate `"Windows.UI.Core.CoreWindow"`, queries its PID, gets the executable path, and retrieves the icon using `SHGetFileInfoW`.
- **High-DPI Scaling & Fonts**:
  - Scales taskbar layout dimensions (`UpdateTaskbarLayout`) using monitor DPI queried from `shcore.dll` (`GetDpiForMonitor`).
  - Sets GDI+ font heights dynamically: `lfHeight = MulDiv(-12, dpi, 96)` in `PreviewWndProc` and `lfHeight = MulDiv(-11, dpi, 96)` in `TrayClockProc`.
- **Powershell Safe Restart Script**: In `WindowProc`, a restart command runs powershell in the background with sleep delays to avoid shell hangs:
  ```cpp
  swprintf_s(psCmd, L"-NoProfile -WindowStyle Hidden -Command \"Start-Sleep -s 1; Stop-Process -Name explorer -Force; Start-Sleep -s 1; Start-Process explorer.exe; Start-Process -FilePath '%s'\"", exePath);
  ShellExecuteW(NULL, NULL, L"powershell.exe", psCmd, NULL, SW_HIDE);
  ```
- **Liveness & Startup**: Removes the registration of the settings notify icon (NIM_ADD / NIM_DELETE) on the primary taskbar.

---

## 2. Logic Chain

Based on these observations, we can map the exact state of requirements in both folders:

1. **R4 (Tray Overflow Fix)**: Fully implemented in the Win32Explorer copy. The scraper double-scrapes `NotifyIconOverflowWindow`, the subclass intercepts mouse movement and clicks, maps coordinates via `TB_HITTEST`, and handles tooltips using a mapped database. In `SourceFiles/`, none of this exists (completely missing).
2. **R5 (UWP App Icons Fix)**: Fully implemented in the Win32Explorer copy. The function `GetWindowIconFix` correctly resolves UWP apps using the Property Store, Shell Items, and CoreWindow executable fallback. In `SourceFiles/`, the code uses traditional `WM_GETICON`, which returns blank/generic icons for UWP apps.
3. **R7 (High-DPI Text Blurriness Fix)**: Partially implemented in the Win32Explorer copy. Scaling layout values and dynamic font height calculations are implemented during startup/layout calculations. However, **`WM_DPICHANGED` handling is missing in both locations**; the window does not scale dynamically when shifted to a monitor with a different scale or when the display settings change at runtime.
4. **R8 (About Dialog Spacing)**: Correct in the Win32Explorer copy. Height is increased to `245` and Y-coordinates of buttons/chin are pushed down to avoid overlaps. Broken in `SourceFiles/TaskbarProperties.cpp`, where the coordinates overlap and clip the text controls.
5. **R9 (Apply Button Hang)**: Fully implemented in the Win32Explorer copy. Setting broadcast runs in a background thread, and the PowerShell restart sequence uses sleep delays to prevent crashes. Missing in `SourceFiles/`.

---

## 3. Caveats

- **EliteSettings.ps1 status**: In earlier milestones, settings were managed via `EliteSettings.ps1`. In the active build chain (`build_settings.ps1`), settings are managed entirely by C++ (`EliteSettingsStub.cpp` and `TaskbarProperties.cpp`), which compile to `EliteSettings.exe` and are embedded in `EliteSettings.cpl`. Consequently, `EliteSettings.ps1` is abandoned and does not need to be updated.
- **Dragging limits**: Drag-and-drop actions on scraped tray icons do not support drag relocation due to OLE shell drag limitations, though basic clicks and mouse hovers are fully routed.

---

## 4. Conclusion

The implementation status is summarized below:

| Requirement | Win32Explorer Copy Status | Standalone (`SourceFiles/`) Status | Action Required |
|---|---|---|---|
| **R4: Tray Overflow Fix** | **Completed** (Double scraping, subclass, event routing) | **Missing** | Merge to `SourceFiles/`, add UI toggle. |
| **R5: UWP App Icons Fix** | **Completed** (GetWindowIconFix, shell item image factory) | **Missing** | Merge to `SourceFiles/`, replace raw icon calls. |
| **R7: High-DPI Fix** | **Partially Completed** (layout scale, font scaling) | **Missing** | Merge layout scaling. Add `WM_DPICHANGED` case to both. |
| **R8: About Dialog** | **Completed** (245 height, shifted Y coordinates) | **Broken** (overlaps) | Mirror correct heights/coordinates to `SourceFiles/`. |
| **R9: Apply Button Hang** | **Completed** (background thread, sleep delay) | **Missing** | Merge thread and powershell script logic to `SourceFiles/`. |

---

## 5. Verification Method

To verify the merged implementation, the following actions must be taken:

### 5.1. Build Command
From the repository root directory, run the primary build script:
```powershell
pwsh -File .\build.ps1
```
Verify that all outputs in `BuildOutput/` and `BuildOutputx86/` (`EliteTaskbar.exe`, `EliteSettings.exe`, `EliteSettings.cpl`, `Win32Explorer.exe`) compile successfully without errors or warnings.

### 5.2. UI Spacing & Overlaps (R8 Verification)
1. Run `EliteSettings.exe`.
2. Open the Help menu and click "About EliteTaskbar".
3. Click "More Info >>".
4. Verify that:
   - The dialog expands dynamically.
   - The buttons ("Less Info <<", "Okay") are fully visible at the bottom of the chin and do not overlap with the scrolling edit box.
   - The text inside the edit box is readable and does not clip.

### 5.3. High-DPI & dynamic scaling (R7 Verification)
1. Run `EliteTaskbar.exe`.
2. Move the taskbar window to a secondary monitor with a different DPI scale (e.g. 125% or 150%).
3. Verify that the taskbar window automatically scales its dimensions and redraws the fonts sharply.

### 5.4. UWP Icons (R5 Verification)
1. Launch UWP Apps (such as Windows Calculator or Settings).
2. Check the taskbar buttons.
3. Verify that they display their correct high-resolution UWP icons rather than a default system blank icon.

### 5.5. Tray Scraping & Click routing (R4 Verification)
1. Expand the tray overflow window.
2. Verify that all hidden tray icons are visible on the custom taskbar.
3. Hover over icons to confirm tooltips appear, and click/right-click icons to verify they trigger their native context menus.

---

## 6. Step-by-Step Guideline for the Worker to Merge & Sync

### Step 1: Prep and Layout Configuration in `resources.rc` and `resource.h`
1. Open `SourceFiles/resource.h` and add `#pragma warning(disable: 4715)` at the top.
2. Open `SourceFiles/resources.rc` and update `IDD_TASKBAR_PROPS` to include BOTH the "Taskbar Appearance" groupbox and the "Custom Icon Theme Folder" groupbox:
   - Keep "Taskbar Appearance" at `7, 58, 238, 77`.
   - Add "Custom Icon Theme Folder" below it, relocated to Y=`138` with height `75`:
     ```rc
     GROUPBOX        "Custom Icon Theme Folder", IDC_STATIC, 7, 138, 238, 75
     LTEXT           "Theme Path:", IDC_STATIC, 14, 153, 45, 8
     EDITTEXT        IDC_THEME_FOLDER_PATH, 65, 151, 120, 12, ES_AUTOHSCROLL | WS_TABSTOP
     PUSHBUTTON      "Browse...", IDC_THEME_FOLDER_BROWSE, 190, 150, 50, 14, WS_TABSTOP
     CONTROL         "Enable Dark Mode", IDC_ENABLE_DARK_MODE, "Button", BS_AUTOCHECKBOX | WS_DISABLED | WS_TABSTOP, 14, 175, 150, 10
     ```
3. Update `IDD_ABOUT_DIALOG` in `SourceFiles/resources.rc` to size `250, 140` and sync it with the Win32Explorer version:
   - Keep `IDC_ABOUT_MOREINFO` at `10, 145, 230, 60`.
   - Buttons must be shifted to Y=`118` (collapsed).
4. Update `Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/resources.rc` to match these layout changes.

### Step 2: Unify Settings Logic in `TaskbarProperties.cpp`
1. Open `SourceFiles/TaskbarProperties.cpp` and update `AboutDlgProc`:
   - Height when expanded: set `rcDlg = { 0, 0, 250, 245 }`.
   - Chin when expanded: set `rcChin = { 0, 215, 250, 245 }`.
   - Expand and OK buttons when expanded: shift Y to `223`.
   - Expand and OK buttons when collapsed: shift Y to `118` and chin to `110..140`.
2. Implement non-blocking `WM_SETTINGCHANGE` (R9) by copying `BroadcastSettingsChangeThread` and updating `NotifySettingsChange()` to invoke it via `CreateThread`.
3. In `TaskbarSettingsDlgProc`:
   - Implement tooltips and load/save logic for `IDC_THEME_FOLDER_PATH` and `IDC_THEME_FOLDER_BROWSE` using `GetEliteRegistryRoot()` to respect Portable Mode.
   - Keep existing width, preview, and portable mirror mode controls loaded and saved as well.
4. Copy these unified properties procedures and dialog mappings to `Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/TaskbarProperties.cpp`.

### Step 3: Implement Advanced Features in `TaskbarWindow.cpp`
1. Open `SourceFiles/TaskbarWindow.cpp` and merge:
   - The UWP Icon getter: `GetWindowIconFix(HWND hwnd)`. Replace all `WM_GETICON` calls with `GetWindowIconFix(hwnd)`.
   - The toolbar subclass helper structures, properties handlers, and `TrayToolbarSubclassProc` for routing clicks and hovers.
   - The `UpdateTaskbarLayout` function to resize tray and clock.
2. In `Initialize`:
   - Load monitor DPI via `shcore.dll`. Scale taskbar layout sizes (`startButtonWidth`, taskswitch, SysPager, clock) with `MulDiv(value, dpi, 96)`.
   - Set the toolbar subclass: `SetWindowSubclass(inst->hToolbar, TrayToolbarSubclassProc, 2, 0);`
3. Update `PreviewWndProc` and `TrayClockProc` to scale GDI+ font heights dynamically:
   - `lf.lfHeight = MulDiv(-12, dpi, 96)` and `lf.lfHeight = MulDiv(-11, dpi, 96)`.
4. In `WindowProc`, implement the missing `WM_DPICHANGED` message case:
   ```cpp
   case WM_DPICHANGED: {
       UINT newDpi = HIWORD(wParam);
       LPRECT lprcSuggested = (LPRECT)lParam;
       TaskbarInstance* inst = GetTaskbarInstance(hwnd);
       if (inst) {
           int baseHeight = 40;
           HKEY hKeySmall;
           if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 0, KEY_READ, &hKeySmall) == ERROR_SUCCESS) {
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
   ```
5. Merge all of these changes into `Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/TaskbarWindow.cpp` to maintain code synchronization.

### Step 4: Merge Double Scraping in `TrayIconScraper.cpp`
1. Open `SourceFiles/TrayIconScraper.cpp` and import `<map>` and `<string>`.
2. Define `g_TrayTooltipsMap` and `GetScrapedTrayTooltip()`.
3. Rewrite `ScrapeTrayIcons` to query `NotifyIconOverflowWindow` and scrape both toolbar windows using the `ScrapeTrayIconsFromToolbar` helper.
4. Copy these modifications to the Win32Explorer copy of `TrayIconScraper.cpp`.
