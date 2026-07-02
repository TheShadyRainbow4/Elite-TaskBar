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
