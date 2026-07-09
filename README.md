<div align="center">

<a href='https://i.postimg.cc/DZTjtNg1/Elite-Software-Company-Simulator-LOGO.png' target='_blank'><img src='https://i.postimg.cc/DZTjtNg1/Elite-Software-Company-Simulator-LOGO.png' border='0' alt='Elite-Software-Client' width="200"></a> 

# **Elite-TaskBar**

*A master architectural project for a classic Windows-style Taskbar replacement tool.*

</div>

---

## 🏢 Core Information 

* **Company**: EliteSoftware / EliteSoftwareTech Co. 
* **Authors**: Zachary Whiteman, Susan Gemm, TheShadyRainbow4, EliteSoftwareTech Co.  
* **Document Version**: 1.2.0.0  
* **Target OS**: Windows 10 IoT Enterprise LTSC (Legacy compatible with Vista / 7)
* **Core Technologies**: Native Unmanaged C++ (Taskbar), .NET Framework 4.6 WinForms (Settings Applet) or Native Windows Functions

---

## 🚀 Elite-Taskbar Deep Dive & Features

### 🖼️ Core Features & Capabilities

Elite-Taskbar is designed from the ground up to seamlessly blend into the Windows environment, providing a perfect legacy Aero Glass experience while retaining 100% of the functionality expected from the Windows Shell.

<details>
<summary><b>1. Core Shell Replacements</b></summary>

* **Complete Taskbar Overhaul**: Aero Glass rendering, Topmost Z-order, per-monitor DPI scaling.
* **Win32Explorer Integration**: Deep integration with the Win32Explorer file manager, capable of completely hijacking native folder browsing.
* **Fallback Start Menu (Open-Shell Integration)**: Integrated custom Start Menu using assimilated Open-Shell (`StartMenu.exe`) invocation hooks on Start Button click.
* **Desktop Replacement (Progman & SysListView32)**: Native `Progman` replacement window with subclassed `SHELLDLL_DefView` and `SysListView32` controls. Features high-quality GDI+ wallpaper drawing with smart bitmap caching to avoid disk reads on paint, and populates desktop icons via `IShellFolder` Namespace binding.


</details>

<details>
<summary><b>2. Visual & Theming Engine</b></summary>

* **GDI & GDI+ Drawing**: Pixel-perfect UI elements, including 3-state animated Start Orbs (Normal, Hover, Pressed).
* **High DPI Awareness**: `PerMonitorV2` compliance ensuring crisp text and imagery across multi-monitor setups.
* **Multi-Monitor DPI Scaling (R7)**: Dynamic recalculation of taskbar sizing, window positions, and fonts upon receiving `WM_DPICHANGED` messages.
* **Custom Start Orbs**: Included themes like Indigo, Sapphire, Uranus, Dunes, AquaBottom, and ViennaBottom.
* **Legacy System Tray**: Options for a modern tray or a classic retro linear system tray.

</details>

<details>
<summary><b>3. Advanced Settings & Configuration</b></summary>

* **EliteSettings Applet**: A dedicated CPL and EXE properties menu utilizing classic WinForms & native C++ property sheets.
* **Aggressive Registry Sync**: Writes settings dynamically to `HKCU` to perfectly match native Explorer behaviors (e.g., `TaskbarSizeMove`, `TaskbarSmallIcons`).
* **Settings Import/Export**: Instantly backup and restore all shell configurations, including native Windows parameters, to a `.reg` file with hot-reloading.

</details>

<details>
<summary><b>4. Native Behavior & Integrity</b></summary>

* **Intelligent Context Menus**: Routes raw undocumented Shell Command IDs (e.g., Cascade Windows, Show Desktop) for 100% native execution.
* **Robust Tray Clock**: Hooks into the native `TrayClockWClass` and seamlessly summons the standard Windows calendar flyout.
* **Graceful Degradation**: Fallback mechanisms (e.g., DWM buffer to raw GDI `DrawTextW`) ensure critical UI elements like the clock are always visible even if themes break.
* **Double Notification Area Scraping (R4)**: Aggregates scraped icons from both the visible taskbar tray and the hidden `NotifyIconOverflowWindow` toolbar.
* **UWP Application Icon Resolving (R5)**: Resolves and extracts high-resolution modern app icons directly from UWP layouts, replacing standard fallback icons.

</details>

<details>
<summary><b>5. Added Features (Teamwork Update)</b></summary>

* **Milestone 1: HKLM Registry Alignment & Shell Replacement**: Unconditionally aligned the suite to utilize `HKEY_LOCAL_MACHINE` registry keys for advanced and master configurations, preventing per-user config isolation. Implemented the primary shell replacement checkbox `IDC_PRIMARY_SHELL_MODE` ("Run EliteTaskbar as primary Windows shell replacement") in the "Native Settings" tab, mapped to HKLM's Winlogon Shell value. Added graceful fallback registration to `Shell_SecondaryTrayWnd` when `Shell_TrayWnd` registration fails. Added dynamic Z-order locking to force the taskbar window to remain `HWND_TOPMOST` (or behind the native taskbar in hybrid/independent mode) and Progman to remain `HWND_BOTTOM` on `WM_WINDOWPOSCHANGING`. Configured settings change notifications (`WM_SETTINGCHANGE` and settings applet apply threads) to reload only the taskbar component itself (`EliteTaskbar.exe`) rather than force-restarting the entire Explorer/shell system. Integrated user updates for dynamic tray toolbar sizing, independent mode tray routing, native desktop shell view hosting, and ODR violation resolutions.
* **Small Icon Tiles View Mode (R1)**: A brand new view mode in Win32Explorer that displays small icons with right-aligned labels.
* **Group by Type Default (R2)**: A new toggle in the Win32Explorer Options window setting the default grouping logic to Type.
* **Independent Taskbar Mode (R3)**: Taskbar now defaults to co-existing with the native explorer taskbar rather than attempting to replace it on first launch.
* **UWP App Icons Fix (R5)**: Accurate rendering of UWP app icons on the taskbar.
* **High-DPI Text Blurriness Fix (R7)**: Font crispness is preserved across mismatched high-DPI monitor configurations.
* **Settings Import/Export**: Backup and restore all shell configurations, native settings, and Elite-Taskbar behaviors directly into a single `.reg` file.
* **Aggressive Restart**: Immediate and full application restart upon applying settings to prevent hidden UI glitches or the "start orb top-left" bug.
* **Desktop Replacement (Phase XI)**: Native `Progman` replacement window with subclassed `SHELLDLL_DefView` and `SysListView32` icon grid. Features high-quality GDI+ wallpaper drawing with smart bitmap caching to avoid redundant disk reads on paint. Supports double-click file execution, inline renaming, and folder watching via `SHChangeNotifyRegister` with a 100ms debounce timer.
* **Fallback Start Menu (Phase XIX)**: Automatic Open-Shell integration that launches the Open-Shell Start Menu (`StartMenu.exe`) from standard, relative, or WinPE directories when clicking the Start button.
* **About Dialog Layout Spacing Optimization**: Re-engineered dialog layouts to eliminate dead space, bringing collapsed height to 110 DUs and expanded to 192 DUs with dynamic button relocation and border height calculation.
* **Start Menu Settings Tab Fix**: Added runtime migration toggle `MigrateStartMenuSettings` (defaulting to 1) to dynamically migrate per-monitor Start Menu comboboxes and previews from the Multi-Monitor tab scroll container to the Start Menu tab scroll container.
* **Property Sheet Sarcastic Tooltips**: Injected custom subclassing logic to dynamically set witty and sarcastic tooltips on standard Property Sheet buttons (Okay, Cancel, Apply).
* **Settings Apply DLL Module Path Resolution**: Patched the folder fallback logic inside the DLL to use `g_hInstance` instead of `NULL` to avoid launching from the System32 directory.
* **DDF Compression Pipeline Acceleration**: Restructured the backup pipeline to ignore locked log/txt files and scan only active code folders, accelerating backups from a hang to under a second.
* **Automated Verification Script (verify_final_polish.ps1)**: Programmatically validates the application of Desktop Background, Quick Launch, 2-Row Tray, and Clock Seconds toggles across both normal (HKCU) and Portable Mirror (HKLM & XML) modes.
* **Comprehensive E2E Test Suite (Subagent_Tests/run_comprehensive_e2e.ps1)**: Runs E2E verification of 10 features across 4 tiers (Feature, Boundary, Pairwise, Scenario) utilizing non-disruptive, single-shell lifecycle dispatch.
* **Tray Item Limit Extension**: Replaced the hardcoded inline notification tray limit of 4 with a new maximum of 48 (`TRAY_LIMIT`), correctly handling overflow rendering and mouse interaction routing across primary and secondary screen taskbars.
* **Missing Tray Icons Process Fallback**: Integrated process icon extraction (`GetProcessIcon`) using `QueryFullProcessImageNameW` and `SHGetFileInfoW` alongside a generic application icon fallback to eliminate blank gaps in the system tray.
* **White Background Bar Theme Removal**: Stripped native visual style themed backgrounds from the notification area `SysPager` and `ToolbarWindow32` controls, and subclassed `WM_ERASEBKGND` to draw the transparent parent background.
* **Desktop Personalization Overrides (Milestone 7)**: Implemented dual-engine wallpaper personalization options. A registry switch `UseNativeWallpaperEngine` under `Software\EliteSoftware\Win32Explorer\Advanced` (defaults to 1) dynamically controls whether wallpaper configuration and styles are read directly from `Control Panel\Desktop` and applied natively via `SystemParametersInfoW`, or rendered using custom GDI+ logic.
* **ListView Enhancements (Milestone 7)**: Enhanced the custom desktop icon ListView with native Explorer theme subclassing (`SetWindowTheme` using `"Explorer"`), high-resolution 48x48 system image lists (`SHGetImageList`), and free drag-and-drop icon placement by disabling auto-arrange style (`LVS_AUTOARRANGE`) and triggering `ListView_Arrange`.
* **Desktop Personalization & Themes Settings (Milestone 7)**: Extends custom desktop replacement with Span and Per-Monitor wallpaper rendering modes using `EnumDisplayMonitors`, dynamic display layout resizing on `WM_DISPLAYCHANGE`, and wallpaper slideshow support using a Win32 timer. Integrates a settings page with theme detection, preview icons, browse button, tutorial SysLink, and a custom GDI+ wallpaper preview control, fully mirrored in the CPL and Settings executables.
* **Desktop Icon Outlines & Unicode Labels (Milestone 7)**: Fixed label truncation by directly sending `LVM_GETITEMW` and `LVM_INSERTITEMW` messages, and forced label transparency by using `ImageList_SetBkColor(hSysIL, CLR_NONE)`.
* **Desktop Thumbnails (Milestone 7)**: Dynamically generates 48x48 file thumbnails using `IShellItemImageFactory`, with a configurable settings toggle `DesktopThumbnailsEnabled` and automatic fallback to standard system icons.
* **Desktop Icon Placement Persistence (Milestone 7)**: Position coordinate coordinates are serialized to a custom registry key (`DesktopIconPositions`) upon refresh (`WM_SHELLCHANGE`) or exit, and automatically loaded and restored on startup.
* **System Tray Multi-Monitor Defaults (Milestone 7)**: System tray clock, icons, and tasks now default to rendering on all displays out-of-the-box (defaulting to enabled/checked when registry configuration keys do not exist).
* **Massive Settings Dialog Extensions (Milestone 7)**: Added three comprehensive new tabs with Aero-compliant layouts (Explorer Settings, DWM Settings, and Colors & Themes) to manage folder options, DWM animations and glass transparency, highlight colors, and dark theme modes. Features full XML-based configuration backup and restore functionality.
* **Quick Launch Folder Toolbars (Milestones 8, 9, 10)**: Dynamic folder-based toolbar parsing engine (Rebar container) that resolves shortcuts (.lnk) and executables using `IShellLink` and SHGetFileInfo, caching configurations to HKLM registry, with toggles for titles, text, and Large/Small icon sizes.
* **File Manager Dialog Hijacking (Milestones 8, 9, 10)**: Overwrites `Directory\shell` and `Folder\shell` registry entries under HKLM to route folder opening events directly to `Win32Explorer.exe`.
* **WinPE & Server Core Fallback Rendering (Milestones 8, 9, 10)**: Dynamic DWM/Theme detection that falls back to custom borderless GDI+ desktop views and renders hardcoded wallpapers when uxtheme and DWM are missing.
* **Settings Timer Debounce & GDI Cleanup (Milestones 8, 9, 10)**: Implemented a 1000ms debounce timer for `WM_SETTINGCHANGE` to prevent spawner loops, alongside strict GDI icon handle deletion and COM interface releases to prevent leaks.
* **Two-Row System Tray Layout (Milestones 8, 9, 10)**: Implemented wrapping and width locking for two-row tray configurations using 12x12 icons.
* **Global Replacement Hotkeys & Broadcasts (Milestones 8, 9, 10)**: Intercepts Win+E, Win+R, and Win+D hotkeys in Replace Mode and sends aggressive TaskbarCreated broadcasts to force tray icon reloading.
* **Win32Explorer Options Dialog Elevation Sync (Milestones 8, 9, 10)**: Triggers UAC dialog on Apply in Win32Explorer options dialog by launching `EliteSettings.exe /import` elevated via `"runas"` verb. Silently parses XML configurations and writes them to the HKLM registry, guaranteeing complete elevation parity while allowing the main file manager window to run as standard user.
* **Regression & Audit Fixes (Draftsman-Dan)**: Reverted taskbar button clickable area custom drawing blocks, removed custom button size override, and restored original task switch vertical centering. Renamed custom window classes (`EliteTaskbarPreview` to `TaskbarPreview`, `Elite_SecondaryTrayWnd` to `Shell_SecondaryTrayWnd`, `Elite_StartOrbWnd` to `StartButtonWnd`, and `EliteDesktopSecondary` to `DesktopSecondary`). Implemented a transparent subclass callback for native IShellView to forward WM_ERASEBKGND to Progman. Excluded exit commands from folder band parsing. Assigned EnumWindows callback in verify_final_polish.ps1 to a local variable to prevent GC crash.

</details>

---

## 🏗️ Building the Project

**CRITICAL REQUIREMENT:** The entire build process for **all** `.exe` files in the entire project and all subfolders **MUST BE MANAGED BY** `C:\Users\Administrator\Desktop\Elite-TaskBar\build.ps1` and nothing else! Do not run individual build scripts manually.

Run the `build.ps1` script in the root directory to compile the project. The script acts as the master orchestrator, handling parallel builds, icon injection, code signing, and automatic backups.

Output binaries will be placed flat in the root directory `C:\Users\Administrator\Desktop\Elite-TaskBar` and in the `BuildOutput/` directories.

### Build Automation & Icon Injection Pipeline
The project is powered by an advanced PowerShell build script (`build.ps1`) that orchestrates the entire compilation pipeline using the Visual Studio `cl.exe` MSVC compiler.

1. **Icon Refresh**: The build script dynamically checks the project root for icon files (e.g., `EliteTaskbar.ico`). If found, it injects them directly into the `Resources` folder right before compilation, guaranteeing that the final executables always reflect the latest brand icons.
2. **Resource Compilation**: `rc.exe` builds the `.rc` files into `.res` objects, embedding the icons and manifest into the binary.
3. **Sub-Project Orchestration**: The script securely calls necessary sub-builds (like `Win32Explorer_26.0.3.0\build_Win32Explorer.ps1`) to ensure every component in the suite is built properly alongside the taskbar.
4. **Dual Deployment**: The final binaries are deployed both to the `BuildOutput/` directory and directly to the root project directory for immediate testing.
5. **Auto-Commits**: Upon a successful compilation, the script automatically stages, commits, and pushes the new binaries and source code to the configured Git repository.

### Requirements
- MSBuild / Visual Studio toolchain
- .NET Framework 4.6 SDK
- Windows SDK

---

## 📁 Directory Structure
- `SourceFiles/`: Source code for the taskbar and preferences applet.
- `Remaining_Shell/Win32Explorer_26.0.3.0/`: The Win32Explorer codebase, managed exclusively by the root `build.ps1`.
- `Resources/`: Icons, Start Button Orbs, and visual assets.
- `Documentation/`: Build requirements, checklists, and architectural ledgers.
- `Backups/`: Historical CAB backups of the project state generated automatically before each build.

---

## 📜 Architecture Principles
Elite-Taskbar strictly follows the EliteSoftwareTech Co. development guidelines:
* **No Generic UI**: Avoids modern flat design entirely.
* **DPI Awareness**: Manifests ensure `PerMonitorV2` DPI scaling.
* **Topmost Z-Order**: Binds to the system with `APPBARDATA` to secure its place on the desktop workspace without being obscured by maximized windows.

For an exhaustive breakdown of individual C++ files and their specific responsibilities, see `Documentation/SourceMap_And_Architecture.md`.

---

## 🤖 A Note on "Vibe Coding"
This project was built in close collaboration with Google's Gemini. Many features, design choices, and lines of code were generated or heavily influenced by AI. The goal was less about perfect, clean code and more about exploring the creative potential of human-AI partnership. It's an experiment in letting the "vibe" of an idea guide the development process.

Expect the unexpected. Embrace the chaos. And have fun!

---

## 📸 Media Assets

<div align="center">

<img width="489" height="269" alt="Friday-07-2026-035358-Clip - Copy" src="https://github.com/user-attachments/assets/ba5fb642-4e2b-4942-8b2d-0a8ba6050518" />
<img width="447" height="531" alt="Sunday-07-2026-160914-Clip" src="https://github.com/user-attachments/assets/c9ba74b3-2d8a-4df3-8317-75cfb73d384a" />
<img width="527" height="557" alt="Sunday-07-2026-160924-Clip" src="https://github.com/user-attachments/assets/debb8f12-aa84-41f1-834e-643906d73bc5" />
<img width="458" height="539" alt="Sunday-07-2026-160929-Clip" src="https://github.com/user-attachments/assets/1adb71a5-cd39-4490-be83-1716021a9698" />
<img width="461" height="520" alt="Sunday-07-2026-160934-Clip" src="https://github.com/user-attachments/assets/f75a505f-97cc-4562-bea6-29d33c4a4c8e" />
<img width="448" height="521" alt="Sunday-07-2026-160938-Clip" src="https://github.com/user-attachments/assets/4991dc7f-3bf4-4e2b-b042-c4a59179c4e3" />
<img width="475" height="555" alt="Sunday-07-2026-160945-Clip" src="https://github.com/user-attachments/assets/3b2a0293-2e33-4bfa-8b2c-bf3e3113fea9" />
<img width="606" height="64" alt="Sunday-07-2026-160957-Clip" src="https://github.com/user-attachments/assets/eae336bb-485b-483c-9d17-afb0c471e28e" />
<img width="1922" height="56" alt="Sunday-07-2026-161003-Clip" src="https://github.com/user-attachments/assets/51c1eab6-1192-4d35-b9ab-d26ca812fac2" />
<img width="417" height="375" alt="Sunday-07-2026-161022-Clip" src="https://github.com/user-attachments/assets/8d2d17f6-6c30-47ac-bcae-4a96f2a4bf4f" />
<img width="409" height="240" alt="Sunday-07-2026-161029-Clip" src="https://github.com/user-attachments/assets/ae59fc5b-b42d-4355-8d18-b0b94d60aaef" />

</div>

---
*License: Proprietary - EliteSoftwareTech Co.*
