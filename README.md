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

* **Small Icon Tiles View Mode (R1)**: A brand new view mode in Win32Explorer that displays small icons with right-aligned labels.
* **Group by Type Default (R2)**: A new toggle in the Win32Explorer Options window setting the default grouping logic to Type.
* **Independent Taskbar Mode (R3)**: Taskbar now defaults to co-existing with the native explorer taskbar rather than attempting to replace it on first launch.
* **UWP App Icons Fix (R5)**: Accurate rendering of UWP app icons on the taskbar.
* **High-DPI Text Blurriness Fix (R7)**: Font crispness is preserved across mismatched high-DPI monitor configurations.
* **Settings Import/Export**: Backup and restore all shell configurations, native settings, and Elite-Taskbar behaviors directly into a single `.reg` file.
* **Aggressive Restart**: Immediate and full application restart upon applying settings to prevent hidden UI glitches or the "start orb top-left" bug.
* **Desktop Replacement (Phase XI)**: Native `Progman` replacement window with subclassed `SHELLDLL_DefView` and `SysListView32` icon grid. Features high-quality GDI+ wallpaper drawing with smart bitmap caching to avoid redundant disk reads on paint. Supports double-click file execution, inline renaming, and folder watching via `SHChangeNotifyRegister` with a 100ms debounce timer.
* **Fallback Start Menu (Phase XIX)**: Automatic Open-Shell integration that launches the Open-Shell Start Menu (`StartMenu.exe`) from standard, relative, or WinPE directories when clicking the Start button.
* **About Dialog Layout Spacing Optimization**: Re-engineered dialog layouts to eliminate dead space, bringing collapsed height to 110 DUs and expanded to 195 DUs with dynamic button relocation.
* **Property Sheet Sarcastic Tooltips**: Injected custom subclassing logic to dynamically set witty and sarcastic tooltips on standard Property Sheet buttons (Okay, Cancel, Apply).
* **Settings Apply DLL Module Path Resolution**: Patched the folder fallback logic inside the DLL to use `g_hInstance` instead of `NULL` to avoid launching from the System32 directory.
* **DDF Compression Pipeline Acceleration**: Restructured the backup pipeline to ignore locked log/txt files and scan only active code folders, accelerating backups from a hang to under a second.

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

<a href='https://github.com/TheShadyRainbow4/Elite-TaskBar/raw/refs/heads/master/Screenshots/Friday-07-2026-040652-Clip.BMP' target='_blank'>
  <img src='https://github.com/TheShadyRainbow4/Elite-TaskBar/raw/refs/heads/master/Screenshots/Friday-07-2026-040652-Clip.BMP' border='0' alt='Elite-Software-LOGO' width="300">
</a>
<a href='https://github.com/TheShadyRainbow4/Elite-TaskBar/raw/refs/heads/master/Screenshots/Friday-07-2026-035358-Clip.BMP' target='_blank'>
  <img src='https://github.com/TheShadyRainbow4/Elite-TaskBar/raw/refs/heads/master/Screenshots/Friday-07-2026-035358-Clip.BMP' border='0' alt='Elite-Software-Client' width="300">
</a>
<a href='https://github.com/TheShadyRainbow4/Elite-TaskBar/raw/refs/heads/master/Screenshots/Friday-07-2026-030438-Clip.BMP' target='_blank'>
  <img src='https://github.com/TheShadyRainbow4/Elite-TaskBar/raw/refs/heads/master/Screenshots/Friday-07-2026-030438-Clip.BMP' border='0' alt='Elite-Software-Client' width="300">
</a>

</div>

---
*License: Proprietary - EliteSoftwareTech Co.*
