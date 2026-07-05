# Handoff Report: Read-Only Investigation of EliteTaskbar R3, R4, R5, and R7

## 1. Observation

We have performed a read-only code exploration of the EliteTaskbar repository under `SourceFiles/`. The following observations were made:

### A. Default Operational Settings (R3)
- In `SourceFiles/main.cpp`, lines 17-17, the global configuration structure `g_Config` is initialized as:
  ```cpp
  EliteTaskbarConfig g_Config = { L"", TaskbarMode::Independent, ButtonWidthMode::Auto, TrayOverflowMode::Win7Flyout, false, false, {} };
  ```
- In `SourceFiles/main.cpp`, lines 23-23, inside `QueryOperationalMode()`:
  ```cpp
  g_Config.Mode = TaskbarMode::Independent; // Default
  ```
- In `SourceFiles/TaskbarProperties.cpp`, lines 448-454, when initializing the Settings properties dialog:
  ```cpp
  if (RegQueryValueExW(hKey, L"TaskbarMode", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
      if (dwValue == 0) SendDlgItemMessageW(hwndDlg, IDC_MODE_INDEPENDENT, BM_SETCHECK, BST_CHECKED, 0);
      else if (dwValue == 1) SendDlgItemMessageW(hwndDlg, IDC_MODE_REPLACE, BM_SETCHECK, BST_CHECKED, 0);
      else if (dwValue == 2) SendDlgItemMessageW(hwndDlg, IDC_MODE_SECONDARY_ONLY, BM_SETCHECK, BST_CHECKED, 0);
  } else {
      SendDlgItemMessageW(hwndDlg, IDC_MODE_INDEPENDENT, BM_SETCHECK, BST_CHECKED, 0);
  }
  ```

### B. System Tray Overflow and Interactions (R4)
- **Loss of Mouse Input:** In `SourceFiles/TaskbarWindow.cpp`, the tray pager window `inst->hSysPager` and toolbar `inst->hToolbar` (created at lines 1882-1883) are not subclassed.
- **Lost Notifications:** The main window procedure `WindowProc` only handles `WM_NOTIFY` from the taskband (`inst->hTaskSwitch`) for item previews/menus (lines 1267-1320). No `WM_NOTIFY` or mouse event routing is implemented for `inst->hToolbar`. Consequently, mouse interactions (clicks, right-clicks, dragging) on the tray icons are entirely lost.
- **Scraper Incompleteness:** The current scraping function `ScrapeTrayIcons()` in `SourceFiles/TrayIconScraper.cpp` (lines 27-75) only queries the visible tray toolbar `ToolbarWindow32` under `Shell_TrayWnd`. It completely ignores the overflow area (`NotifyIconOverflowWindow` -> `ToolbarWindow32`), causing icons in the overflow menu to be cut off and missing from the replicated taskbar.
- **Replacement Mode Click Mapping:** In `SourceFiles/TaskbarWindow.cpp` (lines 802-825), the click mapping divides mouse coordinates without verifying if the click coordinates hit within the specific 16px bound of the icons.

### C. UWP App Icons Extraction (R5)
- In `SourceFiles/TaskbarWindow.cpp`, window icons are retrieved using standard GDI messages (lines 439-442, 1134-1140, 1955-1960):
  ```cpp
  if (!SendMessageTimeoutW(hwnd, WM_GETICON, ICON_SMALL, 0, SMTO_ABORTIFHUNG, 500, &dwRes)) dwRes = 0;
  info.hIcon = (HICON)dwRes;
  if (!info.hIcon) info.hIcon = (HICON)GetClassLongPtrW(hwnd, GCLP_HICONSM);
  ```
- Windows of class `"ApplicationFrameWindow"` are host windows owned by `ApplicationFrameHost.exe`. Sending `WM_GETICON` or querying `GCLP_HICONSM` on these handles returns the default generic icon of `ApplicationFrameHost.exe` instead of the hosted UWP application's icon (e.g. Settings, Calculator, Mail).

### D. Multi-Monitor DPI Blurriness (R7)
- While `SourceFiles/app.manifest` defines `PerMonitorV2` DPI awareness (line 20), there is no handling of `WM_DPICHANGED` messages in `WindowProc` (lines 1108-1320).
- Static layout variables are used: `taskbarHeight` is a local initialization variable (line 1702), and font heights are hardcoded in paint handlers:
  - Previews (line 229): `lf.lfHeight = -12;`
  - Clock (line 890): `lf.lfHeight = -11;`
- Consequently, when moving across multi-monitor setups, DWM performs bitmap scaling, rendering the taskbar text and icons blurry on monitors that differ from the primary monitor's scaling factor.

---

## 2. Logic Chain

### A. Operational Mode (R3)
- Based on observations in `main.cpp` (lines 17, 23) and `TaskbarProperties.cpp` (lines 448-454), `g_Config.Mode` defaults to `TaskbarMode::Independent` if registry parameters are absent or set to `0`. This satisfies the default settings requirement.

### B. Tray Interactions & Overflow (R4)
- **Click Routing:** Subclassing `inst->hToolbar` (e.g. `TrayToolbarSubclassProc`) is necessary to capture client-area mouse messages (`WM_LBUTTONUP`, `WM_RBUTTONUP`, etc.). Sending `TB_HITTEST` retrieves the button index under the cursor, which maps directly to the scraped item in `g_CurrentTrayIcons`. We can then route the messages using `PostMessageW(icon.hwnd, icon.uCallbackMessage, icon.uID, uMsg)`.
- **Tooltip Mapping:** Since `tbb.iString` in Explorer's tray toolbar contains a pointer to the Unicode tooltip string, we can read this string from the remote process using `ReadProcessMemory` during scraping. Enabling `TBSTYLE_TOOLTIPS` on `inst->hToolbar` and subclassing `inst->hSysPager` to handle `TTN_GETDISPINFOW` enables the native tooltip system to query and display these tooltips automatically.
- **Double-Scraping:** Finding both `Shell_TrayWnd` (visible) and `NotifyIconOverflowWindow` (overflow) toolbar controls, and scraping both into the same collection, prevents icons from being cut off.
- **Dynamic Layout:** Querying `TB_GETITEMRECT` for the last button inside the tray toolbar gives the exact client width required by the icons. We can then adjust the width of `SysPager` and shift the clock to `x = W_tray`, updating the parent `TrayNotifyWnd` and taskband dimensions accordingly.

### C. UWP App Icons (R5)
- UWP windows have class name `"ApplicationFrameWindow"`.
- We can extract the unique App User Model ID (AUMID) by querying the window's `IPropertyStore` for the `PKEY_AppUserModel_ID` key.
- By parsing this AUMID inside `shell:AppsFolder\\` via `SHCreateItemFromParsingName`, we obtain an `IShellItem`. Querying `IShellItemImageFactory::GetImage` retrieves the clean 32-bit ARGB bitmap of the UWP application.
- Converting this bitmap to `HICON` via `CreateIconIndirect` provides the correct high-res icon.
- As a robust fallback, locating child windows of class `"Windows.UI.Core.CoreWindow"`, obtaining their PID, querying the image path using `QueryFullProcessImageNameW`, and calling `SHGetFileInfoW` extracts the native application icon.

### D. Multi-Monitor High-DPI Rendering (R7)
- Handling `WM_DPICHANGED` captures the new DPI scaling factor.
- Storing `dpi` in `TaskbarInstance` allows dynamically scaling font sizes:
  `lf.lfHeight = MulDiv(baseFontHeight, dpi, 96);`
- Redefining layout parameters (e.g. `inst->taskbarHeight = MulDiv(inst->baseTaskbarHeight, dpi, 96)`) and resizing window boundaries using `SetWindowPos` according to the suggested rectangle (`lParam`) ensures crisp, native rendering on all monitors.

---

## 3. Caveats

- **COM Initialization:** Calling shell items from `shell:AppsFolder` requires COM initialization. The application calls `CoInitializeEx` on startup, but thread apartment rules must be strictly adhered to during shell operations.
- **OS Version Compatibility:** On older systems (Windows 7/8), `PKEY_AppUserModel_ID` may not be present on frame windows, so the child `CoreWindow` query / `SHGetFileInfoW` executable path fallback is critical for backward compatibility.
- **Registry Permissions:** Querying the root returned by `GetEliteRegistryRoot()` protects registry configurations across portable/installed modes.

---

## 4. Conclusion

The analysis validates the following implementation strategies:
1. **R3 Default settings:** The codebase already defaults to `TaskbarMode::Independent`.
2. **R4 Tray Overflow & Mouse routing:**
   - Double-scrape visible toolbar and `"NotifyIconOverflowWindow"`.
   - Read tooltips using remote process memory scraping (`tbb.iString`).
   - Subclass `inst->hToolbar` (routing click messages to target windows) and `inst->hSysPager` (providing tooltip strings via `TTN_GETDISPINFOW`).
   - Dynamically resize `hSysPager` using `TB_GETITEMRECT` bounds of the last button, and dynamically adjust the positions of the clock and parent notifier.
3. **R5 UWP Icons:**
   - Check if class is `"ApplicationFrameWindow"`.
   - Extract AUMID via Property Store, instantiate `IShellItem` and retrieve `HBITMAP` via `IShellItemImageFactory::GetImage`, converting it to `HICON`.
   - Fallback to reading the executable path of child `"Windows.UI.Core.CoreWindow"`.
4. **R7 High-DPI text scaling:**
   - Initialize `inst->dpi` on monitor startup via `GetDpiForMonitor`.
   - Handle `WM_DPICHANGED` to dynamically scale taskbar height, start button size, clock, and repainting.
   - Scale `lf.lfHeight` in `PreviewWndProc` and `TrayClockProc` by `MulDiv(baseHeight, dpi, 96)`.

A detailed patch file has been generated and saved at:
`C:\Users\Administrator\Desktop\Elite-TaskBar\._agents\explorer_r4_r5_r7_gen2\proposed_changes.patch`

---

## 5. Verification Method

To verify the proposed implementation:
1. **Compilation:** Execute `build.ps1` to build the binary. Ensure it compiles without warnings/errors.
2. **Tray Interactions:**
   - Run the taskbar. Verify that clicking, right-clicking, double-clicking, and hovering over scraped tray icons correctly invokes their menus/dialogs.
   - Ensure the chevron menu (`NotifyIconOverflowWindow`) items are correctly scraped and populated.
3. **UWP Icons:**
   - Open UWP applications (e.g. Settings, Calculator). Confirm that their distinct icons are displayed on the taskbar.
4. **High-DPI Scaling:**
   - Drag the EliteTaskbar window between monitors with different scaling sizes (e.g., 100%, 125%, 150%).
   - Verify that the text and clock remain crisp, and layout bounds resize dynamically on `WM_DPICHANGED`.
