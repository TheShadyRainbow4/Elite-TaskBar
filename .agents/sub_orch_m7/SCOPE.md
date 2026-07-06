# Scope: Milestone 7 - Advanced Desktop & Theme Configuration

## Architecture
Milestone 7 involves expanding the desktop replacement module (`DesktopWindow.cpp` / `DesktopWindow.h`) to support rendering on all connected displays, and implementing a fully functional "Desktop Background" configuration tab in the settings dialog (`TaskbarProperties.cpp` / `resources.rc`).

All settings are persisted in the registry root (HKCU or HKLM depending on Portable Mirror Mode) under `Software\EliteSoftware\Win32Explorer\Advanced`.

## Milestones
| # | Name | Scope | Dependencies | Status |
|---|------|-------|-------------|--------|
| 1 | Exploration & Analysis | Swarm of 3 Explorers analyzes codebase, registers, and files. | None | PLANNED |
| 2 | Multi-Display Progman | Implement forceful Progman render on all monitors without closing native taskbar. | M1 | PLANNED |
| 3 | Desktop Background Tab UI | Implement IDD_DESKTOP_PROPS tab with Span/Per-monitor, slideshow (3s+), theme list dropdown, icons previews, and tutorial link. | M2 | PLANNED |
| 4 | Verification & Audit | Swarm verifies compilation, runs E2E tests, signs binaries, and performs security audit. | M3 | PLANNED |

## Interface Contracts
### Registry Keys (`HKCU\Software\EliteSoftware\Win32Explorer\Advanced`)
- `ForceProgmanAllDisplays` (DWORD): Force render custom Desktop (Progman) on all displays.
- `DesktopBackgroundMode` (DWORD): 0 = Span, 1 = Per-monitor.
- `SlideshowEnabled` (DWORD): Enable wallpaper slideshow.
- `SlideshowInterval` (DWORD): Slideshow timing interval in seconds (>= 3).
- `ThemePath` (SZ): Path to native Windows themes.
- `SelectedTheme` (SZ): Selected theme file name.

### Dialog Templates (`resources.rc` / `resource.h`)
- Dialog ID: `IDD_DESKTOP_PROPS`
- Control IDs:
  - `IDC_FORCE_PROGMAN_ALL` (Checkbox)
  - `IDC_BG_MODE_SPAN` (Radio)
  - `IDC_BG_MODE_PERMON` (Radio)
  - `IDC_THEME_PATH` (Edit)
  - `IDC_THEME_BROWSE` (Button)
  - `IDC_THEME_DROPDOWN` (ComboBox)
  - `IDC_SLIDESHOW_ENABLE` (Checkbox)
  - `IDC_SLIDESHOW_INTERVAL` (Edit/ComboBox)
  - `IDC_THEME_TUTORIAL` (Link/Button)
  - `IDC_WALLPAPER_PREVIEW` (Static custom draw)
  - `IDC_ICON_PREVIEW_1` (Static icon draw)
  - `IDC_ICON_PREVIEW_2` (Static icon draw)
  - `IDC_ICON_PREVIEW_3` (Static icon draw)

## Code Layout
- `SourceFiles/DesktopWindow.cpp`: Progman creation & wallpaper rendering loop.
- `SourceFiles/TaskbarProperties.cpp`: UI dialog procedure and registry handling.
- `SourceFiles/resources.rc`: Dialog layout templates.
- `SourceFiles/resource.h`: Resource IDs.
- Submodule copy in `Remaining_Shell/Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/`: Must mirror all changes.
