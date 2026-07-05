# Elite Taskbar Source Map & Architecture

This document serves as a comprehensive guide to the files, architecture, and overall functionality of the Elite Taskbar custom shell extension.

## 🏛️ Overall Architecture
Elite Taskbar is a Win32 C++ application that overlays or replaces the standard Windows Taskbar (`Shell_TrayWnd`). It heavily utilizes the Desktop Window Manager (DWM) API, GDI+, and the UxTheme API to replicate the native Windows 7 "Aero Glass" aesthetic. 

The application is structured into discrete components, each managing a specific visual or functional area of the taskbar:
- **Core Loop**: `main.cpp`
- **Main Shell Overlay**: `TaskbarWindow.cpp` / `TaskbarWindow.h`
- **Start Orb Overlay**: `StartButton.cpp` / `StartButton.h`
- **Utility / Logging**: `Logger.cpp` / `Logger.h`

## 📂 Source Files Map

### 1. `main.cpp`
- **Purpose**: The absolute entry point (`WinMain`) of the application.
- **Functionality**: 
  - Checks for existing instances using a Mutex (`EliteTaskbar_Instance_Mutex`) to prevent duplicate overlays.
  - Initializes COM (Component Object Model) in Single-Threaded Apartment (STA) mode, which is mandatory for shell interactions, clipboard stability, and native dialog summoning.
  - Queries the Windows Registry for the current "Operational Mode" (Replace vs Independent) set by `build.ps1`.
  - Initializes the DWM Buffered Paint API (`BufferedPaintInit()`) for advanced alpha-channel text rendering on glass.
  - Hands off execution to `TaskbarWindow::Initialize` and runs the core message loop (`RunMessageLoop`).
  - Sets up global exception handling (`__try / __except`) for catastrophic crashes.

### 2. `TaskbarWindow.cpp` & `TaskbarWindow.h`
- **Purpose**: The heart of the Elite Taskbar UI. Manages the primary taskbar background, the system tray, the clock, and the show desktop button.
- **Functionality**:
  - **`TaskbarWindow::Initialize`**: Registers standard window classes, queries the native taskbar (`Shell_TrayWnd`) for its coordinates, dynamically sizes the custom taskbar to match, and creates the primary `WS_EX_TOPMOST` overlay. Creates child layout windows (ReBar, SysPager, TrayNotify, TrayClock, TrayShowDesktopButton).
  - **`TaskbarWindowProc`**: 
    - Handles core painting (`DrawThemeBackground`) for the glass surface.
    - Captures right-clicks (`WM_RBUTTONUP`) and spawns the legacy-style context menu for cascading windows, task manager, properties, etc.
    - Handles `WM_COMMAND` to execute the right-click menu commands by routing native undocumented Shell Command IDs (e.g. `410` for Cascade, `401` for Properties) directly to the native `Shell_TrayWnd` via `PostMessageW`. This delegates the heavy lifting to the OS for flawless standard-user execution.
  - **`TrayClockProc`**: Manages the clock drawing. Uses `BeginBufferedPaint` and `DrawThemeTextEx` to draw glowing white text over Aero Glass. Includes a rigorous GDI fallback (`DrawTextW`) if BufferedPaint fails in restricted environments (like standard user accounts injected by Windhawk).
  - **`ShowLegacyClockExperience`**: A complex COM/Window-Hook hack that finds the native (hidden) taskbar's clock and sends synthetic mouse clicks to it in order to natively summon the Windows Calendar flyout.

### 3. `StartButton.cpp` & `StartButton.h`
- **Purpose**: Manages the dynamic, animated Windows 7-style Start Orb.
- **Functionality**:
  - Uses a **Layered Window** (`WS_EX_LAYERED`) completely separate from the main taskbar to allow the orb to physically "overhang" the top of the taskbar onto the desktop workspace.
  - **`StartButton::Initialize` / `Draw`**: Loads the 3-state Start Orb bitmap image using GDI+ (`GdiplusStartup`). Splits the image vertically into Normal, Hover, and Pressed states.
  - Uses `UpdateLayeredWindow` with `BLENDFUNCTION` for per-pixel alpha transparency, ensuring the desktop background bleeds cleanly through the circular edges of the orb.
  - **`OrbWndProc`**: Handles `WM_MOUSEMOVE`, `WM_MOUSELEAVE`, `WM_LBUTTONDOWN`, and `WM_LBUTTONUP` to simulate native hover glow effects.
  - Simulates the `Win` key (VK_LWIN) upon click to natively summon the Start Menu (which perfectly triggers utilities like OpenShell).

### 4. `ClockWidget.cpp` & `ClockWidget.h`
- **Purpose**: Intended for experimental clock implementations or future widget expansions. Currently, the primary clock logic is embedded directly within the `TrayClockProc` inside `TaskbarWindow.cpp` to mimic exact native layouts.

### 5. `Logger.cpp` & `Logger.h`
- **Purpose**: Diagnostic and error tracking utility.
- **Functionality**:
  - Automatically writes debug information to `%SystemDrive%\EliteSoftware\Logs\EliteTaskbar.log`.
  - Used for tracing crashes, tracking fallback mechanics (like when `BufferedPaint` fails), and verifying taskbar lifecycle events.

### 6. `Config.h`
- **Purpose**: Simple global configuration structure (`EliteTaskbarConfig`).
- **Functionality**: Stores enum states (like `TaskbarMode::Independent` vs `TaskbarMode::Replace`) so that all source files can adapt their behavior dynamically.

### 7. `TrayIconScraper.cpp` & `TrayIconScraper.h`
- **Purpose**: Scrapes, maps, and repopulates notification area tray icons.
- **Functionality**:
  - Scrapes notification icons from both the visible taskbar tray (`Shell_TrayWnd`) and hidden overflow tray (`NotifyIconOverflowWindow`) toolbars.
  - Utilizes remote process memory allocation (`VirtualAllocEx`, `ReadProcessMemory`) to copy `TBBUTTON` parameters and text buffers containing tooltip strings.
  - Maintains `g_TrayTooltipsMap` and supplies scraped tooltips to `TaskbarWindow` subclassing handlers.
  - Repopulates our custom taskbar tray toolbar using `UpdateTrayToolbar`.

### 8. `resource.h` & `resources.rc`
- **Purpose**: Windows Resource Compiler definitions.
- **Functionality**: Embeds the application icon (`IDI_MAIN_PROGRAM`) and the raw bitmap for the Start Orb (`IDB_START_ORB`). Also ensures the compiled executable possesses the correct manifest properties.

### 9. `app.manifest`
- **Purpose**: XML application manifest.
- **Functionality**: Explicitly declares OS compatibility (Windows 7/8/10/11) and sets the process to `PerMonitorV2` DPI awareness. This prevents Windows from forcibly blurring or stretching the taskbar on high-resolution or multi-monitor setups.

### 10. `DesktopWindow.cpp` & `DesktopWindow.h`
- **Purpose**: Implements the Desktop Shell Replacement component, providing a custom desktop window (`Progman` / `SHELLDLL_DefView` / `SysListView32`) with icon grid layout and GDI+ wallpaper drawing.
- **Functionality**:
  - Hides native Windows desktop windows (`Progman` and `WorkerW`) to ensure clean co-existence without rendering collisions.
  - Spawns a custom bottom-Z-order `Progman` window spanning the virtual screen dimensions.
  - Spawns a child `SHELLDLL_DefView` window with a `SysListView32` control to render desktop icons.
  - Uses `SHChangeNotifyRegister` to watch file system changes on the desktop folder and update the list view grid dynamically.
  - Implements an optimized desktop wallpaper painter using GDI+. It queries registry wallpaper settings (path, style, tile) and caches the decoded `Gdiplus::Bitmap` to avoid expensive disk operations on every `WM_PAINT` or `WM_ERASEBKGND` message.

## ⚙️ How It Works (The Lifecycle)
1. **Launch**: `build.ps1` compiles the code and launches `EliteTaskbar.exe`.
2. **Boot**: `main.cpp` locks the instance mutex and initializes COM/GDI+/BufferedPaint.
3. **Capture**: `TaskbarWindow::Initialize` tracks down the native `Shell_TrayWnd`. It steals its dimensions and hides it (if in Replace mode).
4. **Overlay**: The custom Aero Glass UI is drawn precisely where the native taskbar was. Child windows for the Start Orb, ReBar, Clock, and Show Desktop Button are snapped to their mathematical coordinates.
5. **Interaction**: 
    - The Start Orb waits for clicks and triggers the Windows key. 
    - The Clock ticks and uses Buffered Paint for native text glow. 
    - Right-clicks are intercepted and routed to a custom Win32 context menu.
6. **Cleanup**: Upon exit, `TaskbarWindow::Cleanup` explicitly restores/shows the native taskbar so the user is never left stranded without a shell.
