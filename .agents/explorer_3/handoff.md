# Handoff Report - Milestone 7 Desktop Background and Settings Mirroring

This handoff report outlines the findings and implementation strategy for the Desktop Background property page events, initialization, saving/loading settings, Span/Per-monitor modes, theme path selection, wallpaper previews, slideshow mode, theme tutorial controls, and ensuring perfect mirroring in CPL and settings EXE.

## 1. Observation

### File Locations and Analysed Paths:
- **Properties Dialog Logic**: 
  - `SourceFiles/TaskbarProperties.cpp` (Lines 1436-1495 contains `DesktopSettingsDlgProc`)
  - `Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/TaskbarProperties.cpp` (Duplicate copy in the Win32Explorer submodule)
- **Desktop Replacement Engine**: 
  - `SourceFiles/DesktopWindow.cpp` (Lines 424-583 contains the `DrawWallpaper` GDI+ rendering code)
- **Dialog Template Definitions**:
  - `SourceFiles/resources.rc` (Lines 89-98 contains `IDD_DESKTOP_PROPS`)
  - `Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/resources.rc` (Duplicate copy in the Win32Explorer submodule)
- **Resource Identifiers**:
  - `SourceFiles/resource.h` (Lines 84-89 contains desktop-related definitions)
  - `Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/resource.h` (Duplicate copy in the Win32Explorer submodule)
- **Compilation Orchestration**:
  - `build_settings.ps1` (Line 38 builds `EliteSettings.exe` using `TaskbarProperties.cpp`)
  - `SourceFiles/EliteSettingsCpl.cpp` (Extracts and executes embedded `EliteSettings.exe` resource)

### Exact Code References:
In `SourceFiles/resources.rc`, the `IDD_DESKTOP_PROPS` is currently defined as:
```rc
IDD_DESKTOP_PROPS DIALOGEX 0, 0, 252, 218
STYLE DS_SETFONT | WS_CHILD | WS_DISABLED | WS_CAPTION | DS_CONTROL
CAPTION "Desktop"
FONT 8, "Segoe UI Semibold", 600, 0, 0x1
BEGIN
    GROUPBOX        "Desktop Replacement Settings", IDC_STATIC, 7, 7, 238, 70
    CONTROL         "Enable custom desktop window replacement", IDC_DESKTOP_REPLACE_ENABLED, "Button", BS_AUTOCHECKBOX | WS_TABSTOP, 14, 20, 220, 10
    CONTROL         "Draw desktop wallpaper background", IDC_DESKTOP_WALLPAPER_ENABLED, "Button", BS_AUTOCHECKBOX | WS_TABSTOP, 14, 35, 220, 10
    CONTROL         "Show desktop icon grid (SysListView32)", IDC_DESKTOP_ICONS_ENABLED, "Button", BS_AUTOCHECKBOX | WS_TABSTOP, 14, 50, 220, 10
END
```

In `SourceFiles/TaskbarProperties.cpp`, `DesktopSettingsDlgProc` is currently defined as:
```cpp
INT_PTR CALLBACK DesktopSettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG: {
        EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        AddDlgTooltip(hwndDlg, IDC_DESKTOP_REPLACE_ENABLED, L"Enable custom desktop replacement window. Pure classic shell experience.");
        AddDlgTooltip(hwndDlg, IDC_DESKTOP_WALLPAPER_ENABLED, L"Render desktop background wallpaper using custom styles.");
        AddDlgTooltip(hwndDlg, IDC_DESKTOP_ICONS_ENABLED, L"Display folder items and files on the desktop grid.");
        
        HKEY hKey;
        DWORD replaceVal = 1;
        DWORD wallpaperVal = 1;
        DWORD iconsVal = 1;
        DWORD cbData = sizeof(DWORD);
        
        if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            cbData = sizeof(DWORD);
            RegQueryValueExW(hKey, L"DesktopReplacementEnabled", NULL, NULL, (LPBYTE)&replaceVal, &cbData);
            cbData = sizeof(DWORD);
            RegQueryValueExW(hKey, L"DesktopWallpaperEnabled", NULL, NULL, (LPBYTE)&wallpaperVal, &cbData);
            cbData = sizeof(DWORD);
            RegQueryValueExW(hKey, L"DesktopIconsEnabled", NULL, NULL, (LPBYTE)&iconsVal, &cbData);
            RegCloseKey(hKey);
        }
        
        SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_REPLACE_ENABLED, BM_SETCHECK, replaceVal ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_WALLPAPER_ENABLED, BM_SETCHECK, wallpaperVal ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_ICONS_ENABLED, BM_SETCHECK, iconsVal ? BST_CHECKED : BST_UNCHECKED, 0);
        return TRUE;
    }
...
```

In `SourceFiles/DesktopWindow.cpp`, the wallpaper drawing routine gets system parameters or custom registry variables:
```cpp
void DrawWallpaper(HDC hdc, int scrW, int scrH) {
    HKEY hKey;
    DWORD drawWallpaper = 1;
    DWORD cbData = sizeof(DWORD);
    if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExW(hKey, L"DesktopWallpaperEnabled", NULL, NULL, (LPBYTE)&drawWallpaper, &cbData);
        RegCloseKey(hKey);
    }
    // ... Loads HKCU "Control Panel\\Desktop" wallpaper settings and renders via GDI+ DrawImage or TextureBrush
```

## 2. Logic Chain

The architecture of Elite-Taskbar relies on standard Win32 components with a custom settings applet. To implement the new desktop customization features while maintaining parity across standard user environments, settings applets, and the custom explorer shell, we trace the following logic:

1. **Perfect Settings Parity & Mirroring**:
   - `build_settings.ps1` copies `TaskbarProperties.cpp` and compiles it directly alongside `EliteSettingsStub.cpp` to produce `EliteSettings.exe`.
   - The standalone CPL (`EliteSettings.cpl`) is constructed by embedding `EliteSettings.exe` as resource ID 1 (`RT_RCDATA`) inside `settings_cpl.rc` and executing it when double clicked.
   - Therefore, any modifications made to `TaskbarProperties.cpp` and `resources.rc` will automatically apply and mirror perfectly in the standalone settings executable and CPL upon running `build.ps1`.
   - However, since `Win32Explorer` has its own copy of `resource.h`, `resources.rc`, and `TaskbarProperties.cpp`, these files must be modified identically to prevent compilation divergence in the `Win32Explorer` submodule.

2. **Dialog Template Refactoring (`IDD_DESKTOP_PROPS`)**:
   - The "Desktop" tab in the property sheet needs to be redesigned to accommodate the new configuration options:
     - Group 1: Desktop Replacement Settings (existing checkboxes).
     - Group 2: Themes & Personalization (combobox for themes, tutorial SysLink, preview icons).
     - Group 3: Wallpaper Configuration (Wallpaper mode, wallpaper style, slideshow checkboxes/timings, wallpaper thumbnail).
   - This layout fits within the `252 x 218` dialog unit bounds. Since high-density visual components are requested, standard win32 controls (Static, ComboBox, SysLink, CheckBox, Edit, Picture) will be defined with Segoe UI Semibold.

3. **Registry Variables and Key Mapping**:
   - Settings must be read and written from `GetEliteRegistryRoot()` (under `Software\\EliteSoftware\\Win32Explorer\\Advanced`) to ensure they respect the portable mirror mode (HKLM vs HKCU).
   - Key-value pairs to store:
     - `DesktopWallpaperMode` (DWORD: 0 = Span, 1 = Per-Monitor)
     - `DesktopThemePath` (REG_SZ: path to the selected `.theme` file)
     - `DesktopSlideshowEnabled` (DWORD: 0 = Disabled, 1 = Enabled)
     - `DesktopSlideshowInterval` (DWORD: cycle interval in milliseconds)
     - `DesktopSlideshowPath` (REG_SZ: directory path containing slideshow images)
     - `MonitorWallpaper_0`, `MonitorWallpaper_1`, etc. (REG_SZ: per-monitor wallpaper images)

4. **Span/Per-monitor Modes in GDI+ Wallpaper Renderer**:
   - Span Mode: The single desktop replacement window covers the full virtual screen coordinates. The single wallpaper is loaded, scaled, and drawn on the virtual screen dimensions (`scrW, scrH`).
   - Per-Monitor Mode: The wallpaper rendering logic must partition the client coordinates using `EnumDisplayMonitors`. For each monitor, the renderer determines its relative bounding box and draws its designated wallpaper image (`MonitorWallpaper_<index>`) scaled according to the specified layout.

5. **Slideshow Mode Timing Mechanism**:
   - The desktop background window (`s_hProgman`) can run a standard Win32 timer (`SetTimer`) with the configured interval (e.g. `DesktopSlideshowInterval` which allows 3 seconds upwards).
   - When the timer triggers `WM_TIMER`, the system updates the in-memory wallpaper path index to the next file in the slideshow folder and calls `InvalidateRect` on the desktop replacement window to schedule a repaint.

6. **Theme Detection, Tutorial, and Previews**:
   - **Theme Detection**: In `WM_INITDIALOG`, the page uses `FindFirstFileW`/`FindNextFileW` to scan `.theme` files inside `C:\Windows\Resources\Themes` and `%LocalAppData%\Microsoft\Windows\Themes`. The found themes populate `IDC_DESKTOP_THEME_SELECT`.
   - **Tutorial SysLink**: A `SysLink` control containing `<a href="https://elitesoftwaretech.com/themes-tutorial">How to install themes?</a>` intercepts `NM_CLICK` in the parent dialog and invokes `ShellExecuteW` targeting default web browsers.
   - **Icon Previews**: Parse the `.theme` file (which uses the INI format). Extract `DefaultIcon` paths for Computer, User Files, Recycle Bin, and Network. Load these using `ExtractIconExW` / `PrivateExtractIconsW` and render them in the property sheet tab preview using static image controls.
   - **Wallpaper Thumbnail Preview**: Subclass a static control in the properties dialog. Load the selected wallpaper using `Gdiplus::Bitmap` and paint a scaled down thumbnail in the control's client area during `WM_PAINT`.

## 3. Caveats

- **Slideshow Directory Scan Overhead**: Scanning large folders containing thousands of images can block the main UI thread. It is highly recommended to perform directory enumeration in a background thread or cache the image paths list upon selection.
- **Multiple Monitors Bounds Handling**: When active monitors are dynamically added, removed, or their resolutions change (`WM_DISPLAYCHANGE`), the per-monitor coordinates must be updated. This is handled by having the desktop window monitor bounds queried dynamically via `EnumDisplayMonitors` on paint events.
- **GDI+ Resource Leaks**: Loading and unloading high-resolution wallpaper images for previews and desktop painting can lead to memory exhaustion if bitmap handles are not properly destroyed. Custom static control subclasses and wallpaper renderers must explicitly delete previous `Gdiplus::Bitmap` instances.

## 4. Conclusion

The recommended strategy is to implement the desktop personalization features by updating the dialog layout, properties event handler, and desktop rendering code in coordinate sync. Because `build_settings.ps1` integrates `TaskbarProperties.cpp` into both settings targets, the settings EXE and CPL will automatically mirror the functionality.

### Recommended Code Implementation Details:

#### A. New Resource Control Identifiers (`SourceFiles/resource.h`)
```cpp
#define IDC_DESKTOP_WALLPAPER_MODE     296
#define IDC_DESKTOP_THEME_SELECT        297
#define IDC_DESKTOP_THEME_TUTORIAL      298
#define IDC_DESKTOP_SLIDESHOW_ENABLED   299
#define IDC_DESKTOP_SLIDESHOW_INTERVAL  300
#define IDC_DESKTOP_WALLPAPER_PREVIEW   301
#define IDC_ICON_PREVIEW_COMPUTER       302
#define IDC_ICON_PREVIEW_USER           303
#define IDC_ICON_PREVIEW_RECYCLE        304
#define IDC_ICON_PREVIEW_NETWORK        305
```

#### B. Redesigned Dialog Template (`SourceFiles/resources.rc`)
```rc
IDD_DESKTOP_PROPS DIALOGEX 0, 0, 252, 218
STYLE DS_SETFONT | WS_CHILD | WS_DISABLED | WS_CAPTION | DS_CONTROL
CAPTION "Desktop"
FONT 8, "Segoe UI Semibold", 600, 0, 0x1
BEGIN
    GROUPBOX        "Desktop Replacement Settings", IDC_STATIC, 7, 7, 238, 52
    CONTROL         "Enable custom desktop replacement", IDC_DESKTOP_REPLACE_ENABLED, "Button", BS_AUTOCHECKBOX | WS_TABSTOP, 14, 18, 220, 10
    CONTROL         "Draw desktop wallpaper background", IDC_DESKTOP_WALLPAPER_ENABLED, "Button", BS_AUTOCHECKBOX | WS_TABSTOP, 14, 30, 220, 10
    CONTROL         "Show desktop icon grid (SysListView32)", IDC_DESKTOP_ICONS_ENABLED, "Button", BS_AUTOCHECKBOX | WS_TABSTOP, 14, 42, 220, 10

    GROUPBOX        "Themes & Personalization", IDC_STATIC, 7, 62, 238, 55
    LTEXT           "Select Theme:", IDC_STATIC, 14, 74, 50, 8
    COMBOBOX        IDC_DESKTOP_THEME_SELECT, 68, 72, 166, 100, CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP
    CONTROL         "<a>Learn how to customize themes</a>", IDC_DESKTOP_THEME_TUTORIAL, "SysLink", WS_TABSTOP, 14, 90, 120, 10
    CONTROL         "", IDC_ICON_PREVIEW_COMPUTER, "Static", SS_ICON | SS_CENTERIMAGE | WS_BORDER, 145, 88, 18, 18
    CONTROL         "", IDC_ICON_PREVIEW_USER, "Static", SS_ICON | SS_CENTERIMAGE | WS_BORDER, 168, 88, 18, 18
    CONTROL         "", IDC_ICON_PREVIEW_NETWORK, "Static", SS_ICON | SS_CENTERIMAGE | WS_BORDER, 191, 88, 18, 18
    CONTROL         "", IDC_ICON_PREVIEW_RECYCLE, "Static", SS_ICON | SS_CENTERIMAGE | WS_BORDER, 214, 88, 18, 18

    GROUPBOX        "Wallpaper Configuration", IDC_STATIC, 7, 120, 238, 91
    LTEXT           "Layout Mode:", IDC_STATIC, 14, 132, 50, 8
    COMBOBOX        IDC_DESKTOP_WALLPAPER_MODE, 68, 130, 80, 100, CBS_DROPDOWNLIST | WS_TABSTOP
    CONTROL         "Slideshow Mode", IDC_DESKTOP_SLIDESHOW_ENABLED, "Button", BS_AUTOCHECKBOX | WS_TABSTOP, 14, 148, 80, 10
    LTEXT           "Interval (sec):", IDC_STATIC, 100, 149, 50, 8
    EDITTEXT        IDC_DESKTOP_SLIDESHOW_INTERVAL, 150, 147, 30, 12, ES_NUMBER
    CONTROL         "", IDC_DESKTOP_WALLPAPER_PREVIEW, "Static", SS_OWNERDRAW | WS_BORDER, 14, 165, 224, 40
END
```

#### C. Subclassing and Witty Tooltip Bindings (`SourceFiles/TaskbarProperties.cpp`)
In `DesktopSettingsDlgProc` under `WM_INITDIALOG`:
```cpp
AddDlgTooltip(hwndDlg, IDC_DESKTOP_WALLPAPER_MODE, L"Decide whether to stretch a single wallpaper till it screams, or display different wallpapers on each screen.");
AddDlgTooltip(hwndDlg, IDC_DESKTOP_THEME_SELECT, L"Select from detected Windows themes. Give your desktop a quick makeover.");
AddDlgTooltip(hwndDlg, IDC_DESKTOP_THEME_TUTORIAL, L"Learn how to craft themes. Warning: may lead to retro design obsessions.");
AddDlgTooltip(hwndDlg, IDC_DESKTOP_SLIDESHOW_ENABLED, L"Periodically flip wallpapers so you don't get bored staring at the same pixel layout.");
AddDlgTooltip(hwndDlg, IDC_DESKTOP_SLIDESHOW_INTERVAL, L"Slideshow cycle frequency. Timings start at 3 seconds, if you like strobe light wallpapers.");
AddDlgTooltip(hwndDlg, IDC_DESKTOP_WALLPAPER_PREVIEW, L"Your tiny, non-interactive window to the aesthetic soul of your machine.");
```

#### D. Theme Enumeration Logic in properties dialog:
```cpp
void PopulateThemesComboBox(HWND hCombo) {
    SendMessageW(hCombo, CB_RESETCONTENT, 0, 0);
    
    std::vector<std::wstring> searchPaths = {
        L"C:\\Windows\\Resources\\Themes",
        L"" // Placeholder for LocalAppDataThemes
    };
    
    // Resolve %LocalAppData%\Microsoft\Windows\Themes
    wchar_t localPath[MAX_PATH];
    if (GetEnvironmentVariableW(L"USERPROFILE", localPath, MAX_PATH) > 0) {
        searchPaths[1] = std::wstring(localPath) + L"\\AppData\\Local\\Microsoft\\Windows\\Themes";
    }

    for (const auto& path : searchPaths) {
        if (path.empty() || !PathFileExistsW(path.c_str())) continue;
        std::wstring query = path + L"\\*.theme";
        WIN32_FIND_DATAW fd;
        HANDLE hFind = FindFirstFileW(query.c_str(), &fd);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                std::wstring themeName = fd.cFileName;
                // Strip extension for UI presentation
                size_t dot = themeName.find_last_of(L'.');
                if (dot != std::wstring::npos) themeName = themeName.substr(0, dot);
                SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)themeName.c_str());
            } while (FindNextFileW(hFind, &fd));
            FindClose(hFind);
        }
    }
}
```

## 5. Verification Method

To verify the successful analysis and implementation plan:

1. **Compilation Check**:
   Run the project's build command from the powershell console to compile both x86 and x64 variants along with stubs:
   ```powershell
   & "C:\Users\Administrator\Desktop\Elite-TaskBar\build.ps1"
   ```
   Ensure the output binaries `BuildOutput\EliteTaskbar.exe`, `BuildOutput\EliteSettings.exe` and `BuildOutput\EliteSettings.cpl` generate without errors.

2. **Settings Synchronisation Checks**:
   - Launch the properties sheet using `EliteTaskbar.exe /settings` or double-click the `EliteSettings.cpl` control panel applet.
   - Verify that changes on the "Desktop" page controls accurately write to registry keys under `HKCU\Software\EliteSoftware\Win32Explorer\Advanced`.
   - Verify that clicking "Apply" successfully notifies and restarts the shell instances, cleanly loading the new wallpaper settings.
