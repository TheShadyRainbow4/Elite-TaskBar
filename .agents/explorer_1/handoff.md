# Handoff Report: Custom Desktop (Progman) Rendering Strategy

## 1. Observation

A detailed review of `SourceFiles/DesktopWindow.cpp`, `SourceFiles/DesktopWindow.h`, `SourceFiles/TaskbarWindow.cpp`, and related configuration structures reveals the following:

### Custom Progman Creation & Virtual Screen Geometry
- In `SourceFiles/DesktopWindow.cpp` (lines 84-106), the custom `Progman` window is created as a single top-level `WS_POPUP` window spanning the entire virtual desktop area:
  ```cpp
  // Get full virtual screen dimensions to span all monitors
  int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
  int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
  int cx = GetSystemMetrics(SM_CXVIRTUALSCREEN);
  int cy = GetSystemMetrics(SM_CYVIRTUALSCREEN);

  s_hProgman = CreateWindowExW(
      WS_EX_TOOLWINDOW,
      L"Progman",
      L"Program Manager",
      WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
      x, y, cx, cy,
      NULL, NULL, hInst, NULL
  );
  ```
- In `ProgmanWndProc` (lines 170-175), the window is forced to the bottom of the Z-order:
  ```cpp
  case WM_WINDOWPOSCHANGING: {
      WINDOWPOS* lpw = (WINDOWPOS*)lParam;
      lpw->hwndInsertAfter = HWND_BOTTOM;
      lpw->flags &= ~SWP_NOZORDER;
      break;
  }
  ```

### Hiding and Restoring Native Desktop Shell Windows
- In `DesktopWindow::Initialize` (lines 39-54), native shell windows `Progman` and the specific `WorkerW` hosting the `SHELLDLL_DefView` child window are hidden via `ShowWindow(..., SW_HIDE)`:
  ```cpp
  // Coexistence: find and hide native desktop windows
  s_hNativeProgman = FindWindowW(L"Progman", NULL);
  if (s_hNativeProgman) {
      Logger::Log(L"Native Progman window detected. Hiding.");
      ShowWindow(s_hNativeProgman, SW_HIDE);
  }
  
  HWND hWorker = NULL;
  while ((hWorker = FindWindowExW(NULL, hWorker, L"WorkerW", NULL)) != NULL) {
      HWND hDefView = FindWindowExW(hWorker, NULL, L"SHELLDLL_DefView", NULL);
      if (hDefView) {
          Logger::Log(L"Native WorkerW desktop window detected. Hiding.");
          s_hNativeWorkerW = hWorker;
          ShowWindow(hWorker, SW_HIDE);
      }
  }
  ```
- In `DesktopWindow::Cleanup` (lines 131-140), these hidden native desktop windows are restored:
  ```cpp
  // Restore native desktop windows if hidden
  if (s_hNativeProgman && IsWindow(s_hNativeProgman)) {
      Logger::Log(L"Restoring native Progman window.");
      ShowWindow(s_hNativeProgman, SW_SHOW);
  }
  if (s_hNativeWorkerW && IsWindow(s_hNativeWorkerW)) {
      Logger::Log(L"Restoring native WorkerW window.");
      ShowWindow(s_hNativeWorkerW, SW_SHOW);
  }
  ```

### Desktop Replacement Conditional Activation
- In `SourceFiles/TaskbarWindow.cpp` (lines 2953-2955), `DesktopWindow::Initialize()` is only executed when `Replace` mode is active:
  ```cpp
  if (g_Config.Mode == TaskbarMode::Replace && desktopReplaceEnabled) {
      DesktopWindow::Initialize();
  }
  ```
- When in `Replace` mode, the native taskbar is hidden (lines 2619-2626), whereas in `Independent` mode, the native taskbar remains visible and unmodified.

### Wallpaper Rendering
- Wallpaper configuration is queried from registry keys (lines 438-454):
  - Path: `HKCU\Control Panel\Desktop\Wallpaper`
  - Style: `HKCU\Control Panel\Desktop\WallpaperStyle`
  - Tile: `HKCU\Control Panel\Desktop\TileWallpaper`
- The wallpaper drawing function `DrawWallpaper` (lines 424-583) loads a single cached `Gdiplus::Bitmap` and renders it across the client coordinates of the custom desktop window. Under current styling options (Stretch, Center, Fit, Fill, Span), the image is scaled relative to the *entire virtual screen client bounds* `(scrW, scrH)` as a single viewport rather than per monitor.

---

## 2. Logic Chain

1. **Enabling Custom Desktop with Active Native Taskbar**:
   - Currently, if the operational mode is `Independent` (keeping the native taskbar visible), `DesktopWindow::Initialize()` is bypassed entirely.
   - By introducing a registry setting `ForceProgmanAllDisplays` (under `HKCU\Software\EliteSoftware\Win32Explorer\Advanced`), we can bypass the `TaskbarMode::Replace` check.
   - If `ForceProgmanAllDisplays` is set to `1` (and `desktopReplaceEnabled` is true), we will invoke `DesktopWindow::Initialize()` even in `Independent` mode.
   - Since the custom `Progman` window intercepts `WM_WINDOWPOSCHANGING` and forces its Z-order to `HWND_BOTTOM`, it will render underneath the native taskbar (`Shell_TrayWnd`) and any custom taskbars, allowing them to cleanly co-exist.

2. **Multimonitor Layout Integrity (Display Change)**:
   - The custom `Progman` window is created with virtual screen coordinates captured at initialization. However, it does not handle layout adjustments when displays are connected, disconnected, or resized.
   - To fix this, a handler for `WM_DISPLAYCHANGE` must be added to `ProgmanWndProc`. Upon receiving this message, the window bounds should be recalculated using `GetSystemMetrics` for `SM_*VIRTUALSCREEN` and resized via `SetWindowPos`.

3. **Per-Monitor Wallpaper Rendering**:
   - Spanning a single wallpaper stretch/fill across multiple monitors of varying dimensions and aspect ratios degrades aesthetics.
   - Adding a multimonitor mode toggle `DesktopWallpaperMultimonMode` (0 = Span, 1 = Per-Monitor) will determine drawing behavior.
   - Under Per-Monitor mode, we can use `EnumDisplayMonitors` within `DrawWallpaper` to partition the destination DC. A callback `DrawWallpaperMonitorProc` will retrieve the rectangle coordinates of each active monitor, map them to client coordinates, and scale the wallpaper individually to fit each screen bounds.
   - To support different wallpapers per monitor (standard Windows 10/11 behavior), we can instantiate the COM interface `IDesktopWallpaper` (CLSID `CLSID_DesktopWallpaper`, IID `IID_IDesktopWallpaper`) to query the specific image file path and position layout configuration for each display.

---

## 3. Caveats

- **Native Start Menu / Flyout Overlay Conflicts**: When both the native taskbar and the custom desktop replacement are active, clicking the native Start menu or notification flyouts may cause transient Z-order anomalies. The custom `Progman` must strictly maintain `HWND_BOTTOM` to avoid visual overlap.
- **Double-Click Desktop Area Actions**: If native shell handlers listen for desktop clicks, they might fail to capture events on our custom `Progman` window. Subclassing and click forwarding handles this.
- **DPI Scaling Mismatch**: Displays with different DPI scaling settings can cause list view icon grids (`SysListView32`) to scale incorrectly or align off-screen if left-aligned. The list view's margins and sizes should be dynamically adjusted using standard DPI metrics.

---

## 4. Conclusion

To implement the requested features, the following development plan is recommended:

### Code Snippets / Proposed Changes

#### A. Supporting `ForceProgmanAllDisplays` in `SourceFiles/TaskbarWindow.cpp`
Modify the initialization checks to query and respect `ForceProgmanAllDisplays` when evaluating whether to launch `DesktopWindow`:

```cpp
// In TaskbarWindow.cpp (around line 2953)
bool forceProgman = false;
HKEY hKeyForce;
if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKeyForce) == ERROR_SUCCESS) {
    DWORD dwVal = 0;
    DWORD cbData = sizeof(DWORD);
    if (RegQueryValueExW(hKeyForce, L"ForceProgmanAllDisplays", NULL, NULL, (LPBYTE)&dwVal, &cbData) == ERROR_SUCCESS) {
        forceProgman = (dwVal == 1);
    }
    RegCloseKey(hKeyForce);
}

if ((g_Config.Mode == TaskbarMode::Replace || forceProgman) && desktopReplaceEnabled) {
    DesktopWindow::Initialize();
}
```

#### B. Handle `WM_DISPLAYCHANGE` in `SourceFiles/DesktopWindow.cpp`
Ensure the custom desktop window resizes dynamically when display settings change:

```cpp
// In DesktopWindow.cpp -> ProgmanWndProc
case WM_DISPLAYCHANGE: {
    int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int cx = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int cy = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    SetWindowPos(hwnd, HWND_BOTTOM, x, y, cx, cy, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    return 0;
}
```

#### C. Multimonitor Per-Display Wallpaper Drawing Strategy
Integrate `EnumDisplayMonitors` and optionally `IDesktopWallpaper` to render wallpapers relative to each display's bounds. Below is the proposed layout structure:

```cpp
struct MonitorWallpaperData {
    HDC hdc;
    Gdiplus::Bitmap* pBitmap;
    int style;
    bool tile;
    COLORREF bgColor;
};

// Callback function to draw wallpaper per display
BOOL CALLBACK DrawWallpaperMonitorProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    MonitorWallpaperData* pData = (MonitorWallpaperData*)dwData;
    
    // Convert virtual monitor rect coordinates to client coordinates of s_hProgman
    int virtualX = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int virtualY = GetSystemMetrics(SM_YVIRTUALSCREEN);
    
    int monLeft = lprcMonitor->left - virtualX;
    int monTop = lprcMonitor->top - virtualY;
    int monWidth = lprcMonitor->right - lprcMonitor->left;
    int monHeight = lprcMonitor->bottom - lprcMonitor->top;
    
    Gdiplus::Graphics graphics(pData->hdc);
    graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
    
    if (pData->tile) {
        Gdiplus::TextureBrush brush(pData->pBitmap);
        brush.SetWrapMode(Gdiplus::WrapModeTile);
        brush.TranslateTransform((Gdiplus::REAL)monLeft, (Gdiplus::REAL)monTop);
        graphics.FillRectangle(&brush, monLeft, monTop, monWidth, monHeight);
    } else {
        int imgW = pData->pBitmap->GetWidth();
        int imgH = pData->pBitmap->GetHeight();
        
        int destX = monLeft, destY = monTop, destW = monWidth, destH = monHeight;
        int srcX = 0, srcY = 0, srcW = imgW, srcH = imgH;
        
        double imgAspect = (double)imgW / imgH;
        double scrAspect = (double)monWidth / monHeight;
        
        switch (pData->style) {
            case 0: // Center
                if (imgW <= monWidth) {
                    destX = monLeft + (monWidth - imgW) / 2;
                    destW = imgW;
                } else {
                    destX = monLeft;
                    destW = monWidth;
                    srcX = (imgW - monWidth) / 2;
                    srcW = monWidth;
                }
                if (imgH <= monHeight) {
                    destY = monTop + (monHeight - imgH) / 2;
                    destH = imgH;
                } else {
                    destY = monTop;
                    destH = monHeight;
                    srcY = (imgH - monHeight) / 2;
                    srcH = monHeight;
                }
                {
                    Gdiplus::SolidBrush bgBrush(Gdiplus::Color(GetRValue(pData->bgColor), GetGValue(pData->bgColor), GetBValue(pData->bgColor)));
                    graphics.FillRectangle(&bgBrush, monLeft, monTop, monWidth, monHeight);
                }
                break;
                
            case 2: // Stretch
                destX = monLeft; destY = monTop;
                destW = monWidth; destH = monHeight;
                break;
                
            case 6: // Fit
                if (imgAspect > scrAspect) {
                    destW = monWidth;
                    destH = (int)(monWidth / imgAspect);
                    destX = monLeft;
                    destY = monTop + (monHeight - destH) / 2;
                } else {
                    destH = monHeight;
                    destW = (int)(monHeight * imgAspect);
                    destY = monTop;
                    destX = monLeft + (monWidth - destW) / 2;
                }
                {
                    Gdiplus::SolidBrush bgBrush(Gdiplus::Color(GetRValue(pData->bgColor), GetGValue(pData->bgColor), GetBValue(pData->bgColor)));
                    graphics.FillRectangle(&bgBrush, monLeft, monTop, monWidth, monHeight);
                }
                break;
                
            case 10: // Fill
            default:
                if (imgAspect > scrAspect) {
                    destH = monHeight;
                    destW = (int)(monHeight * imgAspect);
                    destY = monTop;
                    destX = monLeft + (monWidth - destW) / 2;
                } else {
                    destW = monWidth;
                    destH = (int)(monWidth / imgAspect);
                    destX = monLeft;
                    destY = monTop + (monHeight - destH) / 2;
                }
                break;
        }
        
        graphics.DrawImage(pData->pBitmap, 
            Gdiplus::Rect(destX, destY, destW, destH),
            srcX, srcY, srcW, srcH, 
            Gdiplus::UnitPixel);
    }
    
    return TRUE;
}
```

Replace the single image rendering block in `DrawWallpaper` with:

```cpp
    bool perMonitorMode = true; // Read from registry/config: "DesktopWallpaperMultimonMode" (0 = Span, 1 = PerMonitor)
    
    if (perMonitorMode) {
        MonitorWallpaperData data;
        data.hdc = hdc;
        data.pBitmap = s_pCachedWallpaper;
        data.style = s_cachedStyle;
        data.tile = s_cachedTile;
        data.bgColor = GetSysColor(COLOR_BACKGROUND);
        
        EnumDisplayMonitors(NULL, NULL, DrawWallpaperMonitorProc, (LPARAM)&data);
    } else {
        // Fall back to original single-screen spanning logic...
    }
```

#### D. Settings Dialog Controls & Registry Hooking (`TaskbarProperties.cpp` and `resources.rc`)
- Register a new checkbox under the "Desktop" page template in `resources.rc`:
  ```rc
  CONTROL "Force custom desktop on all connected displays (Independent Mode)", IDC_DESKTOP_FORCE_ALL_DISPLAYS, "Button", BS_AUTOCHECKBOX | WS_TABSTOP, 14, 65, 220, 10
  ```
- Define `IDC_DESKTOP_FORCE_ALL_DISPLAYS` as `295` in `resource.h`.
- Load/Save this setting in `DesktopSettingsDlgProc` within `TaskbarProperties.cpp` targeting the value `ForceProgmanAllDisplays` under `HKCU\Software\EliteSoftware\Win32Explorer\Advanced`.

---

## 5. Verification Method

To verify the implementation of this strategy, run:

1. **Compilation**:
   Run `build.ps1` in a powershell console to confirm successful compilation without errors or warnings.
   ```powershell
   & "C:\Users\Administrator\Desktop\Elite-TaskBar\build.ps1"
   ```

2. **Registry Setting Verification**:
   Ensure the following registry settings are created/edited under `HKCU\Software\EliteSoftware\Win32Explorer\Advanced`:
   - `ForceProgmanAllDisplays` (DWORD) = `1`
   - `DesktopReplacementEnabled` (DWORD) = `1`
   - `TaskbarMode` (DWORD) = `0` (Independent Mode)

3. **Behavioral Inspection**:
   - Run `EliteTaskbar.exe`.
   - Confirm that the native Windows taskbar (`Shell_TrayWnd`) is visible and functional.
   - Confirm that the custom `Progman` desktop window is visible and rendering custom wallpaper and desktop icons across all displays.
   - Verify that the custom desktop stays below the native taskbar in the Z-order.
   - Check `C:\EliteSoftware\Logs\EliteTaskbar.log` for logs verifying successful startup of `DesktopWindow::Initialize`.
