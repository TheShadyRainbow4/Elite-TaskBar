# Handoff Report - explorer_testing

## 1. Observation

### Existing E2E Test Scripts (`Subagent_Tests/`)

1. **`Subagent_Tests/verify_desktop_shell.ps1`**:
   - Compiles Win32 helper definitions in memory using C# `Add-Type -TypeDefinition` mapping to functions from `user32.dll` (`FindWindowW`, `FindWindowExW`, `SendMessageW`, etc.).
   - Sets registry values to `0` under `HKCU\Software\EliteSoftware\Win32Explorer\Advanced` (`DesktopReplacementEnabled`, `DesktopWallpaperEnabled`, `DesktopIconsEnabled`, `FallbackStartMenuEnabled`).
   - Runs E2E tests:
     - **Test 1 (`SettingsRegistryToggles`)**: Launches `EliteSettings.exe`. Resolves property sheet windows and handles tabs dynamically. Simulates `BN_CLICKED` commands for checkboxes `290` (`DesktopReplacement`), `291` (`DesktopWallpaper`), `292` (`DesktopIcons`), and `293` (`FallbackStartMenu`). Saves via `IDOK` (1) and asserts that the registry keys are set to `1` on settings exit.
     - **Test 2 (`DesktopStartupDynamic`)**: Runs `EliteTaskbar.exe` first with `DesktopReplacementEnabled = 0` and verifies no custom `Progman` class window belongs to the process. Toggles the key to `1` and verifies that the `Progman` window is created.
     - **Test 3 (`ClassRegistration`)**: Queries the custom `Progman` window to verify its child windows exist: `Progman` -> `SHELLDLL_DefView` -> `SysListView32`.
     - **Test 4 (`ZOrderConstraints`)**: Clicks the desktop window and verifies it doesn't steal focus. Asserts that `WM_MOUSEACTIVATE` returns `MA_NOACTIVATE (3)`. Traverses window list via `GetWindow(..., GW_HWNDNEXT)` to ensure the window remains at the bottom of the Z-order.
     - **Test 5 (`DesktopIconsLoading`)**: Sends `LVM_GETITEMCOUNT` (0x1004) to the desktop `SysListView32` and asserts that the returned item count is `> 0`.
     - **Test 6 (`DirectoryChangeNotify`)**: Creates a temporary file `EliteTestIcon.txt` on the Desktop. Sleeps, verifies `LVM_GETITEMCOUNT` increases, deletes the file, and verifies it returns to the original count.
     - **Test 7 (`StartButtonFallback`)**: Copies `BuildOutput\EliteStartMenu.exe` as `StartMenu.exe`. Simulates left click via `WM_LBUTTONDOWN` and `WM_LBUTTONUP` on the `Elite_StartOrbWnd` start button and asserts that `StartMenu.exe` process is spawned.
   - Output is saved to `Subagent_Tests\desktop_shell_test_output.txt`.

2. **`Subagent_Tests/run_empirical_tests.ps1` & `run_re_verification.ps1`**:
   - Use `psexec64` to launch `Win32Explorer.exe` non-elevated (avoiding UIPI / drag-and-drop issues).
   - Run tests:
     - **`SmallIconTilesView`**: Writes `ViewModeGlobal = 12` under `HKCU\Software\Win32Explorer\Settings`, starts `Win32Explorer.exe`, and verifies `LVM_GETVIEW` on `SysListView32` returns `4` (`LV_VIEW_TILE`) and the image list handle is not `NULL`.
     - **`DefaultGroupByType`**: Deletes registry settings, launches explorer, and checks `LVM_ISGROUPVIEWENABLED` is `1` on first run, and `EnableDefaultGroupByType` registry key flushes as `1`.
     - **`OptionsToggleAndSave`/`OptionsToggleRegistry`/`OptionsToggleXML`**: Launches explorer, simulates opening the Options dialog (WM_COMMAND 40101), toggles the default group checkbox (ID 1382), clicks "Okay", closes explorer, and verifies settings persist in Registry (`EnablePortableMirror = 0`) or `config.xml` (`EnablePortableMirror = 1`).
     - **`EliteTaskbarIsolation`** (in `run_empirical_tests.ps1` only): Sets `EnableEliteTaskbar = 1`. Launches `Win32Explorer.exe` (which spawns `EliteTaskbar.exe`). Closes explorer and verifies `Win32Explorer.exe` terminates but `EliteTaskbar.exe` continues running.
   - Outputs are saved to `Subagent_Tests\test_results.txt` and `Subagent_Tests\verdict.txt`.

---

### Registry Keys for the 4 Target Features

#### Feature 1: Desktop Background
- **Elite Custom Keys** (Read in `SourceFiles/DesktopWindow.cpp:428` and `SourceFiles/TaskbarProperties.cpp:1238`):
  - **Key Path**: `HKCU\Software\EliteSoftware\Win32Explorer\Advanced` (under `GetEliteRegistryRoot()`)
  - **Value**: `DesktopWallpaperEnabled`
  - **Type**: `REG_DWORD`
  - **Values**: `1` (draw custom wallpaper), `0` (do not draw, fall back to native color)
- **Standard Windows Wallpaper Keys** (Read in `SourceFiles/DesktopWindow.cpp:438`):
  - **Key Path**: `HKEY_CURRENT_USER\Control Panel\Desktop`
  - **Values**:
    - `Wallpaper` (Type: `REG_SZ`): Absolute path to the background image file (e.g. `C:\Windows\web\wallpaper\Windows\img0.jpg`).
    - `WallpaperStyle` (Type: `REG_SZ`): Style code:
      - `"0"`: Center
      - `"2"`: Stretch
      - `"6"`: Fit (Letterbox / Pillarbox)
      - `"10"`: Fill (Default)
      - `"22"`: Span
    - `TileWallpaper` (Type: `REG_SZ`): `"1"` (Tile wallpaper), `"0"` (No tiling)
- **Custom Theme Path Key** (Read in `SourceFiles/TaskbarProperties.cpp:564`):
  - **Key Path**: `HKCU\Software\EliteSoftware\Win32Explorer\Advanced` (under `GetEliteRegistryRoot()`)
  - **Value**: `CustomThemePath`
  - **Type**: `REG_SZ`
  - **Values**: Absolute directory path where custom PNG/ICO files are located.
- **Slideshow Timings / Theme Dropdowns**:
  - *Not yet implemented* in the C++ codebase. Timing registry keys (e.g. `SlideshowInterval`) or multi-monitor slideshow configurations are absent from `DesktopWindow.cpp` and `TaskbarProperties.cpp` source code. Currently, wallpaper drawing in `DesktopWindow.cpp:424` checks `Control Panel\Desktop` values, loads the bitmap, and draws it statically without any timer-based transitions.

#### Feature 2: Quick Launch
- **Key Path / Value Names**: *Not yet implemented* in the C++ codebase.
- **Expected Paths**: 
  - Enabled state: `HKCU\Software\EliteSoftware\Win32Explorer\Advanced` -> `QuickLaunchEnabled` (Type: `REG_DWORD`).
  - Folder path: `%APPDATA%\Microsoft\Internet Explorer\Quick Launch`.
- **Expected C++ behavior**: Taskbar properties need to write a toggle. The taskbar window (`TaskbarWindow.cpp`) needs to initialize a `ToolbarWindow32` control on the left side of the program buttons, parse `.lnk` files in the Quick Launch directory using `IShellLinkW` / `IPersistFile` to extract icons and targets, support non-elevated drag-and-drop (`IDropTarget`), and reposition active task switch buttons.

#### Feature 3: 2-Row Tray
- **Key Path / Value Names**: *Not yet implemented* in the C++ codebase. (Only `TrayMode` exists under `Advanced` to switch between classic linear tray (`1`) and native Win7 tray (`0`)).
- **Expected Paths**: `HKCU\Software\EliteSoftware\Win32Explorer\Advanced` -> `TrayTwoRowsEnabled` or `TrayRows` (Type: `REG_DWORD`).
- **Expected C++ behavior**: When active, `TrayNotifyWnd` inside `TaskbarWindow.cpp` should wrap tray icons into two rows within the taskbar height using smaller icon sizes (e.g. 12x12 or 16x16 with tighter padding).

#### Feature 4: Clock Seconds
- **Key Path / Value Names**: *Not yet implemented* in the C++ codebase. The clock formatting inside `ClockWidget.cpp:18` and `TaskbarWindow.cpp:1380` hardcodes `TIME_NOSECONDS` in `GetTimeFormatW`.
- **Expected Paths**: `HKCU\Software\EliteSoftware\Win32Explorer\Advanced` -> `ShowClockSeconds` (Type: `REG_DWORD`).
- **Expected C++ behavior**: When enabled, `GetTimeFormatW` should omit the `TIME_NOSECONDS` flag. The clock redraw timer must fire every `1000` milliseconds instead of only on minute boundary ticks.

---

## 2. Logic Chain

1. **Test Verification Observation**: Inspection of files under `Subagent_Tests/` reveals that test automation scripts like `verify_desktop_shell.ps1`, `run_empirical_tests.ps1`, and `run_re_verification.ps1` dynamically check registry settings and query runtime properties of the UI (such as finding window classes `Progman`, `SHELLDLL_DefView`, and checking `LVM_GETVIEW` via message passing).
2. **Registry Mapping Observation**: Code searches on `RegQueryValueExW` and `RegSetValueExW` in `TaskbarProperties.cpp`, `TaskbarWindow.cpp`, and `DesktopWindow.cpp` reveal the exact keys queried:
   - Custom keys are located under `Software\EliteSoftware\Win32Explorer\Advanced` under root `GetEliteRegistryRoot()` (dynamic mapping to HKCU or HKLM).
   - Wallpaper keys are queried from standard user preferences in `HKEY_CURRENT_USER\Control Panel\Desktop` (`Wallpaper`, `WallpaperStyle`, `TileWallpaper`).
3. **Missing Features Observation**: Searching for "Quick", "Seconds", "Slideshow", or "Row" in `SourceFiles/` shows no registry queries or UI code for slideshow timings, Quick Launch shortcuts, 2-row tray wraps, or clock seconds. Lines `ClockWidget.cpp:18` and `TaskbarWindow.cpp:1380` explicitly call `GetTimeFormatW(..., TIME_NOSECONDS, ...)`.
4. **Conclusion Reasoning**:
   - The Desktop Background feature currently draws static wallpapers based on the user's Control Panel settings when `DesktopWallpaperEnabled = 1`. Theme folders are tracked via `CustomThemePath` (REG_SZ). However, per-monitor slideshow, timing loops, and theme lists do not exist.
   - Quick Launch, 2-Row Tray, and Clock Seconds are planned features that have no implementation or registry keys in the current codebase.

---

## 3. Caveats

- **Advanced Registry Root**: Elite keys are resolved via `GetEliteRegistryRoot()`. If `EnablePortableMirror` (REG_DWORD) is `1` in either HKLM or HKCU `Software\EliteSoftware\Win32Explorer\Advanced`, the registry root falls back to `HKEY_LOCAL_MACHINE`. If not, it defaults to `HKEY_CURRENT_USER`.
- **Windows Version Dependency**: Wallpaper style code `"22"` (Span) is only fully supported by GDI+ on Windows 8 and higher. On older OS versions (like Vista/Win7), it might fallback to Stretch or Tile.

---

## 4. Conclusion

- **Existing Coverage**: The E2E tests currently cover settings checkbox synchronization, dynamic desktop replacement startup/z-order constraints/icon grid list updates, fallback start menu launches, explorer view mode changes (`SmallIconTilesView`, default grouping), and process lifetime isolation.
- **Implementation Status**: Timed slideshows, custom theme listing dropdowns, clock seconds toggle, 2-row system tray layouts, and resizable Quick Launch toolbars are not present in the current codebase and must be implemented to fulfill upcoming Milestone requirements.

---

## 5. Verification Method

- To inspect the current test execution:
  - Run `powershell -ExecutionPolicy Bypass -File Subagent_Tests\verify_desktop_shell.ps1` from the root directory.
  - Run `powershell -ExecutionPolicy Bypass -File Subagent_Tests\run_empirical_tests.ps1` from the root directory.
- Verify registry references by checking:
  - `SourceFiles/DesktopWindow.cpp` around line 424.
  - `SourceFiles/TaskbarProperties.cpp` around line 1224.
  - `SourceFiles/ClockWidget.cpp` around line 18.

---

# TEST_INFRA.md Coverage Matrix & Recommendations

The table below reviews the 10 features listed in `TEST_INFRA.md` and details:
1. **Existing Test Cases**: What is already tested in the `Subagent_Tests/` folder.
2. **Needed Test Cases**: Concrete test cases required to fully cover Tier 1 (Feature), Tier 2 (Boundary), Tier 3 (Cross-Feature), and Tier 4 (Scenario) thresholds.

| Feature # | Feature Name | Current Tests | Needed Tier 1 (Feature) | Needed Tier 2 (Boundary) | Needed Tier 3 (Cross-Feature) | Needed Tier 4 (Scenario) |
|---|---|---|---|---|---|---|
| **1** | Multi-Monitor Flyouts | None | - Flyout opens on secondary monitor click.<br>- Sound/Volume flyouts route to correct display.<br>- Position aligns to secondary taskbar edges. | - Multi-monitor setups with different screen resolutions/DPIs.<br>- Display unplugged while flyout is active. | - Flyout positioning while taskbar auto-hide is enabled on secondary screen. | **Scenario 2 (Multi-Monitor Workspace)**: Flyout coordinates, clocks, and secondary tray items are tested together. |
| **2** | Clock/Tray Gap & 2-Row Tray | None | - Verifying clock/tray spacing.<br>- Toggling 2-Row tray in Settings page.<br>- Verifying tray icon heights (e.g. 2 rows of 16px). | - 100+ tray icons loaded (stress-testing row wrapping).<br>- Padding adjustment on vertical taskbars. | - 2-Row tray active under "Small Icons" taskbar mode. | **Scenario 1 (Desktop Shell Replacement)**: Exercises custom 2-Row tray and Quick Launch layout boundaries. |
| **3** | Tray Icon Backgrounds & Fallbacks | None | - Check tray icon transparency.<br>- Check fallback default icon usage. | - Corrupted/invalid icon handles.<br>- 32-bit vs 64-bit shell tray notify messages. | - Tray Mode switches (Legacy vs Native) with transparent icons. | **Scenario 3 (Shell Upgrade & Migration)**: Verification of tray icon re-population on explorer restart. |
| **4** | Settings UI & About Dialog | - `SettingsRegistryToggles` (checkbox writes)<br>- `OptionsToggleRegistry`/`OptionsToggleXML` | - Start Menu tab rendering without hover.<br>- About dialog expansion controls.<br>- Import/Export profiles. | - Empty/missing custom theme paths.<br>- Settings save with read-only registry keys. | - Theme custom settings applied dynamically during active desktop slideshow. | **Scenario 3 (Shell Upgrade & Migration)**: Apply changes, reload without duplicate windows. |
| **5** | Clean Up old.exe & Reload Bug | - `EliteTaskbarIsolation` (lifetime isolation) | - Deletion of legacy `*old*.exe` files on startup.<br>- Re-execution after Apply does not spawn extra explorer windows. | - `*old*.exe` files that are locked by system processes.<br>- Multi-threaded settings applies in short interval. | - Setting changes applied while taskbar process is forced to terminate. | **Scenario 3 (Shell Upgrade & Migration)**: Clean upgrade migration with deletion of old binaries. |
| **6** | Progman Multi-Display & Desktop Tab | - `DesktopStartup` (reg)<br>- `ClassRegistration` (hierarchy)<br>- `ZOrderConstraints` (bottom lock)<br>- `DesktopIcons` (count)<br>- `DirChangeNotify` | - Displaying replacement on all monitors.<br>- Custom wallpaper drawing.<br>- Slideshow interval timings. | - Slideshow with missing/invalid background files.<br>- Timing intervals set below 3 seconds (boundary). | - Desktop Replacement + secondary monitor flyouts active. | **Scenario 1 (Desktop Shell Replacement)**: Verifies slideshow, icons, Win+R hooks and 2-Row tray. |
| **7** | Keyboard Hooks (Win+R) | None | - Pressing `Win+R` launches Run dialog.<br>- Custom keyboard combinations registered. | - Pressing keys when native `explorer.exe` is killed.<br>- Callback hooks timing out. | - Shortcut overrides while settings dialog or desktop is active. | **Scenario 1 (Desktop Shell Replacement)**: Exercises keyboard hook fallbacks when Explorer is dead. |
| **8** | Tray Click Actions | None | - Single click Win32Explorer tray -> About.<br>- Double click Win32Explorer tray -> New window. | - Double-click speed threshold boundary values.<br>- Clicking during process load state. | - Tray actions when Taskbar mode is toggled between Independent and Replace. | **Scenario 2 (Multi-Monitor Workspace)**: Custom click handlers for secondary display taskbar tray icons. |
| **9** | Taskbar Extras (Seconds, QuickLaunch) | None | - Toggle clock seconds.<br>- Load Quick Launch from folder shortcuts. | - Clock updating at exactly 1000ms intervals.<br>- Empty or missing AppData Quick Launch folder. | - Resizable Quick Launch boundaries with 2-Row tray icon wraps. | **Scenario 1 (Desktop Shell Replacement)**: Exercises Quick Launch shortcut wrapping and seconds tick. |
| **10** | Win32Explorer View Modes | - `SmallIconTilesView`<br>- `DefaultGroupByType` | - Apply "Tiles with thumbnails".<br>- Apply "Small tiles with thumbnails". | - Folder contains corrupted images.<br>- Loading high density folders (10,000+ files) in thumbnail modes. | - View mode changes saved to `config.xml` under Portable Mirror mode. | **Scenario 1 (Desktop Shell Replacement)**: Integrates desktop shell replacement and file view modes. |
