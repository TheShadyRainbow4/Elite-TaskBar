# Handoff Report: Wallpaper Personalization Logic Refactoring Strategy

## 1. Observation

Direct code references and settings mechanisms observed in the repository:
1. **Custom Slideshow Mechanisms** in `SourceFiles/DesktopWindow.cpp`:
   - Line 28: Timer ID macro `#define TIMER_SLIDESHOW 1002`
   - Lines 30–78: Helper function `GetThemeDirectory()` for parsing custom registry `DesktopThemePath` or defaulting to `%SystemRoot%\Web\Wallpaper`.
   - Lines 80–125: Helper function `AdvanceSlideshow(HWND)` that scans the directory for images, sorts them, selects the next index, updates the cached wallpaper path, and triggers a window repaint.
   - Lines 299–304: `WM_TIMER` handler invoking `AdvanceSlideshow(hwnd)` in `ProgmanWndProc`.
   - Line 323: `KillTimer(hwnd, TIMER_SLIDESHOW)` inside `WM_DESTROY`.
   - Lines 704–770: Inside `DrawWallpaper(HWND, HDC, int, int)`, slideshow registry keys `DesktopSlideshowEnabled` and `DesktopSlideshowInterval` are read, and the timer is managed dynamically.

2. **Custom Wallpaper Settings** in `SourceFiles/DesktopWindow.cpp` `DrawWallpaper` function:
   - Lines 671–677: Queries custom `DesktopWallpaperEnabled` key to determine whether to paint a wallpaper.
   - Lines 813–829: Queries custom `DesktopWallpaperMode` key (0 for Span, 1 for Per-Monitor) to decide between multi-monitor drawing (`EnumDisplayMonitors`) and virtual screen span.

3. **Intact Theme Icon Previews** in `SourceFiles/TaskbarProperties.cpp` (must be preserved):
   - Lines 1590–1599: `GetWallpaperPathFromTheme()`
   - Lines 1601–1620: `GetActiveWallpaperPath()`
   - Lines 1622–1663: `DrawWallpaperPreview()`
   - Lines 1665–1681: `LoadThemeSettings()`
   - Lines 1580–1588: `UpdateIconPreviews()`

4. **Custom Wallpaper & Slideshow UI Logic** in `SourceFiles/TaskbarProperties.cpp`:
   - Lines 1739–1740: Tooltips for slideshow elements.
   - Lines 1775–1778: Initializing slideshow variables from `Software\EliteSoftware\Win32Explorer\Advanced`.
   - Lines 1792–1801: Rendering slideshow checkbox and interval combo in UI.
   - Lines 1888–1893 & 1902–1903: Retrieving and saving slideshow values to custom registry.

5. **Native Registry Keys & API Targets**:
   - Location: `HKCU\Control Panel\Desktop`
   - Registry values: `Wallpaper` (REG_SZ), `WallpaperStyle` (REG_SZ), `TileWallpaper` (REG_SZ)
   - APIs: `SystemParametersInfoW(SPI_GETDESKTOPWALLPAPER, ...)` and `SystemParametersInfoW(SPI_SETDESKWALLPAPER, ...)`

---

## 2. Logic Chain

1. **Theme Previews Retention**: Preserving `GetWallpaperPathFromTheme`, `GetActiveWallpaperPath`, `LoadThemeSettings`, and `UpdateIconPreviews` ensures that the visual `.theme` configuration parsing still translates to native theme icon preview static controls without breaking the layout or visual aids in the Settings UI.
2. **Eliminating Custom Slideshows**: Deleting `TIMER_SLIDESHOW`, `GetThemeDirectory()`, `AdvanceSlideshow()`, and corresponding `WM_TIMER`/`WM_DESTROY` hooks in `DesktopWindow.cpp` ensures the application no longer maintains its own directory scanner or background thread/timer scheduling.
3. **Registry and SPI Direct Sync**:
   - Reading `Wallpaper`, `WallpaperStyle`, and `TileWallpaper` directly from `HKCU\Control Panel\Desktop` inside `DrawWallpaper` ensures the custom desktop manager stays perfectly synchronized with any native personalization settings.
   - Writing the selected values directly to the same native registry keys and executing `SystemParametersInfoW(SPI_SETDESKWALLPAPER, ...)` inside `TaskbarProperties.cpp` under `PSN_APPLY` applies the settings system-wide instantly.
4. **Style-Driven Layout Resolution**:
   - WallpaperStyle = `"22"` represents Span mode in Windows. Reading this value lets us draw a single spanned background across the virtual monitor boundaries (`scrW`, `scrH`).
   - For any other styles (Fill `"10"`, Fit `"6"`, Stretch `"2"`, Tile `"0"`, Center `"0"`), calling `EnumDisplayMonitors` ensures monitor-specific rendering is handled properly for multimonitor setups.
5. **Handling Personalization Updates (WM_SETTINGCHANGE)**:
   - Intercepting `WM_SETTINGCHANGE` in `ProgmanWndProc` and checking for system-wide personalization modifications allows us to clear the cached `Gdiplus::Bitmap` object (`s_pCachedWallpaper = nullptr`), force-repaint the manager window client area, and avoid visual lag.

---

## 3. Caveats

- **Gdiplus File Locks**: The GDI+ `Bitmap` constructor locks the source image file on disk. Hence, clearing `s_pCachedWallpaper` immediately upon `WM_SETTINGCHANGE` is critical to allow Windows or other theme editors to modify or overwrite the wallpaper file.
- **Grayscale Registry Styles**: The `WallpaperStyle` and `TileWallpaper` values are stored as string types (`REG_SZ`) in the Windows registry, not double-words (`REG_DWORD`). String-to-integer conversion (`_wtoi`) must be used consistently.
- **Visual Styles & Color Fallbacks**: If the wallpaper path is empty (solid color mode) or the file is missing/unreadable, GDI+ initialization fails. We fall back to painting the window using `GetSysColor(COLOR_BACKGROUND)`, maintaining correct legacy Win32 theme colors.

---

## 4. Conclusion

We recommend the following precise code changes:

### A. Modifications to `SourceFiles/DesktopWindow.cpp`

1. **Remove Slideshow Helpers and Timer Definitions**:
   - Delete `#define TIMER_SLIDESHOW 1002` (Line 28)
   - Delete the `GetThemeDirectory()` function (Lines 30–78)
   - Delete the `AdvanceSlideshow(HWND)` function (Lines 80–125)

2. **Clean up Window Proc Handlers**:
   - Remove the `TIMER_SLIDESHOW` branch under `WM_TIMER` in `ProgmanWndProc` (Lines 299–304).
   - Remove `KillTimer(hwnd, TIMER_SLIDESHOW)` from `WM_DESTROY` (Line 323).
   - Add a `WM_SETTINGCHANGE` case in `ProgmanWndProc` to clear cached wallpaper assets dynamically:
     ```cpp
     case WM_SETTINGCHANGE: {
         if (s_pCachedWallpaper) {
             delete s_pCachedWallpaper;
             s_pCachedWallpaper = nullptr;
         }
         s_cachedWallpaperPath.clear();
         s_cachedStyle = -1;
         s_cachedTile = false;
         
         InvalidateRect(hwnd, NULL, TRUE);
         if (hwndDefView) {
             InvalidateRect(hwndDefView, NULL, TRUE);
         }
         break; // Forward to DefWindowProcW
     }
     ```

3. **Simplify `DrawWallpaper` function**:
   - Rewrite `DrawWallpaper` to strictly query the native registry:
     ```cpp
     void DrawWallpaper(HWND hwnd, HDC hdc, int scrW, int scrH) {
         std::wstring wallpaperPath = L"";
         int style = 10; // Default: Fill
         bool tile = false;

         HKEY hKey;
         if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
             wchar_t szBuffer[MAX_PATH] = { 0 };
             DWORD dwSize = sizeof(szBuffer);
             if (RegQueryValueExW(hKey, L"Wallpaper", NULL, NULL, (LPBYTE)szBuffer, &dwSize) == ERROR_SUCCESS) {
                 wallpaperPath = szBuffer;
             }
             
             dwSize = sizeof(szBuffer);
             if (RegQueryValueExW(hKey, L"WallpaperStyle", NULL, NULL, (LPBYTE)szBuffer, &dwSize) == ERROR_SUCCESS) {
                 style = _wtoi(szBuffer);
             }
             
             dwSize = sizeof(szBuffer);
             if (RegQueryValueExW(hKey, L"TileWallpaper", NULL, NULL, (LPBYTE)szBuffer, &dwSize) == ERROR_SUCCESS) {
                 tile = (_wtoi(szBuffer) == 1);
             }
             RegCloseKey(hKey);
         }

         // Update cache if settings changed
         bool settingsChanged = (wallpaperPath != s_cachedWallpaperPath) ||
                                (style != s_cachedStyle) ||
                                (tile != s_cachedTile);

         if (settingsChanged || !s_pCachedWallpaper) {
             if (s_pCachedWallpaper) {
                 delete s_pCachedWallpaper;
                 s_pCachedWallpaper = nullptr;
             }

             s_cachedStyle = style;
             s_cachedTile = tile;
             s_cachedWallpaperPath = wallpaperPath;

             if (!s_cachedWallpaperPath.empty() && PathFileExistsW(s_cachedWallpaperPath.c_str())) {
                 s_pCachedWallpaper = new Gdiplus::Bitmap(s_cachedWallpaperPath.c_str());
                 if (s_pCachedWallpaper->GetLastStatus() != Gdiplus::Ok) {
                     delete s_pCachedWallpaper;
                     s_pCachedWallpaper = nullptr;
                 }
             }
         }

         if (!s_pCachedWallpaper) {
             HBRUSH hBrush = CreateSolidBrush(GetSysColor(COLOR_BACKGROUND));
             RECT rc = { 0, 0, scrW, scrH };
             FillRect(hdc, &rc, hBrush);
             DeleteObject(hBrush);
             return;
         }

         // Style 22 represents Span (stretch across virtual screen client coordinates)
         if (s_cachedStyle != 22) {
             MonitorWallpaperData data;
             data.hdc = hdc;
             data.pBitmap = s_pCachedWallpaper;
             data.style = s_cachedStyle;
             data.tile = s_cachedTile;
             data.bgColor = GetSysColor(COLOR_BACKGROUND);
             
             EnumDisplayMonitors(NULL, NULL, DrawWallpaperMonitorProc, (LPARAM)&data);
         } else {
             int imgW = s_pCachedWallpaper->GetWidth();
             int imgH = s_pCachedWallpaper->GetHeight();

             Gdiplus::Graphics graphics(hdc);
             graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
             graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);

             if (s_cachedTile) {
                 Gdiplus::TextureBrush brush(s_pCachedWallpaper);
                 brush.SetWrapMode(Gdiplus::WrapModeTile);
                 graphics.FillRectangle(&brush, 0, 0, scrW, scrH);
             } else {
                 int destX = 0, destY = 0, destW = scrW, destH = scrH;
                 int srcX = 0, srcY = 0, srcW = imgW, srcH = imgH;
                 // Span stretched rendering logic...
                 graphics.DrawImage(s_pCachedWallpaper, Gdiplus::Rect(destX, destY, destW, destH), srcX, srcY, srcW, srcH, Gdiplus::UnitPixel);
             }
         }
     }
     ```

### B. Modifications to `SourceFiles/TaskbarProperties.cpp`

1. **Remove Slideshow UI References**:
   - Delete tooltip calls for slideshow controls (Lines 1739–1740).
   - Delete variables `slideshowEnabledVal` and `slideshowIntervalVal` and their query logic from `WM_INITDIALOG` (Lines 1755–1756, 1775–1778, 1792–1801).
   - Hide slideshow controls in `WM_INITDIALOG`:
     ```cpp
     ShowWindow(GetDlgItem(hwndDlg, IDC_DESKTOP_SLIDESHOW_ENABLED), SW_HIDE);
     ShowWindow(GetDlgItem(hwndDlg, IDC_DESKTOP_SLIDESHOW_INTERVAL), SW_HIDE);
     HWND hLabel = FindWindowExW(hwndDlg, NULL, L"Static", L"Interval (s):");
     if (hLabel) {
         ShowWindow(hLabel, SW_HIDE);
     }
     ```

2. **Align UI Selection to Native Windows Personalization**:
   - In `WM_INITDIALOG`, read state directly from native personalization settings:
     ```cpp
     wchar_t sysWallpaper[MAX_PATH] = {0};
     SystemParametersInfoW(SPI_GETDESKTOPWALLPAPER, MAX_PATH, sysWallpaper, 0);
     bool hasWallpaper = (wcslen(sysWallpaper) > 0);
     SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_WALLPAPER_ENABLED, BM_SETCHECK, hasWallpaper ? BST_CHECKED : BST_UNCHECKED, 0);

     wchar_t styleVal[32] = {L"10"};
     wchar_t tileVal[32] = {L"0"};
     HKEY hDesktopKey;
     if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop", 0, KEY_READ, &hDesktopKey) == ERROR_SUCCESS) {
         DWORD dwSize = sizeof(styleVal);
         RegQueryValueExW(hDesktopKey, L"WallpaperStyle", NULL, NULL, (LPBYTE)styleVal, &dwSize);
         dwSize = sizeof(tileVal);
         RegQueryValueExW(hDesktopKey, L"TileWallpaper", NULL, NULL, (LPBYTE)tileVal, &dwSize);
         RegCloseKey(hDesktopKey);
     }
     
     int styleNum = _wtoi(styleVal);
     if (styleNum == 22) {
         SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_MODE_SPAN, BM_SETCHECK, BST_CHECKED, 0);
         SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_MODE_PERMONITOR, BM_SETCHECK, BST_UNCHECKED, 0);
     } else {
         SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_MODE_SPAN, BM_SETCHECK, BST_UNCHECKED, 0);
         SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_MODE_PERMONITOR, BM_SETCHECK, BST_CHECKED, 0);
     }
     ```

3. **Align UI Save Action under `PSN_APPLY`**:
   - Strip out custom slideshow/mode saves to Software\EliteSoftware\Win32Explorer\Advanced (Lines 1888–1893, 1902–1903).
   - Write directly to native settings and trigger updates:
     ```cpp
     bool wallpaperEnabled = (SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_WALLPAPER_ENABLED, BM_GETCHECK, 0, 0) == BST_CHECKED);
     std::wstring wallpaperPath = L"";
     std::wstring wallpaperStyle = L"10"; // Default: Fill
     std::wstring tileWallpaper = L"0";

     if (wallpaperEnabled) {
         wallpaperPath = GetActiveWallpaperPath(hwndDlg);
         
         bool isSpan = (SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_MODE_SPAN, BM_GETCHECK, 0, 0) == BST_CHECKED);
         if (isSpan) {
             wallpaperStyle = L"22";
             tileWallpaper = L"0";
         } else {
             wchar_t themePathVal[MAX_PATH] = {0};
             GetDlgItemTextW(hwndDlg, IDC_DESKTOP_THEME_PATH, themePathVal, MAX_PATH);
             if (wcslen(themePathVal) > 0 && PathFileExistsW(themePathVal)) {
                 wchar_t styleBuf[32] = {L"10"};
                 wchar_t tileBuf[32] = {L"0"};
                 GetPrivateProfileStringW(L"Control Panel\\Desktop", L"WallpaperStyle", L"10", styleBuf, 32, themePathVal);
                 GetPrivateProfileStringW(L"Control Panel\\Desktop", L"TileWallpaper", L"0", tileBuf, 32, themePathVal);
                 
                 if (_wtoi(styleBuf) == 22) {
                     wallpaperStyle = L"10";
                 } else {
                     wallpaperStyle = styleBuf;
                 }
                 tileWallpaper = tileBuf;
             } else {
                 wallpaperStyle = L"10";
                 tileWallpaper = L"0";
             }
         }
     }

     HKEY hDesktopKey;
     if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop", 0, KEY_SET_VALUE, &hDesktopKey) == ERROR_SUCCESS) {
         RegSetValueExW(hDesktopKey, L"Wallpaper", 0, REG_SZ, (const BYTE*)wallpaperPath.c_str(), (DWORD)(wallpaperPath.length() + 1) * sizeof(wchar_t));
         RegSetValueExW(hDesktopKey, L"WallpaperStyle", 0, REG_SZ, (const BYTE*)wallpaperStyle.c_str(), (DWORD)(wallpaperStyle.length() + 1) * sizeof(wchar_t));
         RegSetValueExW(hDesktopKey, L"TileWallpaper", 0, REG_SZ, (const BYTE*)tileWallpaper.c_str(), (DWORD)(tileWallpaper.length() + 1) * sizeof(wchar_t));
         RegCloseKey(hDesktopKey);
     }

     SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, (void*)wallpaperPath.c_str(), SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
     ```

---

## 5. Verification Method

To verify these changes independently:
1. **Compilation Check**:
   Run the build suite command from pwsh in the project workspace:
   `powershell -ExecutionPolicy Bypass .\build.ps1`
   Confirm that all executables compile cleanly without errors or warnings.
2. **Behavioral Invalidation Test**:
   - Manually change wallpaper settings in Windows 10/11 Personalization Settings (e.g. from Solid Color to a Picture or different Fit option).
   - Verify that the custom Progman window dynamically captures `WM_SETTINGCHANGE`, invalidates the previous cached picture structure, reads the newly set native values, and repaints monitors instantly.
   - Toggle radio buttons in Settings UI (Span vs. Per-monitor) and click "Apply". Verify that native registry value `WallpaperStyle` under `HKCU\Control Panel\Desktop` toggles between `22` and `10`, and changes affect desktop rendering immediately.
