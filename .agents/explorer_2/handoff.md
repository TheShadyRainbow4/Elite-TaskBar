# Handoff Report: Desktop Background Configuration Resource Design (Milestone 7)

## 1. Observation
* **Observed Files and Paths**:
  * **Primary UI Definition Files**:
    * `SourceFiles/resource.h` (Lines 84-90):
      ```cpp
      #define IDC_DESKTOP_REPLACE_ENABLED   290
      #define IDC_DESKTOP_WALLPAPER_ENABLED 291
      #define IDC_DESKTOP_ICONS_ENABLED     292
      #define IDC_FALLBACK_STARTMENU_ENABLED 293
      #define IDC_TWO_ROW_TRAY              294
      #define IDC_MIGRATE_START_MENU_SETTINGS 295
      ```
    * `SourceFiles/resources.rc` (Lines 89-98):
      ```rc
      IDD_DESKTOP_PROPS DIALOGEX 0, 0, 252, 218
      STYLE DS_SETFONT | WS_CHILD | WS_DISABLED | WS_CAPTION | DS_CONTROL
      CAPTION "Desktop"
      FONT 8, "Segoe UI Semibold", 600, 0, 0x1
      BEGIN
          GROUPBOX        "Desktop Replacement Settings", IDC_STATIC, 7, 7, 238, 70
          CONTROL         "Enable custom desktop window replacement", IDC_DESKTOP_REPLACE_ENABLED, "Button", BS_AUTOCHECKBOX | WS_TABSTOP, 14, 20, 220, 10
          CONTROL         "Draw desktop wallpaper background", IDC_DESKTOP_WALLPAPER_ENABLED, "Button", BS_AUTOCHECKBOX | WS_TABSTOP, 14, 35, 220, 10
          CONTROL         "Show desktop icon grid (SysListView32)", IDC_DESKTOP_ICONS_ENABLED, "Button", BS_AUTOCHECKBOX | WS_TABSTOP, 14, 50, 220, 10
      END
      ```
    * `SourceFiles/TaskbarProperties.cpp` (Lines 1436-1464, lines 1554-1555):
      * `INT_PTR CALLBACK DesktopSettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)` handles settings page events and reads/writes registry values `DesktopReplacementEnabled`, `DesktopWallpaperEnabled`, and `DesktopIconsEnabled` under `Software\EliteSoftware\Win32Explorer\Advanced`.
      * `psp[6].pszTemplate = MAKEINTRESOURCEW(IDD_DESKTOP_PROPS);` maps the page template.
  * **Mirrored Win32Explorer Files**:
    * `Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/resource.h` contains identical control macro definitions matching `SourceFiles/resource.h`.
    * `Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/resources.rc` contains an identical template definition for `IDD_DESKTOP_PROPS` matching `SourceFiles/resources.rc` (Lines 88-97).
  * **Path Discrepancy**:
    * The directory `Remaining_Shell` does not exist in the root of the workspace. A search directory query for `C:\Users\Administrator\Desktop\Elite-TaskBar\Remaining_Shell` returned:
      > `Encountered error in step execution: directory C:\Users\Administrator\Desktop\Elite-TaskBar\Remaining_Shell does not exist`
    * The actual path of the mirrored source files is: `C:\Users\Administrator\Desktop\Elite-TaskBar\Win32Explorer_26.0.3.0\App_Source\EliteTaskbar\resources.rc`.
  * **Reserved Identifier Ranges**:
    * IDs 301-307 are reserved in both resource headers for Start Orb bitmaps (`IDB_START_ORB_1ORB` through `IDB_START_ORB_VIENNABOTTOM`).

## 2. Logic Chain
1. **Control ID Selection**:
   * The highest control ID defined in the 200-series block of `resource.h` is `IDC_MIGRATE_START_MENU_SETTINGS` (295).
   * The IDs 301-307 are occupied by Start Orb bitmaps (`IDB_START_ORB_...`).
   * To keep control identifiers grouped, continuous, and clear from bitmap resources, we select the range **320 to 328** as the target block for the new controls.
   * **Assignments**:
     * `IDC_DESKTOP_FORCE_PROGMAN_ALL` = `320`: Checkbox to forcefully render Progman on all displays.
     * `IDC_DESKTOP_MODE_SPAN` = `321`: Radio button for Span background mode.
     * `IDC_DESKTOP_MODE_PERMONITOR` = `322`: Radio button for Per-monitor background mode.
     * `IDC_DESKTOP_THEME_PATH` = `323`: Edit control for custom theme directory path.
     * `IDC_DESKTOP_THEME_BROWSE` = `324`: Button to browse directory paths.
     * `IDC_DESKTOP_THEME_SELECT` = `325`: ComboBox for theme dropdown selection.
     * `IDC_DESKTOP_SLIDESHOW_ENABLED` = `326`: Checkbox to toggle wallpaper slideshow.
     * `IDC_DESKTOP_SLIDESHOW_INTERVAL` = `327`: ComboBox (drop-down type allowing custom text/edit inputs) for slideshow timing interval.
     * `IDC_DESKTOP_THEME_TUTORIAL_LINK` = `328`: SysLink control representing a hyperlink to theme documentation.

2. **UI Layout Design (`IDD_DESKTOP_PROPS`)**:
   * The page dimensions are `252 x 218` DUs.
   * The existing dialog uses a `GROUPBOX` ("Desktop Replacement Settings") spanning from `y=7` to `y=77` (height 70) containing 3 checkboxes.
   * We expand this groupbox height to `75` DUs to house the new "Forcefully render Progman on all displays" checkbox at `y=65`.
   * We place a second `GROUPBOX` ("Wallpaper & Theme Configuration") in the remaining space from `y=85` down to `y=211` (height 126), matching the `7` DU margin.
   * Inside this second groupbox:
     * Radios are placed side-by-side or stacked at `y=98` and `y=110`. The first radio has `WS_GROUP` to start a new radio group.
     * The theme path edit and browse button are placed horizontally at `y=122-125` (`x=65` for edit, `x=190` for browse).
     * The theme dropdown combobox is placed at `y=140` (`x=65`).
     * The slideshow controls are placed horizontally at `y=157-159` (`x=14` for check, `x=130` for label, `x=165` for interval).
     * The theme tutorial SysLink is placed at the bottom, `y=185` (`x=14`).

3. **Mirroring Strategy**:
   * Manual mirroring across `SourceFiles/` and `Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/` is highly error-prone.
   * To achieve a robust, single-source-of-truth strategy:
     1. Add an automated pre-build copy step to `build.ps1` that mirrors `SourceFiles/resources.rc` and `SourceFiles/resource.h` to the corresponding folders in `Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/` before invoking the secondary build scripts.
     2. In code modifications, the settings implementation in `TaskbarProperties.cpp` should define the tooltip strings and event handlers inside `DesktopSettingsDlgProc`.

## 3. Caveats
* **Path Assumptions**:
  * The prompt references the path `Remaining_Shell/Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/resources.rc`. As observed, the `Remaining_Shell` prefix directory is not present in the workspace, so the mirroring process must target the workspace directory `Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/` instead.
* **Common Control Dependencies**:
  * The `SysLink` control (class name `"SysLink"`) requires common controls version 6.0 (which is already configured via `app.manifest`). Inside `TaskbarProperties.cpp`, clicking the link will generate a `WM_NOTIFY` message with a notification code of `NM_CLICK` or `NM_RETURN`. The dialog procedure must intercept this message and call `ShellExecuteW` to open the URL.

## 4. Conclusion
A detailed layout plan, patch file, and matching control IDs have been designed and written to the patch file:
`C:\Users\Administrator\Desktop\Elite-TaskBar\.agents\explorer_2\desktop_background_settings.patch`

### Proposed Resource Header Definitions
```cpp
#define IDC_DESKTOP_FORCE_PROGMAN_ALL   320
#define IDC_DESKTOP_MODE_SPAN           321
#define IDC_DESKTOP_MODE_PERMONITOR      322
#define IDC_DESKTOP_THEME_PATH          323
#define IDC_DESKTOP_THEME_BROWSE        324
#define IDC_DESKTOP_THEME_SELECT        325
#define IDC_DESKTOP_SLIDESHOW_ENABLED   326
#define IDC_DESKTOP_SLIDESHOW_INTERVAL  327
#define IDC_DESKTOP_THEME_TUTORIAL_LINK 328
```

### Proposed Dialog Layout Block (`IDD_DESKTOP_PROPS`)
```rc
IDD_DESKTOP_PROPS DIALOGEX 0, 0, 252, 218
STYLE DS_SETFONT | WS_CHILD | WS_DISABLED | WS_CAPTION | DS_CONTROL
CAPTION "Desktop"
FONT 8, "Segoe UI Semibold", 600, 0, 0x1
BEGIN
    GROUPBOX        "Desktop Replacement Settings", IDC_STATIC, 7, 7, 238, 75
    CONTROL         "Enable custom desktop window replacement", IDC_DESKTOP_REPLACE_ENABLED, "Button", BS_AUTOCHECKBOX | WS_TABSTOP, 14, 20, 220, 10
    CONTROL         "Draw desktop wallpaper background", IDC_DESKTOP_WALLPAPER_ENABLED, "Button", BS_AUTOCHECKBOX | WS_TABSTOP, 14, 35, 220, 10
    CONTROL         "Show desktop icon grid (SysListView32)", IDC_DESKTOP_ICONS_ENABLED, "Button", BS_AUTOCHECKBOX | WS_TABSTOP, 14, 50, 220, 10
    CONTROL         "Forcefully render Progman on all displays", IDC_DESKTOP_FORCE_PROGMAN_ALL, "Button", BS_AUTOCHECKBOX | WS_TABSTOP, 14, 65, 220, 10

    GROUPBOX        "Wallpaper & Theme Configuration", IDC_STATIC, 7, 85, 238, 126
    CONTROL         "Span wallpaper across all displays", IDC_DESKTOP_MODE_SPAN, "Button", BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP, 14, 98, 220, 10
    CONTROL         "Per-monitor wallpaper mode", IDC_DESKTOP_MODE_PERMONITOR, "Button", BS_AUTORADIOBUTTON | WS_TABSTOP, 14, 110, 220, 10
    LTEXT           "Theme path:", IDC_STATIC, 14, 125, 48, 8
    EDITTEXT        IDC_DESKTOP_THEME_PATH, 65, 123, 120, 12, ES_AUTOHSCROLL | WS_TABSTOP
    PUSHBUTTON      "Browse...", IDC_DESKTOP_THEME_BROWSE, 190, 122, 45, 14
    LTEXT           "Active Theme:", IDC_STATIC, 14, 142, 48, 8
    COMBOBOX        IDC_DESKTOP_THEME_SELECT, 65, 140, 170, 80, CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP
    CONTROL         "Enable wallpaper slideshow", IDC_DESKTOP_SLIDESHOW_ENABLED, "Button", BS_AUTOCHECKBOX | WS_TABSTOP, 14, 158, 110, 10
    LTEXT           "Interval:", IDC_STATIC, 130, 159, 32, 8
    COMBOBOX        IDC_DESKTOP_SLIDESHOW_INTERVAL, 165, 157, 70, 80, CBS_DROPDOWN | WS_VSCROLL | WS_TABSTOP
    CONTROL         "<a>Theme customization tutorial</a>", IDC_DESKTOP_THEME_TUTORIAL_LINK, "SysLink", WS_TABSTOP, 14, 185, 220, 10
END
```

### Proposed Tooltip Mappings (`TaskbarProperties.cpp`)
In `DesktopSettingsDlgProc` -> `WM_INITDIALOG`:
```cpp
AddDlgTooltip(hwndDlg, IDC_DESKTOP_FORCE_PROGMAN_ALL, L"Forcibly injects our Progman replacement onto every screen. Because one display is never enough for Elite performance.");
AddDlgTooltip(hwndDlg, IDC_DESKTOP_MODE_SPAN, L"Stretches your wallpaper across your entire multi-monitor setup, hoping the aspect ratios align.");
AddDlgTooltip(hwndDlg, IDC_DESKTOP_MODE_PERMONITOR, L"Allows each display to render its own distinct wallpaper, like a true professional workspace.");
AddDlgTooltip(hwndDlg, IDC_DESKTOP_THEME_PATH, L"Specifies the directory path of your custom desktop wallpaper themes. Choose wisely.");
AddDlgTooltip(hwndDlg, IDC_DESKTOP_THEME_BROWSE, L"Browse directory folders to discover where your hidden themes are stored.");
AddDlgTooltip(hwndDlg, IDC_DESKTOP_THEME_SELECT, L"Select your active visual theme. Instant aesthetic upgrade.");
AddDlgTooltip(hwndDlg, IDC_DESKTOP_SLIDESHOW_ENABLED, L"Periodically rotates wallpapers. Keeps your eyes occupied while the system works.");
AddDlgTooltip(hwndDlg, IDC_DESKTOP_SLIDESHOW_INTERVAL, L"Select how long a single wallpaper remains before being unceremoniously swapped out.");
```

## 5. Verification Method
* **Independent Verification**:
  1. Apply the patch `desktop_background_settings.patch` using git or patch utility:
     `git apply --check .agents/explorer_2/desktop_background_settings.patch`
  2. Verify resources compile cleanly via MSVC Resource Compiler:
     `rc /i SourceFiles SourceFiles/resources.rc`
     `rc /i Win32Explorer_26.0.3.0/App_Source/EliteTaskbar Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/resources.rc`
  3. Compile the project using the master build command:
     `powershell -File build.ps1`
  4. Once compiled, run the properties settings UI:
     `EliteTaskbar.exe /settings`
     Navigate to the "Desktop" tab and verify the layout, controls alignment, tab navigation sequence, and hover tooltips render correctly.
