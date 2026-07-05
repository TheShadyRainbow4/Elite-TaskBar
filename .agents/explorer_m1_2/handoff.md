# Wallpaper Rendering Logic & Message Interception Report

## 1. Observation
We conducted tests and queries on the Windows host machine to gather facts about the active wallpaper settings and the desktop window hierarchy:

- **Registry Values**:
  By executing `reg query "HKCU\Control Panel\Desktop" /v Wallpaper` and associated keys, we observed the following configuration:
  - Wallpaper file path:
    ```
    HKEY_CURRENT_USER\Control Panel\Desktop
        Wallpaper    REG_SZ    C:\Windows\resources\Themes\Aero10\Wallpapers\img36.png
    ```
  - Wallpaper layout style:
    ```
    HKEY_CURRENT_USER\Control Panel\Desktop
        WallpaperStyle    REG_SZ    10
    ```
  - Wallpaper tiling status:
    ```
    HKEY_CURRENT_USER\Control Panel\Desktop
        TileWallpaper    REG_SZ    0
    ```

- **Window Tree Structure**:
  We compiled a Win32 window tree inspector to trace the desktop window structure.
  - *State A: Before Sending Message `0x052C` to Progman*:
    ```
    HWND: 0x17C151E | Class: Progman | Title: Program Manager
      -> Child HWND: 0x14B0888 | Class: SHELLDLL_DefView | Title: 
    ```
  - *State B: After Sending Message `0x052C` to Progman*:
    ```
    HWND: 0x2D40F96 | Class: WorkerW | Title: 
      -> Child HWND: 0x14B0888 | Class: SHELLDLL_DefView | Title: 
    HWND: 0x9F0A6E | Class: WorkerW | Title: 
    HWND: 0x17C151E | Class: Progman | Title: Program Manager
    ```
    We observed that sending the undocumented message `0x052C` causes Explorer to split the desktop shell view:
    1. The desktop icons (`SHELLDLL_DefView`) are reparented under a newly active `WorkerW` window (`0x2D40F96`).
    2. A sibling `WorkerW` window (`0x9F0A6E`) is created directly behind the icon window. This sibling window is responsible for drawing the desktop wallpaper.

---

## 2. Logic Chain
1. **Registry Reading**: Reading the `Wallpaper` path registry key yields the path to the current wallpaper. However, because modern Windows wallpapers are often `.png` or `.jpg` formats, standard GDI functions like `LoadImage` (which only natively decodes `.bmp` or `.ico`) will fail. Therefore, the drawing engine must use a modern decoding API, such as **GDI+** (`Gdiplus::Bitmap::FromFile`) or **WIC** (Windows Imaging Component).
2. **Layout Logic**:
   - `TileWallpaper = 1` indicates **Tiled** mode.
   - `TileWallpaper = 0` requires evaluating `WallpaperStyle`:
     - `0` = **Centered**
     - `2` = **Stretched**
     - `6` = **Fit**
     - `10` = **Fill**
     - `22` = **Span**
3. **Coordinate Calculations**:
   - For **Stretch**: Scale the image source boundaries directly to the target monitor width and height.
   - For **Center**: Calculate the offset `(target - image) / 2` for the X/Y axes and draw 1:1. Clip if the image is larger than the screen.
   - For **Tile**: Repeat the image horizontally and vertically starting at `(0,0)`.
   - For **Fit**: Uniformly scale the image to fit within the screen boundaries, leaving letterbox/pillarbox empty bars colored with the desktop background color.
   - For **Fill**: Uniformly scale the image to cover the screen entirely, clipping any overflow.
4. **GDI Quality**: Calling `StretchBlt` directly uses nearest-neighbor scaling by default, causing visible pixelation. To ensure high-quality resampling, the destination Device Context (HDC) must be configured using `SetStretchBltMode(hdcDest, HALFTONE)` and `SetBrushOrgEx(hdcDest, 0, 0, NULL)`.
5. **Interception Constraints**:
   - The desktop background window (`WorkerW` or `Progman`) is owned by `explorer.exe` (a separate process).
   - Direct cross-process subclassing (using `SetWindowSubclass` or `SetWindowLongPtr` from a separate user-mode process) fails with `ERROR_ACCESS_DENIED`.
   - Therefore, message interception requires either:
     - **In-process Hooking (DLL Injection)**: Injecting a custom DLL into `explorer.exe` to intercept `WM_ERASEBKGND` or `WM_PAINT` directly on the wallpaper window.
     - **Overlay Window (Out-of-Process)**: Creating a borderless popup window within our own process, finding the desktop window structure, and reparenting our window into the Z-order (as a child of the wallpaper `WorkerW` or just behind the folder view `WorkerW`). This allows us to handle rendering in our own process safely.

---

## 3. Caveats
- **Multi-Monitor Layouts**: The `Wallpaper` key only points to a single file. Under multi-monitor configurations, Windows stores independent monitor wallpaper paths under other keys (like `TranscodedImageCache` keys under `HKCU\Control Panel\Desktop`). If a span or distinct per-monitor layout is configured, reading only the default `Wallpaper` key will miss the secondary monitors' wallpapers.
- **Dynamic Transitions**: Active Desktop, slideshow transitions, or third-party wallpaper applications (like Wallpaper Engine) will routinely overwrite or reset the `WorkerW` hierarchy, which might hide or detach subclassed or parented windows.
- **Shell Actions**: Certain Windows operations (such as Win+Tab / Task View and desktop switching) may temporarily hide or manipulate `WorkerW` windows, meaning an overlay window must monitor shell events (`SetWinEventHook`) to maintain its proper Z-order.

---

## 4. Conclusion
Windows wallpaper paths and styles are read from `HKCU\Control Panel\Desktop` and rendered either on `Progman` or a background `WorkerW` window. For custom rendering, we recommend creating a borderless `WS_POPUP` window in our process, parenting it to the desktop's background `WorkerW` (obtained by sending message `0x052C` to `Progman` and enumerating top-level windows), and rendering using GDI+ (`Gdiplus::Graphics::DrawImage`) or GDI (`StretchBlt` with `HALFTONE` mode).

### Recommended C++ Code Structures

#### A. Registry Query (Wallpaper & Styles)
```cpp
#include <windows.h>
#include <string>

struct WallpaperSettings {
    std::wstring path;
    int style = 10; // Default: Fill (10)
    bool tile = false;
};

WallpaperSettings GetWallpaperSettings() {
    WallpaperSettings settings;
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        wchar_t szBuffer[MAX_PATH] = { 0 };
        DWORD dwSize = sizeof(szBuffer);
        
        // Retrieve wallpaper image file path
        if (RegQueryValueExW(hKey, L"Wallpaper", NULL, NULL, (LPBYTE)szBuffer, &dwSize) == ERROR_SUCCESS) {
            settings.path = szBuffer;
        }
        
        // Retrieve wallpaper style
        dwSize = sizeof(szBuffer);
        if (RegQueryValueExW(hKey, L"WallpaperStyle", NULL, NULL, (LPBYTE)szBuffer, &dwSize) == ERROR_SUCCESS) {
            settings.style = _wtoi(szBuffer);
        }
        
        // Retrieve tile flag
        dwSize = sizeof(szBuffer);
        if (RegQueryValueExW(hKey, L"TileWallpaper", NULL, NULL, (LPBYTE)szBuffer, &dwSize) == ERROR_SUCCESS) {
            settings.tile = (_wtoi(szBuffer) == 1);
        }
        
        RegCloseKey(hKey);
    }
    return settings;
}
```

#### B. Coordinate Calculations for Wallpaper Styles
```cpp
struct DrawRects {
    int destX, destY, destW, destH;
    int srcX, srcY, srcW, srcH;
};

DrawRects CalculateWallpaperCoords(int style, bool tile, int scrW, int scrH, int imgW, int imgH) {
    DrawRects r = { 0 };
    
    // Default source covers full image
    r.srcX = 0; r.srcY = 0;
    r.srcW = imgW; r.srcH = imgH;

    if (tile) {
        // Tile is handled using pattern brushes in GDI/GDI+
        r.destX = 0; r.destY = 0;
        r.destW = scrW; r.destH = scrH;
        return r;
    }

    double imgAspect = (double)imgW / imgH;
    double scrAspect = (double)scrW / scrH;

    switch (style) {
        case 0: // Center
            if (imgW <= scrW) {
                r.destX = (scrW - imgW) / 2;
                r.destW = imgW;
            } else {
                r.destX = 0;
                r.destW = scrW;
                r.srcX = (imgW - scrW) / 2;
                r.srcW = scrW;
            }
            if (imgH <= scrH) {
                r.destY = (scrH - imgH) / 2;
                r.destH = imgH;
            } else {
                r.destY = 0;
                r.destH = scrH;
                r.srcY = (imgH - scrH) / 2;
                r.srcH = scrH;
            }
            break;

        case 2: // Stretch
            r.destX = 0; r.destY = 0;
            r.destW = scrW; r.destH = scrH;
            break;

        case 6: // Fit (Letterbox / Pillarbox)
            if (imgAspect > scrAspect) {
                r.destW = scrW;
                r.destH = (int)(scrW / imgAspect);
                r.destX = 0;
                r.destY = (scrH - r.destH) / 2;
            } else {
                r.destH = scrH;
                r.destW = (int)(scrH * imgAspect);
                r.destY = 0;
                r.destX = (scrW - r.destW) / 2;
            }
            break;

        case 10: // Fill (Crop overflow)
        default:
            if (imgAspect > scrAspect) {
                r.destH = scrH;
                r.destW = (int)(scrH * imgAspect);
                r.destY = 0;
                r.destX = (scrW - r.destW) / 2;
            } else {
                r.destW = scrW;
                r.destH = (int)(scrW / imgAspect);
                r.destX = 0;
                r.destY = (scrH - r.destH) / 2;
            }
            break;
            
        case 22: // Span (Virtual screen coordinates - virtual bounding box should replace scrW/scrH)
            r.destX = 0; r.destY = 0;
            r.destW = scrW; r.destH = scrH;
            break;
    }
    return r;
}
```

#### C. GDI+ Rendering Engine
```cpp
#include <gdiplus.h>

void RenderWallpaperGDIPlus(HDC hdcDest, const WallpaperSettings& settings, int scrW, int scrH) {
    Gdiplus::Bitmap bitmap(settings.path.c_str());
    if (bitmap.GetLastStatus() != Gdiplus::Ok) return;

    int imgW = bitmap.GetWidth();
    int imgH = bitmap.GetHeight();

    Gdiplus::Graphics graphics(hdcDest);
    graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);

    if (settings.tile) {
        Gdiplus::TextureBrush brush(&bitmap);
        brush.SetWrapMode(Gdiplus::WrapModeTile);
        graphics.FillRectangle(&brush, 0, 0, scrW, scrH);
    } else {
        DrawRects r = CalculateWallpaperCoords(settings.style, false, scrW, scrH, imgW, imgH);
        
        // Fill letterbox/pillarbox areas for Fit style
        if (settings.style == 6) {
            graphics.Clear(Gdiplus::Color(0, 0, 0)); // or GetSysColor(COLOR_BACKGROUND)
        }
        
        graphics.DrawImage(&bitmap, 
            Gdiplus::Rect(r.destX, r.destY, r.destW, r.destH),
            r.srcX, r.srcY, r.srcW, r.srcH, 
            Gdiplus::UnitPixel);
    }
}
```

#### D. Pure GDI Scaling Rendering (Alternative)
```cpp
void RenderWallpaperGDI(HDC hdcDest, const WallpaperSettings& settings, int scrW, int scrH) {
    Gdiplus::Bitmap bitmap(settings.path.c_str());
    if (bitmap.GetLastStatus() != Gdiplus::Ok) return;

    HBITMAP hBmp = NULL;
    bitmap.GetHBITMAP(Gdiplus::Color(0, 0, 0), &hBmp);
    if (!hBmp) return;

    int imgW = bitmap.GetWidth();
    int imgH = bitmap.GetHeight();

    HDC hdcMem = CreateCompatibleDC(hdcDest);
    HGDIOBJ hOld = SelectObject(hdcMem, hBmp);

    if (settings.tile) {
        HBRUSH hBrush = CreatePatternBrush(hBmp);
        RECT rc = { 0, 0, scrW, scrH };
        FillRect(hdcDest, &rc, hBrush);
        DeleteObject(hBrush);
    } else {
        DrawRects r = CalculateWallpaperCoords(settings.style, false, scrW, scrH, imgW, imgH);

        if (settings.style == 6) {
            HBRUSH hBg = CreateSolidBrush(GetSysColor(COLOR_BACKGROUND));
            RECT rc = { 0, 0, scrW, scrH };
            FillRect(hdcDest, &rc, hBg);
            DeleteObject(hBg);
        }

        // Enable Halftone rendering for high-quality resizing
        SetStretchBltMode(hdcDest, HALFTONE);
        SetBrushOrgEx(hdcDest, 0, 0, NULL);

        StretchBlt(hdcDest, 
            r.destX, r.destY, r.destW, r.destH, 
            hdcMem, 
            r.srcX, r.srcY, r.srcW, r.srcH, 
            SRCCOPY);
    }

    SelectObject(hdcMem, hOld);
    DeleteDC(hdcMem);
    DeleteObject(hBmp);
}
```

#### E. Find Background Window and Parent Overlay
```cpp
HWND GetDesktopBackgroundWindow() {
    HWND hwndDesktop = FindWindowW(L"Progman", NULL);
    
    // Force split and creation of WorkerW windows
    SendMessageTimeoutW(hwndDesktop, 0x052C, 0, 0, SMTO_NORMAL, 1000, NULL);
    
    struct EnumData {
        HWND hwndWallpaper;
    } data = { NULL };

    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        HWND shellDefView = FindWindowExW(hwnd, NULL, L"SHELLDLL_DefView", NULL);
        if (shellDefView != NULL) {
            // Find the WorkerW immediately behind this SHELLDLL_DefView window
            HWND hwndWorkerW = FindWindowExW(null, hwnd, L"WorkerW", NULL);
            if (hwndWorkerW != NULL) {
                ((EnumData*)lParam)->hwndWallpaper = hwndWorkerW;
                return FALSE; // Terminate enumeration
            }
        }
        return TRUE;
    }, (LPARAM)&data);

    if (data.hwndWallpaper == NULL) {
        // Fallback to Progman if split WorkerW is not present
        data.hwndWallpaper = hwndDesktop;
    }
    return data.hwndWallpaper;
}

// Parent custom overlay to desktop wallpaper window
void SetupOverlay(HWND hwndCustom) {
    HWND hwndWallpaper = GetDesktopBackgroundWindow();
    SetParent(hwndCustom, hwndWallpaper);
    
    // Adjust size of the overlay to virtual screen bounds
    int scrX = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int scrY = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int scrW = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int scrH = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    
    SetWindowPos(hwndCustom, HWND_BOTTOM, scrX, scrY, scrW, scrH, SWP_NOACTIVATE | SWP_SHOWWINDOW);
}
```

---

## 5. Verification Method
1. **Registry Verification**:
   Execute the query:
   ```cmd
   reg query "HKCU\Control Panel\Desktop" /v Wallpaper
   ```
   Verify that a valid absolute path to an image file is returned.
2. **Window Hierarchy Verification**:
   Compile the window tree inspector program, execute it, and verify that sending message `0x052C` creates a separate sibling `WorkerW` window next to the `WorkerW` holding `SHELLDLL_DefView`.
3. **Aspect Ratio Verification**:
   Create a test Win32 application utilizing the provided `CalculateWallpaperCoords` and `RenderWallpaperGDIPlus` / `RenderWallpaperGDI` functions. Load the wallpaper path from the registry and check that:
   - Fit style renders pillarbox/letterbox bands properly.
   - Fill style zooms and crops center portion without stretching layout.
   - Stretch style fills full test canvas stretching aspect.
   - Tile style pattern repeats horizontally/vertically.
