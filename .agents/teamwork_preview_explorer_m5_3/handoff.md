# Handoff Report: Milestone 5 Tray Icons Investigation

## 1. Observation
The following configurations, functions, and files were directly analyzed:
* **Registry Configuration & Settings Loading**:
  * In `SourceFiles/Config.h` (lines 23-32), `struct EliteTaskbarConfig` defines configurations (such as operational `Mode`, button styling, theme directories) but currently does not support the two-row tray layout flag.
  * In `SourceFiles/main.cpp` (lines 20-80), `QueryOperationalMode()` fetches the properties from `HKCU\Software\EliteSoftware\Win32Explorer\Advanced` (or `HKLM` depending on portable mirror settings) using the registry handle returned by `GetEliteRegistryRoot()`.
* **Tray Icon Scraper & Populations**:
  * In `SourceFiles/TrayIconScraper.cpp` (lines 40-126), `ScrapeTrayIcons` searches the explorer shell class structure (`Shell_TrayWnd` -> `TrayNotifyWnd` -> `SysPager` -> `ToolbarWindow32` and `NotifyIconOverflowWindow` -> `ToolbarWindow32`) and reads remote process memory to retrieve icons and tooltips.
  * In `SourceFiles/TrayIconScraper.cpp` (lines 128-163), `UpdateTrayToolbar(HWND hToolbar, HIMAGELIST hImageList, const std::vector<ScrapedTrayIcon>& icons)` populates the taskbar's system tray toolbar. Icons are appended to `hImageList`, and corresponding buttons are deleted/re-created using `TB_DELETEBUTTON` and `TB_ADDBUTTONS`.
* **Taskbar Sizing, Layout, and Creation**:
  * In `SourceFiles/TaskbarWindow.cpp` (lines 2503-2526), child windows for the system tray are initialized:
    * `inst->hSysPager` is a `SysPager` class.
    * `inst->hToolbar` is a `ToolbarWindow32` class.
    * `inst->hTrayImageList` is created on line 2516 via `ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 20)`.
  * In `SourceFiles/TaskbarWindow.cpp` (lines 318-369), `UpdateTaskbarLayout` calculates and adjusts the layout coordinates of `inst->hTrayNotify`, `inst->hSysPager`, `inst->hToolbar`, and `inst->hTrayClock`:
    * If `Mode == TaskbarMode::Replace` (line 322), the layout size is computed from `g_TrayIcons` and visibility. Width is computed as `W_tray = MulDiv(iconOffset + numDrawn * 24, dpi, 96)`.
    * If `Mode != TaskbarMode::Replace` (line 345), width is calculated dynamically using button counts and button sizes: `W_tray = btnCount * MulDiv(24, dpi, 96)`.
* **Tray Drawing and Mouse Event Interception**:
  * In `SourceFiles/TaskbarWindow.cpp` (lines 1052-1096), `TrayNotifyProc` paints notification icons when `Mode == TaskbarMode::Replace` is active. Icons are drawn using `DrawIconEx` with hardcoded `16x16` sizes and horizontal spacing increments `x += 24` on a single row starting at `y = (height - 16) / 2`.
  * In `SourceFiles/TaskbarWindow.cpp` (lines 1165-1179, 1290-1301), mouse events (e.g. clicks and hover tooltips) in `Replace` mode are hit-tested assuming a single row layout using the coordinate formula `(xPos - iconOffset) / 24` and vertical bounding checks.
* **Dynamic Settings Changes & Broadcast**:
  * In `SourceFiles/TaskbarWindow.cpp` (lines 2154-2187), the `WM_SETTINGCHANGE` handler intercepts registry notifications and triggers a PowerShell command to restart `explorer.exe` and `EliteTaskbar` when key layout parameters are modified.
* **CPL vs EXE Mirroring**:
  * In `SourceFiles/EliteSettingsCpl.cpp` (lines 4-48), the Control Panel Applet (CPL) does not contain separate settings drawing code. Instead, double-clicking it executes `RunEmbeddedExe`, which extracts `EliteSettings.exe` (stored as Resource ID `1` of type `RT_RCDATA` inside the CPL library) to a temp directory, runs it, waits for completion, and deletes it.
  * In `SourceFiles/TaskbarProperties.cpp` (lines 504-633), `TaskbarSettingsDlgProc` is the shared dialog procedure that reads/writes layout parameters and invokes `NotifySettingsChange()`.

---

## 2. Logic Chain
1. **Perfect Mirroring**: Since the Control Panel Applet (`EliteSettings.cpl`) extracts and executes `EliteSettings.exe` at runtime, modifying the UI layout in `resources.rc` and dialog event logic in `TaskbarProperties.cpp` automatically ensures 100% feature-parity and mirroring between both Settings interfaces.
2. **Registry Storage**: We should define the custom toggle key `EnableTwoRowTray` inside `HKCU\Software\EliteSoftware\Win32Explorer\Advanced`. To comply with Rule 1 of the project guidelines, when this key is missing (or first run), the default state must evaluate to `1` (true / enabled).
3. **UI Property Tab Integration**: To provide settings management, we must declare `IDC_TWO_ROW_TRAY` in `resource.h` and insert it as an auto-checkbox inside the dialog template `IDD_TASKBAR_PROPS` in `resources.rc`. The height of the `Taskbar Appearance` GroupBox can be stretched by 15 pixels, shifting the `Custom Icon Theme Folder` elements down without expanding the property page height limit (218 pixels).
4. **Restart & Rebuild Action**: Changing between single-row and two-row tray layouts modifies the window style (`TBSTYLE_WRAPABLE`) and image list size (`12x12` vs `16x16`). Hooking `EnableTwoRowTray` changes to trigger `requiresRestart = true` in `WM_SETTINGCHANGE` ensures the windows are re-created correctly and prevents memory leaks or layout warping.
5. **Non-Replace Mode Layout (Toolbar-based)**:
   * By adding the `TBSTYLE_WRAPABLE` style to the `ToolbarWindow32` during creation, the standard toolbar wraps buttons automatically when the window width is exceeded.
   * Creating `hTrayImageList` with `12x12` dimensions automatically scales the scraped icons. The standard button size becomes `20x20` pixels.
   * `W_tray` must be updated to `cols * btnWidth`, where `cols = (btnCount + 1) / 2` and `btnWidth = MulDiv(20, dpi, 96)`. Latching the toolbar width to this value causes the second half of the icons to wrap to the second row.
6. **Replace Mode Layout (GDI+ Custom Drawing & Hit-testing)**:
   * In `WM_PAINT`, if two-row layout is enabled, icons are drawn in a column-first order: column index `col = index / 2` and row index `row = index % 2`. `DrawIconEx` is invoked with `12x12` bounds at coordinates `x = startX + col * 18` and `y = topY + row * 14`.
   * Clicks and tooltips are hit-tested by mapping pointer coordinates to column (`col = (xPos - offset) / 18`) and row (`row = 0` if `yPos` is in top row bounds, `1` if in bottom row bounds). The icon index is reconstructed as `col * 2 + row`.

---

## 3. Caveats
* **Taskbar Heights**: The proposed implementation assumes the taskbar is standard height (~40px at 96 DPI), which has enough vertical headroom to draw two rows of `12x12` icons (occupying 26px total). On extremely small taskbars, the icons may clip.
* **Chevron Placement**: The chevron (expand button) used in `Replace` mode when `totalVisible > 4` is kept as a centered `16x16` character/icon for simplicity.
* **Offline Code-Only Investigation**: No files were modified, and the project was not compiled. The findings rely on detailed analysis of the C++ and resource structures.

---

## 4. Conclusion & Recommendation
It is recommended that the implementing agent:
1. Declare `IDC_TWO_ROW_TRAY` as `294` in `resource.h`.
2. Add the checkbox control in `resources.rc` within `IDD_TASKBAR_PROPS`.
3. Add a `bool EnableTwoRowTray` field in `EliteTaskbarConfig` in `Config.h`.
4. Read and write the setting in `main.cpp` and `TaskbarProperties.cpp` using `GetEliteRegistryRoot()` targeting `EnableTwoRowTray` defaulting to `1`.
5. Implement conditional `TBSTYLE_WRAPABLE` flag and `12x12` image list creation in `TaskbarWindow.cpp`'s tray creation block.
6. Implement two-row mathematics for `Replace` mode drawing, clicking, and tooltip positioning in `TrayNotifyProc` within `TaskbarWindow.cpp`.

### Proposed Implementation Details

#### 4.1. `SourceFiles/resource.h`
Add (line 89):
```cpp
#define IDC_TWO_ROW_TRAY             294
```

#### 4.2. `SourceFiles/resources.rc`
Modify `IDD_TASKBAR_PROPS` (lines 25-40):
```rc
    GROUPBOX        "Taskbar Appearance", IDC_STATIC, 7, 58, 238, 92
    LTEXT           "Button Width:", IDC_STATIC, 14, 73, 50, 8
    CONTROL         "Auto", IDC_WIDTH_AUTO, "Button", BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP, 65, 72, 35, 10
    CONTROL         "Fixed", IDC_WIDTH_FIXED, "Button", BS_AUTORADIOBUTTON, 105, 72, 35, 10
    CONTROL         "Icons Only", IDC_WIDTH_ICONS, "Button", BS_AUTORADIOBUTTON, 145, 72, 45, 10
    LTEXT           "Fixed Size:", IDC_WIDTH_FIXED_SIZE_LBL, 65, 87, 40, 8
    COMBOBOX        IDC_WIDTH_FIXED_SIZE, 105, 85, 80, 150, CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP
    CONTROL         "Show window previews", IDC_HOVER_PREVIEW, "Button", BS_AUTOCHECKBOX | WS_TABSTOP, 14, 103, 114, 10
    CONTROL         "Enable Portable Mirror Mode", IDC_PORTABLE_MIRROR, "Button", BS_AUTOCHECKBOX | WS_TABSTOP, 14, 118, 200, 10
    CONTROL         "Enable Two-Row System Tray", IDC_TWO_ROW_TRAY, "Button", BS_AUTOCHECKBOX | WS_TABSTOP, 14, 133, 200, 10

    GROUPBOX        "Custom Icon Theme Folder", IDC_STATIC, 7, 153, 238, 57
    LTEXT           "Theme Path:", IDC_STATIC, 14, 168, 45, 8
    EDITTEXT        IDC_THEME_FOLDER_PATH, 65, 166, 120, 12, ES_AUTOHSCROLL | WS_TABSTOP
    PUSHBUTTON      "Browse...", IDC_THEME_FOLDER_BROWSE, 190, 165, 50, 14, WS_TABSTOP
    CONTROL         "Enable Dark Mode", IDC_ENABLE_DARK_MODE, "Button", BS_AUTOCHECKBOX | WS_DISABLED | WS_TABSTOP, 14, 185, 150, 10
```

#### 4.3. `SourceFiles/Config.h`
Add `bool EnableTwoRowTray;` to `struct EliteTaskbarConfig`.

#### 4.4. `SourceFiles/main.cpp`
In `QueryOperationalMode()`:
```cpp
        dwValue = 1;
        bufferSize = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"EnableTwoRowTray", NULL, NULL, (LPBYTE)&dwValue, &bufferSize) == ERROR_SUCCESS) {
            g_Config.EnableTwoRowTray = (dwValue == 1);
        } else {
            g_Config.EnableTwoRowTray = true;
        }
```

#### 4.5. `SourceFiles/TaskbarProperties.cpp`
In `TaskbarSettingsDlgProc` under `WM_INITDIALOG`:
```cpp
        AddDlgTooltip(hwndDlg, IDC_TWO_ROW_TRAY, L"Stack notification area icons in two rows because horizontal space is precious.");
```
And:
```cpp
            cbData = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"EnableTwoRowTray", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                SendDlgItemMessageW(hwndDlg, IDC_TWO_ROW_TRAY, BM_SETCHECK, dwValue ? BST_CHECKED : BST_UNCHECKED, 0);
            } else {
                SendDlgItemMessageW(hwndDlg, IDC_TWO_ROW_TRAY, BM_SETCHECK, BST_CHECKED, 0); // Default to checked
            }
```
Under `WM_NOTIFY` -> `PSN_APPLY`:
```cpp
                DWORD twoRowTray = (SendDlgItemMessageW(hwndDlg, IDC_TWO_ROW_TRAY, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                RegSetValueExW(hKey, L"EnableTwoRowTray", 0, REG_DWORD, (const BYTE*)&twoRowTray, sizeof(DWORD));
```

#### 4.6. `SourceFiles/TaskbarWindow.cpp`
In `UpdateTaskbarLayout()`:
```cpp
        if (g_Config.Mode == TaskbarMode::Replace) {
            bool bIsExpanded = (GetPropW(inst->hTrayNotify, L"TrayExpanded") != NULL);
            bool bIsWin7Mode = (g_Config.OverflowMode == TrayOverflowMode::Win7Flyout);
            
            int totalVisible = 0;
            for (const auto& icon : g_TrayIcons) {
                if (icon.hIcon && !(icon.dwState & NIS_HIDDEN)) totalVisible++;
            }
            
            int numDrawn = totalVisible;
            int iconOffset = 2;
            if (totalVisible > 4) {
                iconOffset = 18;
                if (bIsWin7Mode) {
                    numDrawn = 4;
                } else {
                    if (!bIsExpanded) {
                        numDrawn = 4;
                    }
                }
            }
            
            if (g_Config.EnableTwoRowTray) {
                int colWidth = MulDiv(18, dpi, 96);
                int cols = (numDrawn + 1) / 2;
                W_tray = MulDiv(iconOffset, dpi, 96) + cols * colWidth;
            } else {
                W_tray = MulDiv(iconOffset + numDrawn * 24, dpi, 96);
            }
            if (!enableTray) W_tray = 0;
        } else {
            if (inst->hToolbar && enableTray) {
                int btnCount = (int)SendMessageW(inst->hToolbar, TB_BUTTONCOUNT, 0, 0);
                if (g_Config.EnableTwoRowTray) {
                    int btnWidth = MulDiv(20, dpi, 96);
                    int cols = (btnCount + 1) / 2;
                    W_tray = cols * btnWidth;
                } else {
                    int btnWidth = MulDiv(24, dpi, 96);
                    W_tray = btnCount * btnWidth;
                }
            } else {
                W_tray = 0;
            }
        }
```
And:
```cpp
        if (enableTray && inst->hSysPager) {
            SetWindowPos(inst->hSysPager, NULL, 0, 0, W_tray, taskbarHeight, SWP_NOZORDER | SWP_NOACTIVATE);
            if (inst->hToolbar) {
                if (g_Config.EnableTwoRowTray) {
                    int btnHeight = MulDiv(20, dpi, 96);
                    int toolbarY = (taskbarHeight - 2 * btnHeight) / 2;
                    SetWindowPos(inst->hToolbar, NULL, 0, toolbarY, W_tray, 2 * btnHeight, SWP_NOZORDER | SWP_NOACTIVATE);
                } else {
                    SetWindowPos(inst->hToolbar, NULL, 0, 0, W_tray, taskbarHeight, SWP_NOZORDER | SWP_NOACTIVATE);
                }
            }
        }
```
In window creation:
```cpp
                DWORD toolbarStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | CCS_NODIVIDER | CCS_NORESIZE;
                if (g_Config.EnableTwoRowTray) {
                    toolbarStyle |= TBSTYLE_WRAPABLE;
                }
                inst->hToolbar = CreateWindowExW(0, L"ToolbarWindow32", L"", toolbarStyle, 0, 0, MulDiv(100, dpi, 96), inst->taskbarHeight, inst->hSysPager, NULL, hInstance, NULL);
                SetWindowSubclass(inst->hToolbar, TrayToolbarSubclassProc, 2, 0);
                SendMessageW(inst->hToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
                
                int imageSize = g_Config.EnableTwoRowTray ? 12 : 16;
                inst->hTrayImageList = ImageList_Create(imageSize, imageSize, ILC_COLOR32 | ILC_MASK, 0, 20);
                SendMessageW(inst->hToolbar, TB_SETIMAGELIST, 0, (LPARAM)inst->hTrayImageList);
```
In `TrayNotifyProc`'s `WM_PAINT`:
```cpp
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
                
                if (g_Config.EnableTwoRowTray) {
                    int colWidth = 18;
                    int iconSize = 12;
                    int col = drawn / 2;
                    int row = drawn % 2;
                    int iconX = x + col * colWidth;
                    int topY = (rcClient.bottom - rcClient.top - (2 * iconSize + 2)) / 2;
                    int iconY = topY + row * (iconSize + 2);
                    DrawIconEx(hdc, iconX, iconY, icon.hIcon, iconSize, iconSize, 0, NULL, DI_NORMAL);
                } else {
                    DrawIconEx(hdc, x, y, icon.hIcon, 16, 16, 0, NULL, DI_NORMAL);
                    x += 24;
                }
                drawn++;
            }
        }
```
In `TrayNotifyProc`'s `WM_MOUSEMOVE` and mouse button click messages:
```cpp
        int iconOffset = (totalVisible > 4) ? 18 : 2;
        int iconIndex = -1;
        
        if (g_Config.EnableTwoRowTray) {
            int colWidth = 18;
            int iconSize = 12;
            int col = (xPos - iconOffset) / colWidth;
            int relativeX = (xPos - iconOffset) % colWidth;
            if (col >= 0 && relativeX >= 0 && relativeX < iconSize) {
                RECT rc;
                GetClientRect(hwnd, &rc);
                int height = rc.bottom - rc.top;
                int topY = (height - (2 * iconSize + 2)) / 2;
                int row = -1;
                if (yPos >= topY && yPos < topY + iconSize) {
                    row = 0;
                } else if (yPos >= topY + iconSize + 2 && yPos < topY + 2 * iconSize + 2) {
                    row = 1;
                }
                if (row != -1) {
                    iconIndex = col * 2 + row;
                }
            }
        } else {
            int tempIdx = (xPos - iconOffset) / 24;
            if (tempIdx >= 0) {
                RECT rc;
                GetClientRect(hwnd, &rc);
                int height = rc.bottom - rc.top;
                int iconY = (height - 16) / 2;
                int relativeX = (xPos - iconOffset) % 24;
                if (relativeX >= 0 && relativeX < 16 && yPos >= iconY && yPos < iconY + 16) {
                    iconIndex = tempIdx;
                }
            }
        }
```
Under `WM_SETTINGCHANGE` inside `TaskbarWindow.cpp`:
```cpp
                cbData = sizeof(DWORD);
                if (RegQueryValueExW(hKey, L"EnableTwoRowTray", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                    bool newTwoRow = (dwValue == 1);
                    if (newTwoRow != g_Config.EnableTwoRowTray) {
                        requiresRestart = true;
                    }
                }
```

---

## 5. Verification Method
1. **Compilation Check**:
   * Open a PowerShell terminal and run:
     ```powershell
     $env:ELITE_AUDITOR_RUN="1"
     .\build.ps1
     ```
   * Ensure compilation succeeds for both `EliteTaskbar.exe` and `EliteSettings.exe` across x86 and x64 targets.
2. **Settings Dialog Validation**:
   * Run `EliteSettings.exe` or open `EliteSettings.cpl`.
   * Navigate to the **Taskbar** properties tab.
   * Confirm the checkbox `Enable Two-Row System Tray` is visible and default-checked.
   * Check/uncheck the setting and click **Apply** / **Okay**; verify that it correctly alters the `EnableTwoRowTray` DWORD registry entry under `Software\EliteSoftware\Win32Explorer\Advanced` and triggers taskbar process restart.
3. **Tray Rendering and Spacing Test**:
   * Run the taskbar in both `Replace` and `Independent` modes.
   * When `EnableTwoRowTray` is enabled, verify that tray icons are drawn in two stacked rows, scaled down cleanly to `12x12` pixels, and are packed closely.
   * When `EnableTwoRowTray` is disabled, verify they fall back to the classic single-row `16x16` layout.
   * Hover over the top-row and bottom-row icons and click them to verify tooltips and mouse callbacks function accurately.
