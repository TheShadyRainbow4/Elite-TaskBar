# Plan: Phase XI (Desktop Replacement) & Phase XIX (Fallback Start Menu)

## Implementation Plan

### Milestone 1: Phase XI (Desktop Replacement)
- [ ] Create/Register `Progman` window class in `EliteTaskbar` (or helper class).
- [ ] Implement `Progman` WindowProc:
  - Read `HKCU\Control Panel\Desktop\Wallpaper` to get the current wallpaper path.
  - Draw/Stretch wallpaper using GDI on `WM_PAINT` / `WM_ERASEBKGND`.
- [ ] Create `SHELLDLL_DefView` child window inside `Progman`.
- [ ] Initialize `SysListView32` child control inside `SHELLDLL_DefView`.
- [ ] Populate desktop icons:
  - Query `IShellFolder` for both user's Desktop directory (`CSIDL_DESKTOPDIRECTORY`) and common Desktop directory (`CSIDL_COMMON_DESKTOPDIRECTORY`).
  - Add items (icons, labels, shortcut target information) to the list view.
- [ ] Monitor folder changes:
  - Register `SHChangeNotifyRegister` to listen for additions, deletions, and updates in Desktop directories.
  - Dynamically refresh list view items when a change is detected.

### Milestone 2: Phase XIX (Fallback Start Menu)
- [ ] Open-Shell source exploration and extraction of Start Menu rendering and ItemList logic.
- [ ] Wire the Start Orb `HWND` click event to:
  - Unconditionally invoke the custom/assimilated Start Menu rendering class natively.
  - If in Replace mode or if native start menu is unavailable/disabled.
- [ ] Integrate configuration UI options (settings tab / dialog).

## Verification & Testing
- Compile using `.\build.ps1`.
- Verify `Progman` desktop window renders wallpaper behind all other windows.
- Verify desktop icons display and are updated dynamically when files are added/removed.
- Verify clicking the Start button invokes theFallback Start Menu.
