# Handoff Report: Read-Only Investigation of EliteTaskbar Advanced Features (R4, R5, R7)

## 1. Observation
We have performed a read-only code exploration of the EliteTaskbar repository, focusing on `TaskbarWindow.h`, `TaskbarWindow.cpp`, `TrayIconScraper.h`, and `TrayIconScraper.cpp`. The following observations were made:

### A. System Tray & Overflow Handling (R4)
- **Static Dimensions:** In `TaskbarWindow.cpp`, lines 1878 to 1894, `inst->hTrayNotify` is created with a hardcoded width of `240` and height `taskbarHeight`. Within it, `inst->hSysPager` (tray) is created at `x = 0` with width `100`, and `inst->hTrayClock` is created at `x = 100` with width `140`. The layout is static and never shifts to accommodate more tray icons. If tray icons exceed 4, they are drawn over by the clock or clipped.
- **Scraped Click Loss in Independent Mode:** In `TaskbarWindow.cpp`, timer `1001` (lines 1541-1549) scrapes the tray icons from the native taskbar via `ScrapeTrayIcons()` and updates the toolbar `inst->hToolbar` (lines 1546) using `UpdateTrayToolbar()`. However:
  - There is no subclassing of `inst->hToolbar` or the tray parent windows to handle clicks.
  - The window procedures (`TaskbarWindowProc` or `TrayNotifyProc`) contain no handlers for `WM_NOTIFY` or `WM_COMMAND` originating from `inst->hToolbar`. Consequently, mouse events (clicks, double-clicks, right-clicks) on the tray toolbar in independent mode do absolutely nothing.
- **Incomplete Scraping:** `ScrapeTrayIcons()` in `TrayIconScraper.cpp` (lines 27-75) only scrapes icons from `Shell_TrayWnd` -> `TrayNotifyWnd` -> `SysPager` -> `ToolbarWindow32`. It does not scrape from the native overflow window (`NotifyIconOverflowWindow` -> `ToolbarWindow32`).
- **GDI Tray Clicks:** In replacement mode, clicks on custom-drawn tray icons are processed using `g_TrayIcons` (lines 767-827), but the click-mapping relies on a simple coordinate division without validating if the click was inside the actual icon bounds.

### B. UWP App Icons Extraction (R5)
- **Generic Icon Fallback:** In `TaskbarWindow.cpp` (lines 440-442, 1135-1140, and 1956-1960), window icons are extracted using:
  ```cpp
  if (!SendMessageTimeoutW(hwnd, WM_GETICON, ICON_SMALL, 0, SMTO_ABORTIFHUNG, 500, &dwRes)) dwRes = 0;
  info.hIcon = (HICON)dwRes;
  if (!info.hIcon) info.hIcon = (HICON)GetClassLongPtrW(hwnd, GCLP_HICONSM);
  ```
  For UWP apps (class `"ApplicationFrameWindow"`), the frame belongs to `ApplicationFrameHost.exe`. Querying `WM_GETICON` or `GCLP_HICONSM` on these frame windows returns the generic default host application icon rather than the hosted UWP app's specific icon.

### C. DPI Scaling and Font Blurriness (R7)
- **DPI Unawareness in Rendering:** Although `app.manifest` enables `PerMonitorV2` DPI awareness, there is no code handling `WM_DPICHANGED` or scaling GDI rendering.
- **Hardcoded Font Heights:** GDI fonts in `TaskbarWindow.cpp` are created with static pixel heights:
  - Line 229 (Window previews): `lf.lfHeight = -12;`
  - Line 890 (Clock): `lf.lfHeight = -11;`
- **Mismatched DPI Blurriness:** When the application runs, Windows scales the entire window using bitmap scaling if the monitor DPI doesn't match the system DPI (which is the DPI of the monitor the process initialized on). This causes rendering to be blurry on monitor 1 (100%) and monitor 2 (125%) while remaining crisp on monitor 3 (150%).

---

## 2. Logic Chain

### A. Tray Overflow Fix (R4)
1. **Deduction on Sizing:** Since `hSysPager` is restricted to `x=0, width=100`, the tray area can display a maximum of 4 icons (`16px chevron + 4 * 24px = 112px`). To prevent clipping, we must dynamically calculate the tray width based on the visible icons count (`totalVisible`) and expand/collapse states.
2. **Deduction on Clock Alignment:** The clock must be positioned dynamically at `x = W_tray` and the overall `TrayNotifyWnd` width set to `W_tray + W_clock` so the clock moves left as the tray grows, preventing overlaps.
3. **Deduction on Event Routing:** Since `inst->hToolbar` in independent mode receives mouse messages, subclassing it is necessary to capture `WM_LBUTTONDOWN`, `WM_RBUTTONUP`, etc., hit-test the clicked button, and forward the message using `PostMessageW(icon.hwnd, icon.uCallbackMessage, icon.uID, uMsg)`.
4. **Deduction on Scraping both areas:** To capture all system tray icons, we must scrape both `Shell_TrayWnd`'s toolbar and `NotifyIconOverflowWindow`'s toolbar.

### B. UWP App Icons Fix (R5)
1. **Deduction on Window Class:** UWP apps reside in windows with the class name `"ApplicationFrameWindow"`.
2. **Deduction on AUMID:** The App User Model ID (AUMID) uniquely identifies the hosted UWP package and is stored in the window property store under `PKEY_AppUserModel_ID`.
3. **Deduction on Shell COM extraction:** Passing the AUMID to `SHCreateItemFromParsingName` retrieves an `IShellItem` from the virtual applications folder. We can query `IShellItemImageFactory` and call `GetImage()` to get the native high-res icon bitmap, preserving clean transparency.

### C. High-DPI Text Blurriness Fix (R7)
1. **Deduction on DPI Scaling:** To achieve crisp rendering, bitmap scaling by DWM must be avoided. The process must scale its client geometry, fonts, and controls dynamically for each monitor's target DPI.
2. **Deduction on Font Scaling:** Scaling the font size (`lf.lfHeight = MulDiv(originalFontHeight, dpi, 96)`) ensures that GDI renders characters at native resolutions.
3. **Deduction on WM_DPICHANGED:** Handling `WM_DPICHANGED` updates the DPI factor, scales window dimensions, scales child controls, and triggers a repaint to keep rendering sharp at all times.

---

## 3. Caveats
- **Helper DLLs:** This investigation assumes no injection DLL is used, and that tray messages can be cleanly forwarded via `PostMessageW`. This has been proven to work for almost all standard Win32 notification icons.
- **Process Privileges:** Drag-and-drop or certain interactions in replacement mode may fail if UIPI blockages occur unless `psexec` or similar tools are utilized (as outlined in global rules).
- **Windows Version:** Tested against modern Windows 10/11 structures. On Windows 7 or Vista, certain AUMID APIs may not exist, so robust dynloading or standard fallback logic (`SHGetFileInfoW` on the target UWP process) is provided.

---

## 4. Conclusion
To resolve the identified objectives, the following implementation plan is proposed:

### R4: Tray Overflow Fix Implementation
1. **Extend `ScrapedTrayIcon`:** Include `szTip` field in the structure to store tooltips.
2. **Double-Scrape Architecture:** Modify `ScrapeTrayIcons()` to scrape both visible tray toolbar and `NotifyIconOverflowWindow`'s toolbar.
3. **Dynamic Layout Coordinator:** Implement `UpdateTrayLayout(HWND hTrayNotify)` to compute the dynamic size `W_tray` and move the clock to `x = W_tray`.
4. **Toolbar Subclassing:** Subclass `inst->hToolbar` (and `g_hTrayFlyout` for replacement mode) to forward clicks via `PostMessageW`.
5. **Dynamic Tooltips:** Create a tracking tooltip (`TOOLTIPS_CLASSW`) and display it using `TTM_TRACKACTIVATE` with `szTip` on `WM_MOUSEMOVE`.

### R5: UWP App Icons Fix Implementation
1. **Check for UWP frame:** Compare class name to `"ApplicationFrameWindow"`.
2. **Query Property Store:** Retrieve AUMID using `SHGetPropertyStoreForWindow` and `PKEY_AppUserModel_ID`.
3. **Extract Shell Bitmap:** Call `SHCreateItemFromParsingName` and use `IShellItemImageFactory::GetImage` to extract a 16x16 or 32x32 bitmap.
4. **Convert to Icon:** Convert the 32-bit ARGB bitmap to `HICON` using `CreateIconIndirect`.
5. **Fallback:** Enumerate children to find `Windows.UI.Core.CoreWindow` PID, query the executable path, and call `SHGetFileInfoW`.

### R7: High-DPI Text Blurriness Fix Implementation
1. **Add DPI fields:** Add `UINT dpi;` to `TaskbarInstance`.
2. **Query Monitor DPI:** Use dynamic-loaded `GetDpiForMonitor` from `shcore.dll` with a fallback to `GetDeviceCaps` for Windows 7.
3. **DPI-aware Font Heights:** Multiply font `lfHeight` by `DPI / 96`.
4. **DPI-aware Layout Sizing:** Scale taskbar height, start button size, clock width, and notify area size based on the scaling factor.
5. **Handle `WM_DPICHANGED`:** Capture message, update DPI, call `SetWindowPos`, and redraw.

---

## 5. Verification Method
1. **Build Verification:** Run `build.ps1` to ensure compiling completes successfully.
2. **Check DPI Scaling:** Launch `EliteTaskbar.exe` and drag the taskbar to different monitors. Verify the text remains crisp and does not appear blurry or stretched.
3. **Check Tray Interactions:** In both independent and replacement modes, right-click/left-click/double-click all tray icons. Verify that their menus and applications react normally.
4. **Check UWP Icons:** Launch UWP apps (e.g. Settings, Calculator) and confirm that their correct icons are displayed on the taskbar instead of the default generic `ApplicationFrameHost` icon.
