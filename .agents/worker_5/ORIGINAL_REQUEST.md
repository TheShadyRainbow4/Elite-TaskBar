## 2026-07-05T09:32:55Z
You are the Worker subagent (Worker 5) responsible for remediating the Milestone 3 implementation issues identified by the Reviewers.

Please implement the following fixes in both directories (SourceFiles/ and Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/) to maintain perfect sync:

1. GetWindowIconFix HICON Resource Leak:
   - In TaskbarWindow.cpp, under HSHELL_WINDOWDESTROYED, call DestroyIcon(it->hIcon) before erasing the button from the collection.
   - Under HSHELL_REDRAW, call DestroyIcon(btn.hIcon) before reassigning btn.hIcon = GetWindowIconFix(hwndShell).
2. Typography Font Definitions:
   - In resources.rc dialog templates, update the font weight or face to match the Segoe UI Semibold guidelines (e.g. FONT 8, "Segoe UI Semibold" or similar, or dynamic application using FW_SEMIBOLD).
   - In TaskbarProperties.cpp, change CreateFontW's weight parameter from FW_BOLD (700) to FW_SEMIBOLD (600) for the banners.
3. Missing Hover Tooltips:
   - In TaskbarProperties.cpp, add AddDlgTooltip entries for IDC_WIDTH_AUTO, IDC_WIDTH_FIXED, IDC_WIDTH_ICONS, IDC_IMPORT_SETTINGS, and IDC_EXPORT_SETTINGS using witty/sarcastic descriptions.
   - Register hover tooltips for standard property sheet buttons (OK, Cancel, Apply).
4. About and Help Dialog Icons:
   - In TaskbarProperties.cpp, configure the About dialog banner to load the native blue circle information icon (IDI_INFORMATION from system resources via LoadIconW(NULL, IDI_INFORMATION)) instead of the preferences gear icon.
   - In WM_INITDIALOG for both dialog procedures (AboutDlgProc and HelpDlgProc), set the window titlebar icon using SendMessageW with WM_SETICON and the correct native icon handle (IDI_INFORMATION and IDI_QUESTION respectively).
5. About Dialog Layout Spacing:
   - In resources.rc, reduce the 60 dialog unit empty space gap in the expanded state of IDD_ABOUT_DIALOG to make the layout look clean and compact.
6. Resource ID Collision:
   - In resource.h in both folders, change IDC_ORB_SELECTOR to 232 and IDC_START_MONITOR_LIST to 233 to resolve collisions.
7. Replace Mode System Tray DPI Scaling:
   - In TaskbarWindow.cpp, ensure that the system tray width W_tray in Replace mode is scaled dynamically using inst->dpi (similar to independent mode).
8. Compilation & Locking Fix:
   - Run build.ps1 to verify the compilation and signing stage completes successfully without lock errors. Terminate rundll32.exe processes running the CPL before copying the binaries if needed.

## 2026-07-05T09:34:46Z
Additional remediation requirements from parent:
1. GDI Resource Leak Fix (TaskbarWindow.cpp):
   - In SyncTaskbarButtonsAcrossMonitors(), only fetch a new icon using GetWindowIconFix if the button is NOT already present in g_TaskButtons. If it is present, reuse the existing tb.hIcon.
2. Settings Apply Restart Path Fix (TaskbarProperties.cpp):
   - In BroadcastSettingsChangeThread(), check __argc and __wargv. If __argc > 1 and __wargv[1] is a valid folder, use it as the directory path for launching EliteTaskbar.exe and Win32Explorer.exe. Otherwise, fall back to the current directory of the module.


