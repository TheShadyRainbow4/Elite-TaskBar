# Plan: Phase XI (Desktop Replacement) & Phase XIX (Fallback Start Menu)

## Milestone 1: Phase XI (Desktop Replacement)
- [x] Research Progman, Wallpaper, and Desktop Grid child (Explorers 1, 2, 3 completed)
- [ ] UI Resource Definition: define `IDC_DESKTOP_REPLACE_ENABLED`, etc. in `resource.h` and add `IDD_DESKTOP_PROPS` to `resources.rc`.
- [ ] Property Sheet Integration: implement `DesktopSettingsDlgProc` in `TaskbarProperties.cpp` and insert the "Desktop" page into `ShowTaskbarProperties`.
- [ ] DesktopWindow Core Development:
  - Register `Progman` class, handle `WM_MOUSEACTIVATE` and `WM_WINDOWPOSCHANGING` Z-order defense.
  - Integrate native desktop hide/restore logic for coexistence.
  - Implement GDI+ wallpaper renderer supporting Stretch, Center, Tile, Fit, Fill.
  - Create child `SHELLDLL_DefView` and grandchild `SysListView32` controls.
  - Bind to desktop folders using `IShellFolder` and system image lists.
  - Implement double-click execute and inline renaming.
  - Set up `SHChangeNotifyRegister` folder watcher with a 100ms debounce timer.
- [ ] Main Entry Point Integration: hook `DesktopWindow::Initialize` and `DesktopWindow::Cleanup` into `main.cpp` / `TaskbarWindow.cpp` lifecycle.
- [ ] Build Configuration Update: add `DesktopWindow.cpp` to `build_x64.ps1` and `build_x86.ps1`.

## Milestone 2: Phase XIX (Fallback Start Menu)
- [ ] Start Menu UI Toggle: Add `IDC_FALLBACK_STARTMENU_ENABLED` to Start Menu settings tab.
- [ ] Invocation Wiring: Edit `StartButton.cpp` to conditionally invoke Open-Shell menu executable (`StartMenu.exe`) if enabled in Replace mode.
- [ ] Portable Fallback: support relative `.\StartMenu_PE` paths for WinPE environments.

## Verification & Testing
- [ ] Compile using `build.ps1` with `$env:ELITE_AUDITOR_RUN = "1"`.
- [ ] Verify `Progman` class is registered, occupies virtual screen size, and is positioned at `HWND_BOTTOM`.
- [ ] Verify wallpaper drawing handles different layout styles correctly (Center, Stretch, Tile, Fit, Fill).
- [ ] Verify desktop folders are aggregated and items display correct system icons.
- [ ] Verify adding/removing/renaming a file on the desktop updates the listview dynamically.
- [ ] Verify clicking the Start Orb invokes Open-Shell fallback menu.
- [ ] Verify toggling settings disables/enables custom desktop and start menu.
- [ ] Confirm no memory leaks or crashes are logged in `EliteTaskbar.log`.
