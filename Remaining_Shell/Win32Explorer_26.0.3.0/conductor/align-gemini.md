# Project Alignment and Feature Expansion Plan

## Objective
Bring the project into strict alignment with the `GEMINI.MD` Developer Summary, integrate extensive new UI and view variants, centralize registry configuration, remove all dark mode logic (while keeping the setting grayed out), and resolve identified UI glitches. All changes will strictly adhere to the project laws, including continuous append-only logging and the new mandates.

## Key Files & Context
- `planned_features.md` (To be created in project root)
- UI Components: `ShellTreeView`, `HolderWindow`, `MainTabView`, `TabBacking`.
- Dialogs: `DisplayColorsDialog.cpp/h`, `AboutDialog.cpp`, `AppearanceOptionsPage.cpp`.
- Configuration: `ConfigRegistryStorage.cpp`, `ApplicationEditorDialog.cpp`.
- Views & Shell: `FolderView.cpp`, `ShellView.cpp`, `GroupManager.cpp`.
- Build System: `App_Source/Win32Explorer.vcxproj`, `App_Source/VersionConstants.h`.
- Dark Mode: `DarkModeManager.cpp/h`, `DarkModeColorProvider.cpp/h`.

## Implementation Steps

### Phase 1: Setup & Pre-requisites (Completed)
- **Update `GEMINI.MD` Laws**: Added laws for continuous append-only logging, never removing features, and dark mode removal "out of spite".
- **Project Backup**: Complete project backup to `Win32Explorer_26.0.3.0_BACKUP.zip`.

### Phase 2: Planned Features File (Completed)
- **Create `planned_features.md`**: Created a file in the project root containing the verbatim quote of the user's extensive feature request, exactly as provided.

### Phase 3: Documentation Generation & Logging Infrastructure (In Progress)
- **Directory-level Logging**: Initialize or append to `gemini.md` files in all modified directories (e.g., `App_Source/gemini.md`, `Docs/gemini.md`).
- **Analysis Files**: Create `Docs/ComponentAnalysis_TreeAndTabs.md` and `Docs/Architecture_Analysis.md`.

### Phase 4: About Window Icon Fix (Completed)
- **Fix Glitchy Icon**: Modified `AboutDialog.cpp` to reference the correct small icon size and `LR_DEFAULTCOLOR` to resolve color glitches.

### Phase 5: Registry & Configuration Updates (In Progress)
- **Shared Registry Location**: Update `ConfigRegistryStorage.cpp` and `StartupCommandLineProcessor.cpp` to save settings to `HKEY_LOCAL_MACHINE`.
- **Instant Apply**: Modify the "Apply" button logic in settings dialogs to trigger an `OnConfigChanged` broadcast across the application so UI options apply instantly.

### Phase 6: Native Styling Updates (Vista Aero Support)
*(Note: `DisplayWindow` styling omitted per user request).*
- **Folder Tree View**: Remove horizontal scroll bar and apply the "Explorer" theme via native Windows API.
- **HolderWindow**: Update to use `ExplorerBar` theme class (`EBP_NORMALGROUPHEAD`) for the caption area.
- **Tabs**: Remove `TCS_EX_FLATBUTTONS` from `MainTabView` and update `TabBacking` to use native `TAB` theme parts (`TABP_PANE`).
- **Custom Close Buttons**: Update the close button for both Tab and Tree Views (via `ToolbarHelper` or directly in `TabBacking`/`HolderWindow`) to use `close-button.png` (normal) and `pressed-close-button.png` (hover/pressed), sized at 24x24. Both states must be properly handled.

### Phase 6.5: Dark Mode Removal
- **Logic Purge**: Search for and cleanly remove `DarkModeManager`, `DarkModeColorProvider`, and related logic throughout the application (e.g., `App`, `Explorerplusplus`, `HolderWindow`).
- **Spiteful Checkbox**: Locate the dark mode setting checkbox (likely in `AppearanceOptionsPage.cpp`), disable it using `EnableWindow(hwnd, FALSE)`, and remove any functional logic tied to it.

### Phase 7: Views, Grouping, & Sorting Overhaul
- **Expanded Views**: Add all native Explorer views with thumbnails to the view options list, including the Details view and all layout variants (text below, text to right, small/medium/large icons, tiles).
- **Grouping & Sorting**: 
  - Allow sorting and grouping simultaneously.
  - Implement logic to select/add specific groups not in the default list.
  - Add functionality to always group by a specific filter.

### Phase 8: Display Colors Dialog Implementation
- **Implement Dialog**: Fully implement `DisplayColorsDialog.cpp/h`, replacing the disabled stub. Resolve signature mismatches, fix unreferenced parameters, and correct `COLOR16` conversions.
- **Project Integration**: Synchronize `App_Source/Win32Explorer.vcxproj`.

### Phase 9: Version Bump & Final Build
- **Version Update**: Increment `MICRO_VERSION` from 3 to 4 in `App_Source/VersionConstants.h`.
- **Build Execution**: Rebuild `Win32Explorer.exe` (`Release|x64`).
- **Relocation**: Copy the executable to the project root.
- **Final Log**: Append final build success details to respective directory logs.

### Phase 10: Automation Script
- **Create Script**: Write a PowerShell script (`build_and_package.ps1`) that automates:
  1. Zipping the current project directory as a backup.
  2. Invoking MSBuild/vcpkg to compile the project.
  3. Invoking the Inno Setup Compiler (`ISCC.exe`) located at `C:\Users\zwhiteman\AppData\Local\Programs\Inno Setup 6\ISCC.exe` to package the resulting executable into an installer.
- **Inno Setup Script**: Create a basic `.iss` file to define the installer structure if one doesn't exist.

## Verification & Testing
- Build succeeds without linker errors.
- New views and thumbnail layouts are available and functional.
- Settings apply instantly across multiple windows.
- About window icon renders correctly.
- Dark mode logic is completely removed, but the grayed-out checkbox remains in the settings dialog.
- `gemini.md` logging files accurately reflect all changes.