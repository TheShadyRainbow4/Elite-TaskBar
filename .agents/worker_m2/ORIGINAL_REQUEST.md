## 2026-07-04T21:01:18Z

You are Worker 3 (type: teamwork_preview_worker).
Your working directory is C:\Users\Administrator\Desktop\Elite-TaskBar\.agents\worker_m2.
Your parent is the Project Orchestrator (conversation ID: f2f647cc-0a56-4fa6-935c-de6b9def612a).

Task:
Implement the changes for Milestone 2 (R2: System Tray Integration & R5: Custom Icon Theming) as detailed in Explorer 2's report at C:\Users\Administrator\Desktop\Elite-TaskBar\.agents\explorer_r2_r5\handoff.md:

1. R2: System Tray Integration:
- For EliteTaskbar: Add a system tray icon inside TaskbarWindow.cpp. Use the primary taskbar window handle (g_Taskbars[0]->hTaskbar) and resource icon IDI_MAIN_PROGRAM. Handle WM_TRAYICON (WM_USER + 200) and show a right-click context menu containing "Elite Taskbar Settings" and "Quit EliteTaskbar". Clicking "Quit" must send WM_CLOSE to terminate the process cleanly. Handle Shell_NotifyIconW(NIM_DELETE) on exit in TaskbarWindow::Cleanup(). Witty tooltip: L"EliteTaskbar - Keeping your windows in line since Windows Vista went out of fashion."
- For Win32Explorer: Expose HWND GetHWND() const { return m_hwnd.get(); } in EventWindow.h. In App.cpp, register the tray icon using the EventWindow handle and resource icon IDI_MAIN_PROGRAM. Handle RBUTTONUP message on the tray icon to show a context menu with "Open New Window" and "Quit Win32Explorer". Witty tooltip: L"Win32Explorer - Because modern WinUI is just too slow for your heavy lifting." Ensure Shell_NotifyIconW(NIM_DELETE) is called in App's destructor.

2. R5: Custom Icon Theming:
- In both TaskbarProperties.cpp / resources.rc and EliteSettings.ps1, replace the "Taskbar Appearance" groupbox with "Custom Icon Theme Folder" UI page. The UI must include a theme folder path textbox (IDC_THEME_FOLDER_PATH), a Browse button (IDC_THEME_FOLDER_BROWSE) opening a folder picker dialog, and an "Enable Dark Mode" checkbox (IDC_ENABLE_DARK_MODE). Keep the "Enable Dark Mode" checkbox permanently disabled/non-functional.
- In Win32ResourceLoader.cpp (LoadBitmapFromPNGForDpi / LoadGdiplusBitmapFromPNGAndScale), check the registry key (or XML config) for CustomThemePath. If set, check if a custom PNG/ICO file matching the Icon name exists in that directory (e.g. "Back.png" / "Back.ico"). If it does, load it using GDI+ File APIs, scaling it accordingly, with a fallback to the native resources if not found or corrupted.

Development Rules:
- The entire build process must be run via "C:\Users\Administrator\Desktop\Elite-TaskBar\build.ps1".
- Follow the EliteSoftwareTech Co. WinForms and C++ GUI guidelines (Segoe UI Semibold, Visual Styles, no dark mode, WITTY tooltips, etc.).
- Update CHANGELOG.md immediately after editing any file!

MANDATORY INTEGRITY WARNING:
> DO NOT CHEAT. All implementations must be genuine. DO NOT
> hardcode test results, create dummy/facade implementations, or
> circumvent the intended task. A Forensic Auditor will independently
> verify your work. Integrity violations WILL be detected and your
> work WILL be rejected.

Once completed, run the build and verify the changes compile and run correctly, then write a detailed handoff.md report and notify me.
