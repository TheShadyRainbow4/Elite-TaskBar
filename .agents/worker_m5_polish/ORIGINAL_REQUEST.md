## 2026-07-06T00:05:57Z

You are teamwork_preview_worker_m5_polish.
Your working directory is C:\\Users\\Administrator\\Desktop\\Elite-TaskBar\\.agents\\worker_m5_polish.

Your task is to implement the following polish, layout, and robustness fixes:

1. GDI HICON Leak Fix:
   - In `SourceFiles/TrayIconScraper.h`, add a boolean field `bool bOwnsIcon = false;` to the `ScrapedTrayIcon` struct.
   - In `SourceFiles/TrayIconScraper.cpp`:
     - Change the signature of the static helper `GetWindowIconFix` to:
       `static HICON GetWindowIconFix(HWND hwnd, bool& bOutOwnsIcon)`
       Inside `GetWindowIconFix`, if it falls back to calling `GetProcessIcon(hwnd)`, set `bOutOwnsIcon = true;`. Otherwise, set `bOutOwnsIcon = false;`.
     - In `ScrapeTrayIconsFromToolbar`:
       - When retrieving the icon:
         ```cpp
         bool bOwns = false;
         icon.hIcon = GetWindowIconFix(icon.hwnd, bOwns);
         icon.bOwnsIcon = bOwns;
         ```
       - If `icon.hIcon` is NULL after checking both `hIcon10`/`hIcon11` and `GetWindowIconFix`, fallback to a shared default system icon (e.g. `LoadIconW(NULL, IDI_APPLICATION)` which is index 32512), and set `icon.bOwnsIcon = false;` (since shared system icons are not owned and do not require DestroyIcon).
     - In `UpdateTrayToolbar`:
       - If returning early because `changed` is false, free any locally owned icon handles:
         ```cpp
         for (const auto& icon : icons) {
             if (icon.hIcon && icon.bOwnsIcon) {
                 DestroyIcon(icon.hIcon);
             }
         }
         ```
       - Before updating the global `g_CurrentTrayIcons` vector, free any owned icon handles currently in `g_CurrentTrayIcons`:
         ```cpp
         for (const auto& icon : g_CurrentTrayIcons) {
             if (icon.hIcon && icon.bOwnsIcon) {
                 DestroyIcon(icon.hIcon);
             }
         }
         g_CurrentTrayIcons = icons;
         ```
   - In `SourceFiles/TaskbarWindow.cpp`, inside `TrayNotifyProc` under `WM_COPYDATA` (where it receives `NIM_ADD` or `NIM_MODIFY` messages):
     - In the `NIM_MODIFY` block, before overwriting `icon.hIcon` with `CopyIcon(...)`, make sure to call `DestroyIcon(icon.hIcon);` if `icon.hIcon` is non-NULL.

2. Primary Monitor Display Spoofing:
   - In `SourceFiles/TaskbarWindow.cpp`, inside `TrayToolbarSubclassProc` (around line 130), remove the monitor check `inst && inst->hMonitor != MonitorFromWindow(..., MONITOR_DEFAULTTOPRIMARY)` so that `StartNativeTaskbarSpoof` is called unconditionally on all clicks.
   - In `SourceFiles/StartButton.cpp`, inside `WM_LBUTTONUP` (around line 338), remove the `if (isSecondary)` check so that `StartNativeTaskbarSpoof` is called unconditionally when the Start Button is clicked.

3. Toolbar Hover State:
   - In `SourceFiles/TaskbarWindow.cpp` inside `Initialize`, call `SetWindowTheme(inst->hSysPager, L"", L"")` to strip native visual styles from `hSysPager` (making it transparent), but DO NOT call `SetWindowTheme(inst->hToolbar, L"", L"")` on `hToolbar` (to preserve the glossy hover state of the toolbar buttons).

4. Active Items Alignment:
   - In `SourceFiles/TaskbarWindow.cpp` inside `UpdateTaskbarLayout`:
     - Query the button height of `inst->hTaskSwitch` dynamically using `TB_GETBUTTONSIZE`.
     - Center the task switch control vertically on the taskbar:
       ```cpp
       int switchHeight = taskbarHeight;
       int switchY = 0;
       if (inst->hTaskSwitch) {
           DWORD dwBtnSize = (DWORD)SendMessageW(inst->hTaskSwitch, TB_GETBUTTONSIZE, 0, 0);
           int btnHeight = HIWORD(dwBtnSize);
           if (btnHeight > 0 && btnHeight < taskbarHeight) {
               switchHeight = btnHeight;
               switchY = (taskbarHeight - btnHeight) / 2;
           }
       }
       SetWindowPos(inst->hTaskSwitch, NULL, xTaskSwitch, switchY, widthTaskSwitch, switchHeight, SWP_NOZORDER | SWP_NOACTIVATE);
       ```

5. Clock Alignment:
   - In `SourceFiles/TaskbarWindow.cpp` inside `TrayClockProc`'s `WM_PAINT` handler:
     - Center the multi-line clock text vertically in its client bounds by querying text size via `DrawText` with `DT_CALCRECT` and offsetting `rcClient.top` and `rcClient.bottom` before drawing the text. Do this for both the `DrawThemeTextEx` path and the fallback `DrawTextW` path.
     - For example:
       ```cpp
       RECT rcCalc = {0};
       DrawTextW(hdcBuffer, clockText, -1, &rcCalc, DT_CENTER | DT_CALCRECT);
       int textHeight = rcCalc.bottom - rcCalc.top;
       int clientHeight = rcClient.bottom - rcClient.top;
       if (textHeight < clientHeight) {
           int yOffset = (clientHeight - textHeight) / 2;
           rcClient.top += yOffset;
           rcClient.bottom = rcClient.top + textHeight;
       }
       ```

6. Build & Test Fixes:
   - In `build.ps1`, wrap the `EliteStartMenu.ps1` Invoke-ps2exe compilation blocks inside a `if (Test-Path EliteStartMenu.ps1)` check.
   - In `Subagent_Tests/run_comprehensive_e2e.ps1`, fix the E2E verification variables at lines 399-403 to check `$hwndTrayNotify` and `$hwndTrayClock` (instead of `$hwndNotify` and `$hwndClock`).

Build and Validate:
- Run `build.ps1` with `$env:ELITE_AUDITOR_RUN = "1"` to compile and sign all binaries cleanly.
- Run `Subagent_Tests\run_comprehensive_e2e.ps1` to ensure all 108 test cases pass cleanly.
- Write detailed findings and change list to `C:\\Users\\Administrator\\Desktop\\Elite-TaskBar\\.agents\\worker_m5_polish\\handoff.md`.
