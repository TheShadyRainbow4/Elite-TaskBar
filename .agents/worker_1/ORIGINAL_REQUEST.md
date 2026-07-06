## 2026-07-05T19:12:17Z
You are the Worker agent for Milestone 7 of the Elite-Taskbar project.
Your working directory is C:\Users\Administrator\Desktop\Elite-TaskBar\.agents\worker_1.
Your role: teamwork_preview_worker.

Your objective is to implement the following features in accordance with the design recommendations from the Explorers' handoff reports (located at C:\Users\Administrator\Desktop\Elite-TaskBar\.agents\explorer_1\handoff.md, C:\Users\Administrator\Desktop\Elite-TaskBar\.agents\explorer_2\handoff.md, and C:\Users\Administrator\Desktop\Elite-TaskBar\.agents\explorer_3\handoff.md):

1. **Pre-build Synchronisation in `build.ps1`**:
   Add a pre-build copy step in `build.ps1` that automatically copies:
   - `SourceFiles/resource.h` -> `Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/resource.h`
   - `SourceFiles/resources.rc` -> `Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/resources.rc`
   - `SourceFiles/TaskbarProperties.cpp` -> `Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/TaskbarProperties.cpp`
   before compiling. Ensure this is placed before the MSVC environment setup/build triggers.

2. **Resource Definitions**:
   Modify `SourceFiles/resource.h` (which will be synced to the submodule) to define the new control IDs:
   - `IDC_DESKTOP_FORCE_PROGMAN_ALL` = 320
   - `IDC_DESKTOP_MODE_SPAN` = 321
   - `IDC_DESKTOP_MODE_PERMONITOR` = 322
   - `IDC_DESKTOP_THEME_PATH` = 323
   - `IDC_DESKTOP_THEME_BROWSE` = 324
   - `IDC_DESKTOP_THEME_SELECT` = 325
   - `IDC_DESKTOP_SLIDESHOW_ENABLED` = 326
   - `IDC_DESKTOP_SLIDESHOW_INTERVAL` = 327
   - `IDC_DESKTOP_THEME_TUTORIAL_LINK` = 328
   - `IDC_DESKTOP_WALLPAPER_PREVIEW` = 329
   - `IDC_DESKTOP_ICON_PREVIEW_1` = 330
   - `IDC_DESKTOP_ICON_PREVIEW_2` = 331
   - `IDC_DESKTOP_ICON_PREVIEW_3` = 332
   - `IDC_DESKTOP_ICON_PREVIEW_4` = 333

3. **Dialog Templates (`SourceFiles/resources.rc`)**:
   Modify the `IDD_DESKTOP_PROPS` dialog template in `SourceFiles/resources.rc` to include all the new controls using Segoe UI Semibold (600, weight). Reference the layout in `explorer_2\handoff.md` and `explorer_3\handoff.md`. Include Groupboxes, Checkboxes, ComboBoxes, Edit controls, SysLink tutorial link, wallpaper preview, and icon previews.

4. **Settings Dialog Logic (`SourceFiles/TaskbarProperties.cpp`)**:
   Modify `DesktopSettingsDlgProc` to:
   - In `WM_INITDIALOG`:
     - Bind witty/sarcastic tooltips to the new controls.
     - Load values from registry `Software\EliteSoftware\Win32Explorer\Advanced` using `GetEliteRegistryRoot()`:
       - `ForceProgmanAllDisplays` (DWORD, default 0)
       - `DesktopWallpaperMode` (DWORD, default 0 = Span, 1 = Per-monitor)
       - `DesktopThemePath` (SZ)
       - `DesktopSlideshowEnabled` (DWORD, default 0)
       - `DesktopSlideshowInterval` (DWORD, default 300 seconds)
     - Set initial control states.
     - Detect all `.theme` files in `%SystemDrive%\Windows\Resources\Themes` and `%LocalAppData%\Microsoft\Windows\Themes` and populate `IDC_DESKTOP_THEME_SELECT` dropdown.
     - Load/render previews of the first few theme icons (using static controls `IDC_DESKTOP_ICON_PREVIEW_1` to `IDC_DESKTOP_ICON_PREVIEW_4`).
   - In `WM_COMMAND`:
     - Handle `BN_CLICKED`, `CBN_SELCHANGE`, `EN_CHANGE` to mark properties changed.
     - Handle click on `IDC_DESKTOP_THEME_BROWSE` to browse for directory path using Shell API (`SHBrowseForFolder` or similar).
     - Handle theme selection change to load new theme settings.
   - In `WM_NOTIFY`:
     - Intercept `PSN_APPLY` and save the new setting values to registry.
     - Intercept SysLink `IDC_DESKTOP_THEME_TUTORIAL_LINK` click to call `ShellExecuteW` opening the theme tutorial link.
   - In `WM_DRAWITEM` / custom draw:
     - Handle custom drawing for wallpaper preview (`IDC_DESKTOP_WALLPAPER_PREVIEW`) to render a scaled down wallpaper thumbnail.

5. **Desktop Replacement Logic (`SourceFiles/DesktopWindow.cpp` / `DesktopWindow.h`)**:
   - In `DesktopWindow::Initialize()`, read registry toggle `ForceProgmanAllDisplays` and load it. If `ForceProgmanAllDisplays` is 1, initialize desktop window even if the operational mode is Independent (leaving native taskbar open).
   - In `ProgmanWndProc`, handle `WM_DISPLAYCHANGE` to dynamically resize custom `Progman` window using `GetSystemMetrics` for `SM_*VIRTUALSCREEN`.
   - In `DrawWallpaper`, implement Span vs Per-Monitor modes.
     - Span mode: stretches wallpaper across the virtual screen bounds (`scrW, scrH`).
     - Per-monitor mode: use `EnumDisplayMonitors` (or coordinate loop) to partition the DC and render the wallpaper scaled onto each monitor's bounds individually.
   - Implement wallpaper slideshow: if slideshow is enabled, use a Win32 timer (`SetTimer` with configured interval >= 3 seconds) on the desktop replacement window. Upon `WM_TIMER`, swap wallpaper path to the next image file in the configured directory, update cached image, and invalidate window bounds to trigger redrawing.

6. **Post-Flight Changelog**:
   Immediately update `CHANGELOG.md` at the project root to document all added functions, files modified, and rationale.

7. **Compilation & Signing**:
   - Compile using `.\build.ps1` (with `$env:ELITE_AUDITOR_RUN = "1"`). Make sure both x64 and x86 targets, Settings EXE, and CPL are built cleanly without errors.
   - Verify that all resulting binaries are signed using `elite-easysigner` (which is called by `build.ps1` via `build_sign.ps1`).

Deliver a handoff report at `C:\Users\Administrator\Desktop\Elite-TaskBar\.agents\worker_1\handoff.md` detailing implementation, compilation, and test outcomes.

Scope boundaries:
- Do not remove any existing features.
- Do not use flat design or hardcoded generic colors.
- Do not run build/test commands in external windows; run them directly in the powershell command environment.
