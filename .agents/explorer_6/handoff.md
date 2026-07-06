# Handoff Report - Property Sheet Leaks and Signing Order Fix Recommendations

## 1. Observation

### Static Control Handle Leaks (HICON)
- **File**: `C:\Users\Administrator\Desktop\Elite-TaskBar\SourceFiles\TaskbarProperties.cpp`
- **Location**: `DesktopSettingsDlgProc` (lines 1700–1903) and helper function `UpdateIconPreviews` (lines 1564–1588).
- **Code Details**:
  - Inside `UpdateIconPreviews`, private `HICON` handles are extracted from themes using `ExtractIconExW` (line 1557) or standard shared icons (line 1561), and set to static controls `IDC_DESKTOP_ICON_PREVIEW_1` through `IDC_DESKTOP_ICON_PREVIEW_4` via:
    ```cpp
    HICON hOldIcon = (HICON)SendDlgItemMessageW(hwndDlg, previewIds[i], STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
    if (hOldIcon) DestroyIcon(hOldIcon);
    ```
  - When the dialog sheet is closed, the static controls are destroyed. However, the static control class does not automatically call `DestroyIcon` on the currently associated private icon handle.
  - Review of `DesktopSettingsDlgProc` shows no `WM_DESTROY` message handler exists to retrieve and destroy these active handles. This leaks 4 `HICON` handles every time the dialog is closed.

### GDI+ Token Leak
- **File**: `C:\Users\Administrator\Desktop\Elite-TaskBar\SourceFiles\TaskbarProperties.cpp`
- **Location**: Multiple page dialog procedures: `MultiMonSettingsDlgProc` (line 976), `StartMenuSettingsDlgProc` (line 1249), and `DesktopSettingsDlgProc` (line 1701).
- **Code Details**:
  - GDI+ is initialized in each dialog's `WM_INITDIALOG` message handler via static `gdiplusToken` tracking:
    ```cpp
    static ULONG_PTR gdiplusToken = 0;
    // ...
    if (!gdiplusToken) {
        Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    }
    ```
  - Searching for `GdiplusShutdown` in the entire file `TaskbarProperties.cpp` yields 0 results.
  - When the property sheet is closed, GDI+ is never shut down, leaking the initialized token(s) and their associated resources.

### Compilation and Signing Execution Order
- **Files**: `C:\Users\Administrator\Desktop\Elite-TaskBar\build.ps1` and `C:\Users\Administrator\Desktop\Elite-TaskBar\build_sign.ps1`
- **Code Details (Current Execution Sequence)**:
  - In `build.ps1`:
    - Lines 136–158: Compile C++ targets (`EliteTaskbar.exe`, `EliteSettings.exe`/`.cpl`/`everything`/`scanner`) for x64 and x86.
    - Lines 179–183: Copy x64 binaries from `BuildOutput` to root.
    - Line 186: Execute signing stage: `& "$ScriptDir\build_sign.ps1" -BuildDir $BuildDir -BuildDirx86 $BuildDirx86`.
    - Lines 191–204: Build `Win32Explorer.sln` (MSBuild) for x64 and Win32.
    - Lines 240–244: Compile `EliteStartMenu.exe` (ps2exe) from `EliteStartMenu.ps1` for x64 and x86.
  - In `build_sign.ps1`:
    - Lines 10–26: The `$exeFiles` array defines files to sign. It excludes `Win32Explorer.exe`, `Win32Explorer_x86.exe`, and `EliteStartMenu.exe`.
  - Because `build_sign.ps1` runs *before* `Win32Explorer.exe` and `EliteStartMenu.exe` are compiled, and because these binaries are not listed in the signing list, they remain unsigned in their output directories.

---

## 2. Logic Chain

1. **Static Control Icon Leaks**:
   - **Step 1**: `UpdateIconPreviews` assigns HICON handles to the preview controls via `STM_SETIMAGE`.
   - **Step 2**: If an existing HICON is replaced, it is retrieved and destroyed. However, when the dialog is closed, child static controls are destroyed by the OS without cleaning up their assigned `HICON` handles.
   - **Step 3**: To intercept the destruction phase, a `WM_DESTROY` handler must be added to `DesktopSettingsDlgProc`. It must query the current image handle using `STM_GETIMAGE` with type `IMAGE_ICON` for each preview control and call `DestroyIcon` to release GDI resources.

2. **GDI+ Token Leak**:
   - **Step 1**: GDI+ requires balanced `GdiplusStartup` and `GdiplusShutdown` calls.
   - **Step 2**: GDI+ is initialized in `WM_INITDIALOG` of three separate page procedures, but `GdiplusShutdown` is never called.
   - **Step 3**: We have two logical solutions:
     - *Approach A (Centralized - Recommended)*: Move initialization and shutdown to the entry point `ShowTaskbarProperties` wrapping the blocking `PropertySheetW` call. Since the sheet is synchronous, this guarantees GDI+ is initialized exactly once for the duration of the GUI session and cleanly shut down on close.
     - *Approach B (Local - Decoupled)*: Add a `WM_DESTROY` handler in each of the three dialog procedures to shut down GDI+ using their local static token and reset the token to `0`.

3. **Build and Signing Order**:
   - **Step 1**: Centralized code signing requires all compilation units to be complete before the signing tool is executed.
   - **Step 2**: Reordering the compilation stages so that `build_Win32Explorer.ps1` and `EliteStartMenu` compilation run prior to copying files to root and calling `build_sign.ps1` ensures all binaries are compiled and in their final locations.
   - **Step 3**: Expanding `$exeFiles` in `build_sign.ps1` to include `Win32Explorer.exe`, `Win32Explorer_x86.exe`, and `EliteStartMenu.exe` (both x64 and x86) guarantees that the signing tool successfully signs every binary.

---

## 3. Caveats

- **Aero Glass & GDI+ Dependencies**: This analysis assumes GDI+ is only used within the properties page context (e.g. for wallpaper preview in `DrawWallpaperPreview`). If other parts of the settings application require GDI+ to remain active longer, Approach B (Local per-page initialization and shutdown) might be safer, though Approach A is cleaner for a synchronous dialog loop.
- **DPI Scaling on Icons**: Static controls query the active `HICON`. If a dynamic theme change occurs, the handles are replaced and destroyed. The recommended logic safely destroys only the final icon handles when the window is torn down.

---

## 4. Conclusion

- **HICON Leak Fix**: Add a `WM_DESTROY` case inside `DesktopSettingsDlgProc` to retrieve and destroy the 4 icon handles currently bound to `IDC_DESKTOP_ICON_PREVIEW_1` through `IDC_DESKTOP_ICON_PREVIEW_4`.
- **GDI+ Leak Fix**: Implement centralized GDI+ initialization and shutdown in `ShowTaskbarProperties` surrounding the `PropertySheetW` call, or add `GdiplusShutdown` cleanups in the `WM_DESTROY` handler of the respective pages.
- **Signing Fix**: Reorder `build.ps1` to compile Win32Explorer and EliteStartMenu before copying files and calling `build_sign.ps1`. Expand the signing list in `build_sign.ps1` to include the missing executables.

---

## 5. Proposed Code Changes & Diff Patches

### A. Fix for `TaskbarProperties.cpp` (HICON Leaks & GDI+ Shutdown Approach A - Recommended)

```diff
#region 1. Centralized GDI+ Lifecycle in ShowTaskbarProperties
--- SourceFiles/TaskbarProperties.cpp
+++ SourceFiles/TaskbarProperties.cpp
@@ -1909,4 +1909,8 @@
 void ShowTaskbarProperties(HWND hwndOwner) {
+    ULONG_PTR gdiplusToken = 0;
+    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
+    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
+
     std::vector<HPROPSHEETPAGE> pages;
     PROPSHEETPAGEW psp[10] = {0};
     
@@ -2003,4 +2007,8 @@
     PropertySheetW(&psh);
+
+    if (gdiplusToken) {
+        Gdiplus::GdiplusShutdown(gdiplusToken);
+    }
 }
#endregion

#region 2. Remove Local GDI+ Startup Blocks
# Note: Remove local static ULONG_PTR gdiplusToken and GdiplusStartup calls from:
# - MultiMonSettingsDlgProc (lines 976, 980-983)
# - StartMenuSettingsDlgProc (lines 1249, 1253-1256)
# - DesktopSettingsDlgProc (lines 1701, 1705-1708)
#endregion

#region 3. HICON Cleanup in WM_DESTROY
--- SourceFiles/TaskbarProperties.cpp
+++ SourceFiles/TaskbarProperties.cpp
@@ -1700,5 +1700,18 @@
 INT_PTR CALLBACK DesktopSettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
     switch (uMsg) {
+    case WM_DESTROY: {
+        int previewIds[] = {
+            IDC_DESKTOP_ICON_PREVIEW_1,
+            IDC_DESKTOP_ICON_PREVIEW_2,
+            IDC_DESKTOP_ICON_PREVIEW_3,
+            IDC_DESKTOP_ICON_PREVIEW_4
+        };
+        for (int i = 0; i < 4; i++) {
+            HICON hIcon = (HICON)SendDlgItemMessageW(hwndDlg, previewIds[i], STM_GETIMAGE, IMAGE_ICON, 0);
+            if (hIcon) {
+                DestroyIcon(hIcon);
+            }
+        }
+        break;
+    }
     case WM_INITDIALOG: {
```

### B. Alternative Fix for `TaskbarProperties.cpp` (Local `WM_DESTROY` Shutdown Approach B)

If the developer prefers to keep GDI+ initialization local to each dialog page:

```diff
#region MultiMonSettingsDlgProc
--- SourceFiles/TaskbarProperties.cpp
+++ SourceFiles/TaskbarProperties.cpp
@@ -974,5 +974,12 @@
 INT_PTR CALLBACK MultiMonSettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
     static HWND hScroll = NULL;
     static ULONG_PTR gdiplusToken = 0;
     switch (uMsg) {
+    case WM_DESTROY: {
+        if (gdiplusToken) {
+            GdiplusShutdown(gdiplusToken);
+            gdiplusToken = 0;
+        }
+        break;
+    }
     case WM_INITDIALOG: {
```

```diff
#region StartMenuSettingsDlgProc
--- SourceFiles/TaskbarProperties.cpp
+++ SourceFiles/TaskbarProperties.cpp
@@ -1247,5 +1247,12 @@
 INT_PTR CALLBACK StartMenuSettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
     static HWND hScroll = NULL;
     static ULONG_PTR gdiplusToken = 0;
     switch (uMsg) {
+    case WM_DESTROY: {
+        if (gdiplusToken) {
+            GdiplusShutdown(gdiplusToken);
+            gdiplusToken = 0;
+        }
+        break;
+    }
     case WM_INITDIALOG: {
```

```diff
#region DesktopSettingsDlgProc
--- SourceFiles/TaskbarProperties.cpp
+++ SourceFiles/TaskbarProperties.cpp
@@ -1700,5 +1700,22 @@
 INT_PTR CALLBACK DesktopSettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
     static ULONG_PTR gdiplusToken = 0;
     switch (uMsg) {
+    case WM_DESTROY: {
+        int previewIds[] = {
+            IDC_DESKTOP_ICON_PREVIEW_1,
+            IDC_DESKTOP_ICON_PREVIEW_2,
+            IDC_DESKTOP_ICON_PREVIEW_3,
+            IDC_DESKTOP_ICON_PREVIEW_4
+        };
+        for (int i = 0; i < 4; i++) {
+            HICON hIcon = (HICON)SendDlgItemMessageW(hwndDlg, previewIds[i], STM_GETIMAGE, IMAGE_ICON, 0);
+            if (hIcon) {
+                DestroyIcon(hIcon);
+            }
+        }
+        if (gdiplusToken) {
+            Gdiplus::GdiplusShutdown(gdiplusToken);
+            gdiplusToken = 0;
+        }
+        break;
+    }
     case WM_INITDIALOG: {
```

### C. Build Reordering and Signing Fix (`build.ps1` & `build_sign.ps1`)

#### In `build.ps1`:

Move Win32Explorer and EliteStartMenu compilation inside the `try` block and before `build_sign.ps1` runs:

```diff
--- build.ps1
+++ build.ps1
@@ -150,22 +150,47 @@
 
 if (-not $failed) {
     try {
         & "$ScriptDir\build_settings.ps1" -SourceDir $SourceDir -BuildDir $BuildDir -BuildDirx86 $BuildDirx86 -VsDevCmd $vsDevCmd
     } catch {
         Write-Error "Settings Build failed: $_"
         $failed = $true
     }
 }
 
+if (-not $failed) {
+    try {
+        Write-Host "Building Win32Explorer..." -ForegroundColor Cyan
+        $origDir = Get-Location
+        Set-Location "$ScriptDir\Win32Explorer_26.0.3.0"
+        
+        & ".\build_Win32Explorer.ps1" -Platform "x64"
+        if ($LASTEXITCODE -ne 0) { throw "Win32Explorer x64 build failed" }
+        Start-Sleep -Seconds 2
+        
+        & ".\build_Win32Explorer.ps1" -Platform "Win32"
+        if ($LASTEXITCODE -ne 0) { throw "Win32Explorer Win32 build failed" }
+        
+        Set-Location $origDir
+    } catch {
+        Write-Error "Win32Explorer Build failed: $_"
+        $failed = $true
+        if ($origDir) { Set-Location $origDir }
+    }
+}
+
+if (-not $failed -and (Test-Path "$ScriptDir\EliteStartMenu.ps1")) {
+    try {
+        Write-Host 'Compiling EliteStartMenu...' -ForegroundColor Cyan
+        Invoke-ps2exe -inputFile "$ScriptDir\EliteStartMenu.ps1" -outputFile "$BuildDir\EliteStartMenu.exe" -noConsole -STA -iconFile "$ScriptDir\Resources\PREFERENCES.ico"
+        Invoke-ps2exe -inputFile "$ScriptDir\EliteStartMenu.ps1" -outputFile "$BuildDirx86\EliteStartMenu.exe" -noConsole -STA -iconFile "$ScriptDir\Resources\PREFERENCES.ico" -x86
+    } catch {
+        Write-Error "EliteStartMenu Build failed: $_"
+        $failed = $true
+    }
+}
+
 if ($failed) {
     Write-Host "Build failed! Cleaning up recent backup..." -ForegroundColor Red
     $latestBackup = Get-ChildItem -Path (Join-Path $ScriptDir "Backups") -Filter *.cab | Sort-Object LastWriteTime -Descending | Select-Object -First 1
     if ($latestBackup -and (New-TimeSpan -Start $latestBackup.LastWriteTime -End (Get-Date)).TotalMinutes -lt 5) {
         Remove-Item $latestBackup.FullName -Force
         Write-Host "Deleted backup ($($latestBackup.Name)) because the build failed." -ForegroundColor Yellow
     }
     Write-Error "Compilation failed!"
     exit 1
 }
 
 if (-not $failed) {
     $Suffix = (Get-Random).ToString()
     if (Test-Path "$ScriptDir\EliteTaskbar.exe") { Rename-Item "$ScriptDir\EliteTaskbar.exe" "EliteTaskbar_old_$Suffix.exe" -Force -ErrorAction SilentlyContinue }
     if (Test-Path "$ScriptDir\EliteSettings.exe") { Rename-Item "$ScriptDir\EliteSettings.exe" "EliteSettings_old_$Suffix.exe" -Force -ErrorAction SilentlyContinue }
     if (Test-Path "$ScriptDir\EliteSettings.cpl") { Rename-Item "$ScriptDir\EliteSettings.cpl" "EliteSettings_old_$Suffix.cpl" -Force -ErrorAction SilentlyContinue }
     if (Test-Path "$ScriptDir\EliteEverything.exe") { Rename-Item "$ScriptDir\EliteEverything.exe" "EliteEverything_old_$Suffix.exe" -Force -ErrorAction SilentlyContinue }
     if (Test-Path "$ScriptDir\EliteDLLScanner.exe") { Rename-Item "$ScriptDir\EliteDLLScanner.exe" "EliteDLLScanner_old_$Suffix.exe" -Force -ErrorAction SilentlyContinue }
+    if (Test-Path "$ScriptDir\Win32Explorer.exe") { Rename-Item "$ScriptDir\Win32Explorer.exe" "Win32Explorer_old_$Suffix.exe" -Force -ErrorAction SilentlyContinue }
 
     Copy-Item "$BuildDir\EliteTaskbar.exe" "$ScriptDir\EliteTaskbar.exe" -Force
     Copy-Item "$BuildDir\EliteSettings.exe" "$ScriptDir\EliteSettings.exe" -Force
     Copy-Item "$BuildDir\EliteSettings.cpl" "$ScriptDir\EliteSettings.cpl" -Force
     Copy-Item "$BuildDir\EliteEverything.exe" "$ScriptDir\EliteEverything.exe" -Force
     Copy-Item "$BuildDir\EliteDLLScanner.exe" "$ScriptDir\EliteDLLScanner.exe" -Force
+    Copy-Item "$BuildDir\Win32Explorer.exe" "$ScriptDir\Win32Explorer.exe" -Force
+
+    # Run the separate signing stage to sign all completed binaries (including Win32Explorer and EliteStartMenu)
+    & "$ScriptDir\build_sign.ps1" -BuildDir $BuildDir -BuildDirx86 $BuildDirx86
-
-    # Run the separate signing stage
-    & "$ScriptDir\build_sign.ps1" -BuildDir $BuildDir -BuildDirx86 $BuildDirx86
-    
-    Write-Host "Building Win32Explorer..." -ForegroundColor Cyan
-    $origDir = Get-Location
-    
-    Set-Location "$ScriptDir\Win32Explorer_26.0.3.0"
-    & ".\build_Win32Explorer.ps1" -Platform "x64"
-    if ($LASTEXITCODE -ne 0) {
-        Write-Error "Win32Explorer x64 build failed!"
-        Set-Location $origDir
-        exit 1
-    }
-    Start-Sleep -Seconds 2
-    & ".\build_Win32Explorer.ps1" -Platform "Win32"
-    if ($LASTEXITCODE -ne 0) {
-        Write-Error "Win32Explorer Win32 build failed!"
-        Set-Location $origDir
-        exit 1
-    }
-    Set-Location $origDir
-    
-    # Relocate x64 artifact to root for developer execution
-    if (Test-Path "$ScriptDir\Win32Explorer.exe") { Rename-Item "$ScriptDir\Win32Explorer.exe" "Win32Explorer_old_$Suffix.exe" -Force -ErrorAction SilentlyContinue }
-    Copy-Item "$BuildDir\Win32Explorer.exe" "$ScriptDir\Win32Explorer.exe" -Force
-    
     Write-Host "Cleaning up old executables and control panel files..." -ForegroundColor Cyan
     $cleanupPaths = @($PSScriptRoot, "$PSScriptRoot\BuildOutput", "$PSScriptRoot\BuildOutputx86")
     foreach ($path in $cleanupPaths) {
         if (Test-Path $path) {
             Get-ChildItem -Path $path -File -ErrorAction SilentlyContinue | Where-Object {
                 $_.Name -like "*old*.exe" -or $_.Name -like "*Old*.exe" -or $_.Name -like "*old*.cpl" -or $_.Name -like "*Old*.cpl"
             } | Remove-Item -Force -ErrorAction SilentlyContinue
         }
     }
     
     Write-Host "Auto-committing submodules and main repository..." -ForegroundColor Cyan
     $ErrorActionPreference = 'Continue'
     
     Get-ChildItem -Path $PSScriptRoot -Filter ".git" -Recurse -Directory -Force -ErrorAction SilentlyContinue | ForEach-Object {
         $repoPath = $_.Parent.FullName
         if ($repoPath -ne $PSScriptRoot) {
             git -C $repoPath add .
             git -C $repoPath commit -m "Auto-commit after successful build (build.ps1)"
         }
     }
     
     git add .
     git commit -m "Auto-commit after successful build (build.ps1)"
     # git push origin HEAD
     $ErrorActionPreference = 'Stop'
     
     Write-Host "Done!" -ForegroundColor Green
 }
 
-if (Test-Path EliteStartMenu.ps1) {
-    Write-Host 'Compiling EliteStartMenu...' -ForegroundColor Cyan
-    Invoke-ps2exe -inputFile EliteStartMenu.ps1 -outputFile BuildOutput\EliteStartMenu.exe -noConsole -STA -iconFile Resources\PREFERENCES.ico
-    Invoke-ps2exe -inputFile EliteStartMenu.ps1 -outputFile BuildOutputx86\EliteStartMenu.exe -noConsole -STA -iconFile Resources\PREFERENCES.ico -x86
-}
-
 } finally {
```

#### In `build_sign.ps1`:

Update the list of executables to sign to include the missing Win32Explorer and EliteStartMenu binaries:

```diff
--- build_sign.ps1
+++ build_sign.ps1
@@ -10,17 +10,22 @@
 $exeFiles = @(
     (Join-Path $PSScriptRoot "EliteTaskbar.exe"),
     (Join-Path $PSScriptRoot "EliteSettings.exe"),
     (Join-Path $PSScriptRoot "EliteSettings.cpl"),
     (Join-Path $PSScriptRoot "EliteEverything.exe"),
     (Join-Path $PSScriptRoot "EliteDLLScanner.exe"),
+    (Join-Path $PSScriptRoot "Win32Explorer.exe"),
     (Join-Path $BuildDir "EliteTaskbar.exe"),
     (Join-Path $BuildDir "EliteSettings.exe"),
     (Join-Path $BuildDir "EliteSettings.cpl"),
     (Join-Path $BuildDir "EliteEverything.exe"),
     (Join-Path $BuildDir "EliteDLLScanner.exe"),
+    (Join-Path $BuildDir "Win32Explorer.exe"),
+    (Join-Path $BuildDir "EliteStartMenu.exe"),
     (Join-Path $BuildDirx86 "EliteTaskbar_x86.exe"),
     (Join-Path $BuildDirx86 "EliteSettings_x86.exe"),
     (Join-Path $BuildDirx86 "EliteSettings_x86.cpl"),
     (Join-Path $BuildDirx86 "EliteEverything_x86.exe"),
-    (Join-Path $BuildDirx86 "EliteDLLScanner_x86.exe")
+    (Join-Path $BuildDirx86 "EliteDLLScanner_x86.exe"),
+    (Join-Path $BuildDirx86 "Win32Explorer_x86.exe"),
+    (Join-Path $BuildDirx86 "EliteStartMenu.exe")
 )
```

---

## 6. Verification Method

To verify these changes independently:

### A. Static Control Icons and GDI+ Leaks:
1. Compile the CPL settings applet.
2. Launch `EliteSettings.exe` (or control panel settings).
3. Open a process inspector (such as Process Explorer or Task Manager with GDI objects and Handles columns enabled).
4. Select the "Desktop" page/tab, switch tabs, and close the dialog.
5. Confirm that the total handle count and GDI Object/HICON counts do not increase continuously upon opening/closing the settings properties.
6. Verify that `GdiplusShutdown` is invoked exactly matching GDI+ startup by setting a breakpoint in a debugger (Visual Studio) on the shutdown call.

### B. Signing Verification:
1. Run the modified `build.ps1` script to compile and sign the suite.
2. Once complete, verify the digital signature of the following binaries inside `BuildOutput` and `BuildOutputx86` using PowerShell or `signtool.exe`:
   - `BuildOutput\EliteStartMenu.exe`
   - `BuildOutputx86\EliteStartMenu.exe`
   - `BuildOutput\Win32Explorer.exe`
   - `BuildOutputx86\Win32Explorer_x86.exe`
3. Execute:
   ```powershell
   Get-AuthenticodeSignature -FilePath "BuildOutput\EliteStartMenu.exe"
   Get-AuthenticodeSignature -FilePath "BuildOutput\Win32Explorer.exe"
   Get-AuthenticodeSignature -FilePath "BuildOutputx86\EliteStartMenu.exe"
   Get-AuthenticodeSignature -FilePath "BuildOutputx86\Win32Explorer_x86.exe"
   ```
4. Invalidation Condition: If any command output shows `Status: Unsigned` or if signtool returns a signature error, the signing verification has failed.
