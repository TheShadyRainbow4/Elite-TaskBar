## 2026-07-05T23:34:13Z

You are teamwork_preview_worker_m5.
Your working directory is C:\\Users\\Administrator\\Desktop\\Elite-TaskBar\\.agents\\worker_m5.

Your task is to implement the Milestone 5 features:
1. Start Menu & Flyout Spawning Fix (Display Spoofing):
   - Implement dynamic primary display spoofing to trick flyouts (Start Menu, Volume mixer) into rendering on the correct display when clicked on a secondary taskbar.
   - Declare global flags `g_IsSpoofingNativeTaskbar` (bool) and `g_SpoofStartTime` (DWORD) in `SourceFiles/TaskbarWindow.cpp`.
   - Add a helper function `StartNativeTaskbarSpoof(HWND hClickedTaskbar)` to teleport `g_hNativeTaskbar` to the clicked taskbar coordinate and show it.
   - Update the 100ms timer (Timer ID 9999) in `WindowProc` to avoid resetting the spoofed taskbar while `g_IsSpoofingNativeTaskbar` is true (with a 2-second safety timeout).
   - In `SourceFiles/StartButton.cpp` under `WM_LBUTTONUP`, call `StartNativeTaskbarSpoof` before sending clicks to the native target on secondary screens.
   - In `TrayToolbarSubclassProc` in `SourceFiles/TaskbarWindow.cpp`, subclass the toolbar with `inst` as refData. Trigger `StartNativeTaskbarSpoof` before sending uCallbackMessage on secondary screens.

2. Clock Gap Fix:
   - Adjust `W_clock` in `SourceFiles/TaskbarWindow.cpp` to be `MulDiv(85, dpi, 96)` instead of 140 base px.
   - In `TrayClockProc`'s `WM_PAINT` handler, change text alignment parameters to `DT_CENTER | DT_VCENTER` (or `DT_CENTER | DT_VCENTER | DT_NOCLIP`) and center the text (adjust or remove `rcClient.right -= 15;` offset).

3. Tray Icons Fixes:
   - In `SourceFiles/TrayIconScraper.cpp`, implement dynamic layout probing for Windows 10 (offset 24) vs Windows 11 (offset 16) `TRAYDATA` structures using `GetIconInfo`. If `hIcon` is NULL, fallback to window icon retrieval using `GetWindowIconFix(icon.hwnd)`, sending `WM_GETICON`, or `GetClassLongPtrW(icon.hwnd, GCLP_HICONSM)`.
   - Subclass `hSysPager` using `SysPagerSubclassProc` in `SourceFiles/TaskbarWindow.cpp` to handle `WM_ERASEBKGND` / `WM_PAINT` / `WM_PRINTCLIENT` and call `DrawThemeParentBackground` to remove the white background above/behind tray items.

4. Two-Row Tray Option:
   - Define `IDC_TWO_ROW_TRAY` as `294` in `SourceFiles/resource.h`.
   - Add the checkbox control in `SourceFiles/resources.rc` in `IDD_TASKBAR_PROPS` tab layout. Shift GroupBox "Custom Icon Theme Folder" and its child controls down by 15 units to fit.
   - Add `bool EnableTwoRowTray` in `EliteTaskbarConfig` in `SourceFiles/Config.h`.
   - Read/write the checkbox state in `QueryOperationalMode` in `SourceFiles/main.cpp` and `TaskbarSettingsDlgProc` in `SourceFiles/TaskbarProperties.cpp`, writing to registry key `EnableTwoRowTray` (default to 1). Add tooltip for the checkbox in properties dialog init.
   - In `SourceFiles/TaskbarWindow.cpp` layout logic, adjust width (`W_tray`) and toolbar vertical position if `EnableTwoRowTray` is true. Create image list with `12x12` instead of `16x16` and apply `TBSTYLE_WRAPABLE` flag to toolbar window style.
   - In `TrayNotifyProc` for `Replace` mode, draw tray icons in two rows (12x12 icons, 18px horizontal step, 14px vertical step) and implement matching two-row hit-testing for mouse messages.

5. Tray Actions:
   - Define `WM_TRAY_CALLBACK_WIN32EXPLORER` (WM_USER+500), `WM_TRAY_CALLBACK_TASKBAR` (WM_USER+501), `WM_TRAY_CALLBACK_DESKTOP` (WM_USER+502).
   - Register the 3 custom tray icons (Win32Explorer, Taskbar, Desktop Replacement) in `TaskbarWindow::Initialize` on the primary taskbar. Unregister them in `TaskbarWindow::Cleanup`.
   - In `WindowProc`, implement the tray callbacks:
     - Win32Explorer Tray (uses IDI_MAIN_PROGRAM): Single-click = ShowAboutDialog. Double-click = open new Win32Explorer window (`ShellExecuteW`).
     - Taskbar Tray (uses IDI_PREFERENCES): Single-click = ShowAboutDialog. Double-click = ShowTaskbarProperties.
     - Desktop Replacement Tray (uses native desktop tree icon extracted from shell32.dll index 34): Double-click = find `Progman` -> `SHELLDLL_DefView` -> `SysListView32` and toggle visibility, and update registry value `HideIcons`. Context menu = Toggle desktop replacement (toggles `DesktopReplacementEnabled` and runs `DesktopWindow::Initialize()` / `DesktopWindow::Cleanup()`), Restart Taskbar (powershell script execution), Restart Explorer.

Verify your work by running:
`$env:ELITE_AUDITOR_RUN="1"`
`.\build.ps1`
Check that everything compiles cleanly and signs the compiled binaries automatically using the easysigner.
Write your implemented changes and build results to C:\\Users\\Administrator\\Desktop\\Elite-TaskBar\\.agents\\worker_m5\\handoff.md.

MANDATORY INTEGRITY WARNING:
DO NOT CHEAT. All implementations must be genuine. DO NOT hardcode test results, create dummy/facade implementations, or circumvent the intended task. A Forensic Auditor will independently verify your work. Integrity violations WILL be detected and your work WILL be rejected.

## 2026-07-05T23:39:57Z

<USER_REQUEST>
Write the E2E test suite summary file `TEST_READY.md` in the project root (`C:\Users\Administrator\Desktop\Elite-TaskBar\TEST_READY.md`).
The file contents must follow this exact format:

# E2E Test Suite Ready

## Test Runner
- Command: `powershell -ExecutionPolicy Bypass -File Subagent_Tests\run_comprehensive_e2e.ps1`
- Expected: all 108 tests pass with exit code 0

## Coverage Summary
| Tier | Count | Description |
|------|------:|-------------|
| 1. Feature Coverage | 50 | 5 cases per feature for 10 features |
| 2. Boundary & Corner | 50 | 5 cases per feature for 10 features |
| 3. Cross-Feature | 5 | Pairwise combination of major feature interactions |
| 4. Real-World Application | 3 | Full replacement, multi-monitor workspace, and shell upgrade scenarios |
| **Total** | **108** | |

## Feature Checklist
| Feature | Tier 1 | Tier 2 | Tier 3 | Tier 4 |
|---------|:------:|:------:|:------:|:------:|
| 1. Multi-Monitor Flyouts | 5 | 5 | ✓ | ✓ |
| 2. Clock/Tray Gap & 2-Row Tray | 5 | 5 | ✓ | ✓ |
| 3. Tray Icon Backgrounds & Fallbacks | 5 | 5 | ✓ | ✓ |
| 4. Settings UI & About Dialog | 5 | 5 | ✓ | ✓ |
| 5. Clean Up old.exe & Reload Bug | 5 | 5 | ✓ | ✓ |
| 6. Progman Multi-Display & Desktop Tab | 5 | 5 | ✓ | ✓ |
| 7. Keyboard Hooks (Win+R) | 5 | 5 | ✓ | ✓ |
| 8. Tray Click Actions | 5 | 5 | ✓ | ✓ |
| 9. Taskbar Extras (Seconds, QuickLaunch) | 5 | 5 | ✓ | ✓ |
| 10. Win32Explorer view modes | 5 | 5 | ✓ | ✓ |

Write this file, make sure it is saved successfully, and report completion.
</USER_REQUEST>
