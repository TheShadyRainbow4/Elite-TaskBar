# Scope: Shell Replacement (Phases XI & XIX)

## Architecture
- EliteTaskbar acts as the custom shell (`Shell_TrayWnd`).
- When running as shell (Replace mode), standard Windows explorer.exe is not active, meaning no Desktop window (Progman) and no standard Start Menu.
- Milestone 1: Progman Desktop Replacement window class, GDI wallpaper drawing, SysListView32 grid child (SHELLDLL_DefView), IShellFolder binding to desktop directories, SHChangeNotifyRegister watcher.
- Milestone 2: Open-Shell Start Menu assimilation, Start Orb click hook natively launching assimilated Start Menu UI.

## Milestones
| # | Name | Scope | Dependencies | Status |
|---|------|-------|-------------|--------|
| 1 | Milestone 1 (Phase XI) | Desktop Window, GDI Wallpaper, SysListView32 Desktop Icon Grid | None | IN_PROGRESS |
| 2 | Milestone 2 (Phase XIX) | Fallback Start Menu, Open-Shell rendering integration | M1 | PLANNED |

## Interface Contracts
- Registry settings for taskbar mode (`TaskbarMode` and `ReplaceExplorerMode`).
- Desktop files change notify (`SHChangeNotifyRegister`) triggers SysListView32 update.
- Start Orb `WM_LBUTTONUP` invokes custom Start Menu when in Replace mode or unconditionally.

## Code Layout
- EliteTaskbar source code: `SourceFiles/`
- Build output: `BuildOutput/` and root `EliteTaskbar.exe`
- Open-Shell source: `Open-Shell-Menu-Source/`
