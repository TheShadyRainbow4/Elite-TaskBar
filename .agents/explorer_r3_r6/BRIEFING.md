# BRIEFING — 2026-07-04T20:08:06-07:00

## Mission
Analyze Portable Mirror Mode and Settings Synchronization & CPL Repair for Elite-TaskBar.

## 🔒 My Identity
- Archetype: teamwork_preview_explorer
- Roles: explorer_1
- Working directory: C:\Users\Administrator\Desktop\Elite-TaskBar\.agents\explorer_r3_r6
- Original parent: f2f647cc-0a56-4fa6-935c-de6b9def612a
- Milestone: R3 and R6 Exploration

## 🔒 Key Constraints
- Read-only investigation — do NOT implement
- Follow EliteSoftwareTech Co. GUI guidelines (Segoe UI semibold, visual styles, no dark mode, witty tooltips)
- Maintain Git hygiene, build via build.ps1 chain, settings/CPL mirroring

## Current Parent
- Conversation ID: f2f647cc-0a56-4fa6-935c-de6b9def612a
- Updated: not yet

## Investigation State
- **Explored paths**:
  - `build.ps1` and `build_settings.ps1`
  - `SourceFiles/EliteSettingsCpl.cpp`, `EliteSettingsStub.cpp`, `TaskbarProperties.cpp`
  - `Remaining_Shell/Win32Explorer_26.0.3.0/App_Source/RegistryAppStorageFactory.cpp`, `App.cpp`, `Config.h`, `ConfigRegistryStorage.cpp`, `ConfigXmlStorage.cpp`
  - `Remaining_Shell/Win32Explorer_26.0.3.0/Shared_Libraries/SetDefaultFileManager.cpp`, `GeneralOptionsPage.cpp`
  - `SourceFiles/EliteSettings.ps1`
- **Key findings**:
  - **R6 Portable Mirror Mode**: RegistryAppStorageFactory hardcodes HKLM. We must parameterize it with a `useHKLM` boolean. Toggling between HKLM and HKCU is driven by the `enablePortableMirror` config option. If enabled, the config is saved in `config.xml` AND HKLM registry. In `TaskbarProperties.cpp`, we will use `GetEliteRegistryRoot()` to determine HKLM vs HKCU dynamically.
  - **R3 Settings Sync & CPL Repair**: The CPL build currently compiles `EliteSettingsStub.cpp` and links `settings_resources.res` which is for the taskbar dialog properties, instead of `EliteSettingsCpl.cpp` and `settings_cpl.res`. This bypasses the embedded `EliteSettings.exe` extraction. Using `EliteSettingsCpl.cpp` and linking `settings_cpl.res` launches the embedded `EliteSettings.exe` compiled from `EliteSettings.ps1`, ensuring 100% UI mirroring.
  - **Replace Explorer bug**: In `GeneralOptionsPage.cpp`, the cleanup of shell extensions is conditional on `m_config->replaceExplorerMode`, causing the user to get stuck if registry state goes out-of-sync. Changing the cleanup to be unconditional (removing both directory and folder extensions) fixes this. Exposing the options in `EliteSettings.ps1` and `TaskbarProperties.cpp` matches this logic.
- **Unexplored areas**: None. The codebase is thoroughly mapped for this task.

## Key Decisions Made
- Expose "Replace Explorer" and "Portable Mirror" settings in the C++ UI (`TaskbarProperties.cpp`) and the WinForms PowerShell UI (`EliteSettings.ps1`).
- Clean up default file manager keys unconditionally in both C++ and PowerShell to resolve the "stuck" bug.
- Parameterize `RegistryAppStorageFactory` to accept a dynamic registry root and modify `App::SaveSettings` to support dual-write (XML + HKLM).

## Artifact Index
- None
