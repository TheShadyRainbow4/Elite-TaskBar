# Scope: Milestone 6 - Settings UI, About Dialog, & Migration Cleanup

## Architecture
- The settings UI is implemented in `SourceFiles/TaskbarProperties.cpp` and duplicate copies in `Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/TaskbarProperties.cpp`.
- Dialog templates are defined in `SourceFiles/resources.rc` and duplicate copies in `Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/resources.rc`.
- Resource IDs are declared in `SourceFiles/resource.h` and duplicate copies.
- Binary compilation and cleanup is managed by `build.ps1` and related scripts.

## Milestones
| # | Name | Scope | Dependencies | Status |
|---|------|-------|-------------|--------|
| 1 | Fix Start Menu Settings Tab | Resolve the hover-to-reveal/empty controls bug in the Start Menu tab. | None | DONE |
| 2 | Fix About Dialog Layout | Ensure "Less Info <<" and "Okay" or "Close" buttons are properly visible/positioned when expanded. | None | DONE |
| 3 | Fix Explorer Reload Multi-Spawn | Resolve the issue where applying settings opens multiple Win32Explorer windows. | None | DONE |
| 4 | Clean Up Old Executables | Automatically delete `*old*.exe` or `*Old*.exe` files in root and BuildOutput folders after build. | None | DONE |
| 5 | Verify CPL and Settings Parity | Ensure CPL and Settings EXE are perfectly mirrored and functionally identical. | None | DONE |
| 6 | Binary Signing | Digital signing of all compiled binaries using elite-easysigner. | None | DONE |
| 7 | Validate E2E Tests | Run comprehensive E2E tests and ensure everything passes clean. | None | DONE |

## Interface Contracts
- Registry settings: `HKCU\Software\EliteSoftware\Win32Explorer\Advanced`
- Settings notifications: broadcast message or shell reboot triggering registry reload.
- Binary Output: `BuildOutput/` and root workspace folder.
