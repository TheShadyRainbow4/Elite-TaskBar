## 2026-07-05T14:41:44Z

Objective: Implement Phase XI (Desktop Window, GDI wallpaper rendering, and SysListView32 desktop grid) and Phase XIX (Open-Shell fallback Start Menu integration).

Input Information & Technical Specifications:
- Technical Design & Synthesis: Read C:\Users\Administrator\Desktop\Elite-TaskBar\.agents\orchestrator_shell_replacement\technical_design.md
- Plan: Read C:\Users\Administrator\Desktop\Elite-TaskBar\agents\orchestrator_shell_replacement\plan.md
- Progress: Read C:\Users\Administrator\Desktop\Elite-TaskBar\.agents\orchestrator_shell_replacement\progress.md
- Explorer Handoffs:
  - C:\Users\Administrator\Desktop\Elite-TaskBar\.agents\explorer_m1_1\handoff.md (Progman Window lifecycle)
  - C:\Users\Administrator\Desktop\Elite-TaskBar\.agents\explorer_m1_2\handoff.md (Wallpaper GDI rendering and calculations)
  - C:\Users\Administrator\Desktop\Elite-TaskBar\.agents\explorer_m1_3\handoff.md (Desktop Icon Grid & IShellFolder Namespace binding)

Key Action Items:
1. Define UI Resource Toggles (GEMINI.md Rule 1):
   - Define new registry keys `DesktopReplacementEnabled` and `FallbackStartMenuEnabled` (defaulting to 1).
   - In `SourceFiles/resource.h` and `SourceFiles/resources.rc`, add `IDD_DESKTOP_PROPS` settings template with checkboxes: `IDC_DESKTOP_REPLACE_ENABLED`, `IDC_DESKTOP_WALLPAPER_ENABLED`, `IDC_DESKTOP_ICONS_ENABLED`.
   - In `SourceFiles/TaskbarProperties.cpp`, implement dialog procedures (`DesktopSettingsDlgProc`, `StartMenuSettingsDlgProc`) and include pages `IDD_DESKTOP_PROPS` and `IDD_STARTMENU_PROPS` in the properties tabs.
2. Develop `DesktopWindow` Module:
   - Create `SourceFiles/DesktopWindow.h` and `SourceFiles/DesktopWindow.cpp`.
   - Implement `Progman` class registration and creation covering size of virtual screen, Z-order of `HWND_BOTTOM`, and mouse action/position filters.
   - Implement hiding of native `Progman` desktop window on initialization and restoring it on cleanup.
   - Implement GDI+ wallpaper rendering on background painting messages (`WM_PAINT` / `WM_ERASEBKGND`), parsing registry `HKCU\Control Panel\Desktop\Wallpaper` and style keys. Support Centered, Stretched, Tiled, Fit, and Fill styles using halftone scaling quality.
   - Initialize child window `"SHELLDLL_DefView"` and listview control `"SysListView32"` with standard explorer listview styles.
   - Enumerate shell directories using `IShellFolder` and `IEnumIDList` to populate desktop icons. Bind system image lists.
   - Implement `SHChangeNotifyRegister` folder watcher with a 100ms debounced timer to automatically repopulate.
   - Wire double-click executing (`NM_DBLCLK` using `ShellExecuteExW`) and label renaming (`LVN_ENDLABELEDITW` using `IShellFolder::SetNameOf`).
3. Integrate Main Lifecycle:
   - Call `DesktopWindow::Initialize` in `TaskbarWindow::Initialize` and `DesktopWindow::Cleanup` in `TaskbarWindow::Cleanup` conditional on settings values.
4. Integrate Start Menu Fallback:
   - Edit `StartButton.cpp` to call Open-Shell menu executable (`StartMenu.exe`) if `FallbackStartMenuEnabled` is active and in Replace mode, falling back to relative paths or native menu simulation if needed.
5. Build Verification:
   - Register `DesktopWindow.cpp` in `build_x64.ps1` and `build_x86.ps1`.
   - Compile both x64 and x86 targets using `build.ps1` with environment variable `$env:ELITE_AUDITOR_RUN = "1"` set to 1. Ensure the build is clean and auto-commits.
6. Documentation & Logging:
   - Update `CHANGELOG.md` at root with details of the changes.
   - Update `README.md` at root to document the new shell features.
   - Ensure errors are logged to `%SystemDrive%\EliteSoftware\Logs\EliteTaskbar.log`.
