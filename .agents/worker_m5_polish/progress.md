# Progress - worker_m5_polish

Last visited: 2026-07-05T17:10:00-07:00

## Tasks
- [ ] Initialize implementation of polish and feedback fixes
- [ ] Implement Fix 1 (GDI Leak): Fix GDI HICON handle leak in `GetProcessIcon` and `ScrapeTrayIconsFromToolbar` / `UpdateTrayToolbar` in `TrayIconScraper.cpp`
- [ ] Implement Fix 2 (Primary Spoofing): Enable display spoofing on the primary monitor by removing the monitor index checks in `TaskbarWindow.cpp` and `StartButton.cpp`
- [ ] Implement Fix 3 (Toolbar Hover State): Do not call `SetWindowTheme` to strip visual styles from `hToolbar` in `TaskbarWindow.cpp` (keep glossy hover state)
- [ ] Implement Fix 4 (Active Items Alignment): Center the `hTaskSwitch` window vertically on the taskbar based on its button height in `UpdateTaskbarLayout`
- [ ] Implement Fix 5 (Clock Alignment): Center the multi-line clock text vertically in `TrayClockProc` by calculating text height via `DrawText` with `DT_CALCRECT` and offsetting the drawing rectangle
- [ ] Implement Fix 6 (Build & Test Fixes): Update `build.ps1` to compile `EliteStartMenu.ps1` only if it exists, and fix the E2E script variable names (`$hwndTrayNotify` and `$hwndTrayClock`) in `Subagent_Tests\run_comprehensive_e2e.ps1`
- [ ] Compile and sign all binaries successfully using `build.ps1`
- [ ] Write handoff.md and complete the run
