
----

<!-- EliteSoftware Co. LOGO -->


<div align="center">

 <a href='https://i.postimg.cc/DZTjtNg1/Elite-Software-Company-Simulator-LOGO.png' target='_blank'><img src='https://i.postimg.cc/DZTjtNg1/Elite-Software-Company-Simulator-LOGO.png' border='0' alt='Elite-Software-Client' width="200"></a> 
 </div>


 ### Core Information 

 * **Company**: EliteSoftware / EliteSoftwareTech Co. 
 * **Authors**: Zachary Whiteman, Susan Gemm, TheShadyRainbow4, EliteSoftwareTech Co.  
 * **Document Version**: 1.2.0.0  
 * **Minimum OS Target**: Windows Vista / Windows 7  


---

 # **?? Elite-Taskbar ??**
# Elite-TaskBar

A master architectural project for a classic Windows-style Taskbar replacement tool.

## Project Scope
Target OS: Windows 10 IoT Enterprise LTSC (Legacy compatible with Vista / 7)
Core Technologies: Native Unmanaged C++ (Taskbar), .NET Framework 4.6 WinForms (Settings Applet) or Native Windows Functions

## Directory Structure
- `SourceFiles/`: Source code for the taskbar and preferences applet.
- `Resources/`: Icons, Start Button Orbs, and visual assets.
- `Documentation/`: Build requirements, checklists, and architectural ledgers.
- `Backups/`: Historical backups of the project state.

## Building the Project
Run the `build.ps1` script in the root directory to compile the project.
Output binaries will be placed flat in the root directory `C:\Users\Administrator\Desktop\Elite-TaskBar`.

### Requirements
- MSBuild / Visual Studio toolchain
- .NET Framework 4.6 SDK
- Windows SDK

## License
Proprietary - EliteSoftwareTech Co.
---

</div>



### EliteSoftwareTech Co. - GUI Guidelines


A dedicated system utility developed by EliteSoftwareTech Co. to perform system 


- **Company**: EliteSoftware / EliteSoftwareTech Co. 
- **Document Version**: 1.2.0.0 
- **Minimum OS Target**: Windows Vista / Windows 7 No newline at end of file 
- **Minimum OS Target**: Windows Vista / Windows 7

---

## **?? A Note on "Vibe Coding"**

This project was built in close collaboration with Google's Gemini. Many features, design choices, and lines of code were generated or heavily influenced by AI. The goal was less about perfect, clean code and more about exploring the creative potential of human-AI partnership. It's an experiment in letting the "vibe" of an idea guide the development process.

Expect the unexpected. Embrace the chaos. And have fun!

---

## **?? Media Assets**

<div align="center">

 <a href='https://i.postimg.cc/DZTjtNg1/Elite-Software-Company-Simulator-LOGO.png' target='_blank'><img src='https://i.postimg.cc/DZTjtNg1/Elite-Software-Company-Simulator-LOGO.png' border='0' alt='Elite-Software-Client' width="250"></a> &nbsp;&nbsp;&nbsp;&nbsp; <a href='https://i.postimg.cc/85mKDbX8/Elite-Software-Company-Logo.png' target='_blank'><img src='https://i.postimg.cc/85mKDbX8/Elite-Software-Company-Logo.png' border='0' alt='Elite-Software-LOGO' width="250"></a> 

</div>

--- 

## **🚀 Elite-Taskbar Deep Dive & Features**

### 🖼️ Core Features & Capabilities
Elite-Taskbar is designed from the ground up to seamlessly blend into the Windows environment, providing a perfect legacy Aero Glass experience while retaining 100% of the functionality expected from the Windows Shell.

*   **Aero Glass Rendering**: Utilizes `DwmEnableBlurBehindWindow` and `DrawThemeBackground` to draw pixel-perfect Windows 7 style glass taskbars.
*   **Orb Overlays**: Features a mathematically perfect Layered Window (`WS_EX_LAYERED`) start button orb that supports 3-state hover animations (Normal, Hover, Pressed) utilizing raw `Gdiplus` for per-pixel alpha.
*   **Intelligent Context Menus**: The taskbar respects the Windows standard context menus. Rather than attempting to "fake" window cascading or tiling, Elite-Taskbar routes raw, undocumented Shell Command IDs directly to the native (hidden) taskbar. This guarantees 100% stable execution of features like:
    *   Cascade Windows (`ID 410`)
    *   Show Windows Stacked (`ID 409`)
    *   Show Windows Side by Side (`ID 408`)
    *   Show Desktop (`ID 407`)
    *   Task Manager (`ID 405`)
    *   Lock Taskbar (`ID 404`)
    *   Properties (`ID 401`)
*   **Robust Tray Clock**: The clock uses the `BeginBufferedPaint` API to draw composited, glowing white text directly onto the Aero Glass frame. It also includes an ironclad fallback mechanism for Standard User environments (or environments hooked by software like Windhawk) falling back to raw GDI `DrawTextW` if the DWM buffer fails, guaranteeing the time is *always* visible.
*   **Native Flyout Integration**: Clicking the clock summons the *actual* Windows Calendar flyout by locating the native `TrayClockWClass` and firing synthetic mouse clicks at it, bridging the gap between the custom shell and native components.

### 🏗️ Build Automation & Icon Injection
The project is powered by an advanced PowerShell build script (`build.ps1`) that orchestrates the entire compilation pipeline using the Visual Studio `cl.exe` MSVC compiler.

1.  **Icon Refresh**: The build script dynamically checks the project root for `EliteTaskbar.ico`. If found, it injects it directly into the `Resources` folder right before compilation, guaranteeing that the final executable always reflects the latest brand icon.
2.  **Resource Compilation**: `rc.exe` builds the `.rc` files into `.res` objects, embedding the icons and manifest into the binary.
3.  **Dual Deployment**: The final `EliteTaskbar.exe` is deployed both to the `BuildOutput/` directory and directly to the root project directory for immediate testing.
4.  **Auto-Commits**: Upon a successful compilation, the script automatically stages, commits, and pushes the new binaries and source code to the configured Git repository.

### 📜 Architecture Principles
Elite-Taskbar strictly follows the EliteSoftwareTech Co. development guidelines:
*   **No Generic UI**: Avoids modern flat design entirely.
*   **DPI Awareness**: Manifests ensure `PerMonitorV2` DPI scaling.
*   **Topmost Z-Order**: Binds to the system with `APPBARDATA` to secure its place on the desktop workspace without being obscured by maximized windows.

For an exhaustive breakdown of individual C++ files and their specific responsibilities, see `Documentation/SourceMap_And_Architecture.md`.
