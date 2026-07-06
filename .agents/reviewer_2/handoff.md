# Handoff Report - Milestone 7 Review (Reviewer 2)

## 1. Observation
- In `SourceFiles/DesktopWindow.cpp` (Lines 29-48), `GetThemeDirectory` extracts the parent directory from `DesktopThemePath` registry key:
  ```cpp
  static std::wstring GetThemeDirectory() {
      HKEY hKey;
      wchar_t themePathVal[MAX_PATH] = {0};
      ...
      if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
          RegQueryValueExW(hKey, L"DesktopThemePath", NULL, NULL, (LPBYTE)themePathVal, &cbData);
          RegCloseKey(hKey);
      }
      
      if (wcslen(themePathVal) > 0) {
          std::wstring pathStr(themePathVal);
          if (PathIsDirectoryW(pathStr.c_str())) {
              return pathStr;
          } else {
              size_t lastSlash = pathStr.find_last_of(L'\\');
              if (lastSlash != std::wstring::npos) {
                  return pathStr.substr(0, lastSlash);
              }
          }
      }
  ```
- In `SourceFiles/DesktopWindow.cpp` (Lines 726-728), `DrawWallpaper` skips setting `s_cachedWallpaperPath` when slideshow is enabled:
  ```cpp
  if (!slideshowEnabled) {
      s_cachedWallpaperPath = wallpaperPath;
  }
  ```
- In `SourceFiles/TaskbarProperties.cpp` (Lines 1564-1588), `UpdateIconPreviews` sets icon handles on the static controls but does not register cleanup for the final active icons on destruction:
  ```cpp
  for (int i = 0; i < 4; i++) {
      wchar_t iconPath[MAX_PATH] = {0};
      GetPrivateProfileStringW(clsids[i], L"DefaultValue", L"", iconPath, MAX_PATH, themePath.c_str());
      HICON hIcon = LoadThemeIcon(iconPath);
      if (hIcon) {
          HICON hOldIcon = (HICON)SendDlgItemMessageW(hwndDlg, previewIds[i], STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
          if (hOldIcon) DestroyIcon(hOldIcon);
      }
  }
  ```
- In `build.ps1` (Lines 185-195), `build_sign.ps1` is invoked prior to compilation of `Win32Explorer.sln` and compilation of `EliteStartMenu.ps1`:
  ```powershell
  & "$ScriptDir\build_sign.ps1" -BuildDir $BuildDir -BuildDirx86 $BuildDirx86
  Write-Host "Building Win32Explorer..." -ForegroundColor Cyan
  ...
  & ".\build_Win32Explorer.ps1" -Platform "x64"
  ```
- Digital signatures validation for `BuildOutput` files:
  - `BuildOutput\EliteSettings.exe`: `NotSigned`
  - `BuildOutput\EliteSettings.cpl`: `NotSigned`
  - `BuildOutput\EliteTaskbar.exe`: `NotSigned`
  - `BuildOutputx86\EliteTaskbar_x86.exe`: `NotSigned`
  - Root directory versions (copied prior to Win32Explorer build) are properly signed.

## 2. Logic Chain
1. **Slideshow Directory Scan Failure**: The `DesktopThemePath` registry key stores the full file path to the selected `.theme` file (e.g. `C:\Windows\Resources\Themes\aero.theme`). `GetThemeDirectory` strips the filename to return `C:\Windows\Resources\Themes`. However, Windows theme directories store theme files in the root folder, and image files in a separate subdirectory (like `C:\Windows\Web\Wallpaper`). Since there are no image files inside the root theme folder, `AdvanceSlideshow`'s directory scan fails to find any images and returns early. The slideshow never advances.
2. **Slideshow First Launch Delay**: On startup, if slideshow is enabled, the code does not initialize the wallpaper cache with the current desktop wallpaper (`s_cachedWallpaperPath` is empty and `s_pCachedWallpaper` is `nullptr`). Since the timer is set to trigger only after `slideshowInterval` seconds, the desktop renders as a solid background color until the first interval expires.
3. **HICON Resource Leak**: `UpdateIconPreviews` extracts HICON handles using `ExtractIconExW` (which creates unmanaged icon handles) and applies them via `STM_SETIMAGE`. While setting new icons frees old ones, closing the dialog destroys the static controls without freeing the final 4 active HICON handles. According to MSDN, static controls do not automatically destroy their icons upon destruction, resulting in a persistent leak of 4 HICON handles every time properties are closed.
4. **GDI+ Token Leak**: GDI+ is initialized in `WM_INITDIALOG` of `DesktopSettingsDlgProc`, but the matching `GdiplusShutdown` is never called when the dialog window is destroyed, leaving GDI+ active in the settings process space.
5. **Unsigned BuildOutput Binaries**: Running MSBuild on `Win32Explorer.sln` recompiles dependent files in `BuildOutput` and `BuildOutputx86` *after* the signing stage has executed. This leaves the final binaries in those subfolders unsigned, violating milestone requirements for signed binaries.

## 3. Caveats
- secondary multi-monitor coordinates were verified using coordinate transforms but not verified with physical hardware displays.
- Unmanaged GDI/User resource checks were validated via Win32 API design principles rather than heap profiling.

## 4. Conclusion

### Review Summary

**Verdict**: REQUEST_CHANGES

### Findings

#### [Major] Finding 1: Broken Slideshow Theme Directory Scan
- **What**: The wallpaper slideshow fails to rotate wallpapers when selecting standard themes.
- **Where**: `SourceFiles/DesktopWindow.cpp` (Lines 29-58)
- **Why**: `GetThemeDirectory` returns `C:\Windows\Resources\Themes` which contains no images. It should extract the actual wallpaper path specified within the `.theme` file (under `[Control Panel\Desktop]\Wallpaper`) and scan its parent directory instead.
- **Suggestion**: If `themePathVal` points to a theme file, read the `Wallpaper` entry from the theme file using `GetPrivateProfileStringW`, expand environment strings, and extract the parent directory of that wallpaper.

#### [Major] Finding 2: Startup Wallpaper Delay under Slideshow
- **What**: desktop replacement renders a solid background color on startup/initialization when slideshow is enabled.
- **Where**: `SourceFiles/DesktopWindow.cpp` (Lines 726-728)
- **Why**: Wallpaper cache `s_cachedWallpaperPath` remains empty on first launch, so the initial paint renders solid color. It only updates once the timer triggers.
- **Suggestion**: Initialize `s_cachedWallpaperPath` with the active wallpaper path on startup, or immediately trigger `AdvanceSlideshow` when initializing slideshow mode so that the wallpaper loads instantly.

#### [Major] Finding 3: HICON Previews Memory Leak
- **What**: Closing the settings properties window leaks 4 HICON handles.
- **Where**: `SourceFiles/TaskbarProperties.cpp` (Lines 1564-1588)
- **Why**: Dynamically set icons via `STM_SETIMAGE` must be manually destroyed when the controls are destroyed. No WM_DESTROY handler is present to clean them up.
- **Suggestion**: Implement a `WM_DESTROY` case in `DesktopSettingsDlgProc` that retrieves the icons using `STM_GETIMAGE` and calls `DestroyIcon` on each.

#### [Minor] Finding 4: GDI+ Token Leak in Settings
- **What**: `GdiplusShutdown` is never called when closing properties.
- **Where**: `SourceFiles/TaskbarProperties.cpp` (Lines 1700-1903)
- **Why**: GDI+ remains active in the CPL / Settings process after the properties page closes.
- **Suggestion**: Clean up the static `gdiplusToken` inside `WM_DESTROY` of the properties dialog.

#### [Major] Finding 5: Unsigned Binaries in BuildOutput Subdirectories
- **What**: Build output files in `BuildOutput\` and `BuildOutputx86\` are not digitally signed.
- **Where**: `build.ps1` (Lines 185-210)
- **Why**: MSBuild of `Win32Explorer` solutions recompiles dependencies and overwrites signed binaries with unsigned ones.
- **Suggestion**: Re-order the build script so that all MSBuild compilations and ps2exe compilations complete *before* `build_sign.ps1` is invoked.

### Verified Claims
- Custom Progman class hierarchy and list view rendering -> verified via `verify_desktop_shell.ps1` -> PASS
- Registry settings toggling from properties page -> verified via `verify_desktop_shell.ps1` -> PASS
- Directory change watcher debouncing -> verified via `verify_desktop_shell.ps1` -> PASS

### Coverage Gaps
- Physical multi-monitor desktop replacement coordinates -> risk level: LOW.

### Unverified Items
- None.

---

### Challenge Summary

**Overall risk assessment**: MEDIUM

### Challenges

#### [Medium] Challenge 1: Memory Leak under Repeated Settings View
- **Assumption challenged**: Settings page has no memory/resource leaks.
- **Attack scenario**: Open and close the settings dialog repeatedly (e.g. 50 times).
- **Blast radius**: The settings process leaks up to 200 HICON handles. In a long-running process (like the desktop shell or explorer process), this will slowly deplete User objects and eventually lead to rendering failures or crash.
- **Mitigation**: Clean up the icons on `WM_DESTROY`.

#### [High] Challenge 2: Wallpapers Slideshow Failure under Standard Themes
- **Assumption challenged**: Wallpaper slideshow successfully rotates images for any selected theme directory.
- **Attack scenario**: User selects the "Aero" theme and checks "Enable Slideshow".
- **Blast radius**: Slideshow finds 0 files and never rotates, silently failing.
- **Mitigation**: Extract the wallpaper path from the theme file to target the actual wallpaper directory.

### Stress Test Results
- Standard Theme slideshow configuration -> returns 0 wallpapers scanned -> fails to advance -> FAIL.

## 5. Verification Method
1. Compile the project using `build.ps1`.
2. Inspect the signature of the output executables in `BuildOutput` using:
   ```powershell
   Get-ChildItem -Path BuildOutput, BuildOutputx86 -Recurse | Where-Object { $_.Extension -in ".exe", ".cpl" } | Get-AuthenticodeSignature
   ```
   Note that several files report `NotSigned`.
3. Open `EliteSettings.exe`, navigate to the Desktop tab, configure a theme (e.g. Aero), check "Enable Slideshow", and save.
4. Launch `EliteTaskbar.exe` and notice that the desktop wallpaper does not load immediately (delay of up to the slideshow interval, e.g. 5 minutes).
5. Verify that the slideshow fails to cycle wallpapers when Aero theme is active.
