# Handoff Report — explorer_m6_3

## 1. Observation

### Reload/Restart Trigger Mechanism
In `SourceFiles/TaskbarProperties.cpp` (lines 470–502) and its mirrored copy `Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/TaskbarProperties.cpp` (lines 470–502), the functions `NotifySettingsChange()` and `BroadcastSettingsChangeThread()` are defined as follows:

```cpp
DWORD WINAPI BroadcastSettingsChangeThread(LPVOID lpParam) {
    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"TraySettings", SMTO_ABORTIFHUNG, 5000, NULL);
    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"EliteTaskbarSettings", SMTO_ABORTIFHUNG, 500, NULL);
    
    // Aggressively restart the apps to ensure all settings are applied safely and cleanly
    WCHAR exePath[MAX_PATH] = {0};
    bool pathResolved = false;
    if (__argc > 1 && __wargv[1] != NULL) {
        DWORD dwAttrs = GetFileAttributesW(__wargv[1]);
        if (dwAttrs != INVALID_FILE_ATTRIBUTES && (dwAttrs & FILE_ATTRIBUTE_DIRECTORY)) {
            wcscpy_s(exePath, MAX_PATH, __wargv[1]);
            pathResolved = true;
        }
    }
    if (!pathResolved) {
        GetModuleFileNameW(g_hInstance, exePath, MAX_PATH);
        PathRemoveFileSpecW(exePath); // Get directory of the current settings CPL/EXE
    }

    wchar_t psCmd[2048];
    swprintf_s(psCmd, L"-NoProfile -WindowStyle Hidden -Command \"Stop-Process -Name EliteTaskbar -Force; Stop-Process -Name Win32Explorer -Force; Start-Sleep -Milliseconds 500; Start-Process -FilePath '%s\\EliteTaskbar.exe' -ErrorAction SilentlyContinue; Start-Process -FilePath '%s\\Win32Explorer.exe' -ErrorAction SilentlyContinue\"", exePath, exePath);

    ShellExecuteW(NULL, NULL, L"powershell.exe", psCmd, NULL, SW_HIDE);
        
    return 0;
}

void NotifySettingsChange() {
    HANDLE hThread = CreateThread(NULL, 0, BroadcastSettingsChangeThread, NULL, 0, NULL);
    if (hThread) {
        CloseHandle(hThread);
    }
}
```

### Generation of `old.exe` and `old.cpl` Files
In `build.ps1` (lines 154–159 and line 190), files are renamed to include a random suffix bypass lock strategy before copying the newly compiled binaries:

```powershell
    $Suffix = (Get-Random).ToString()
    if (Test-Path "$ScriptDir\EliteTaskbar.exe") { Rename-Item "$ScriptDir\EliteTaskbar.exe" "EliteTaskbar_old_$Suffix.exe" -Force -ErrorAction SilentlyContinue }
    if (Test-Path "$ScriptDir\EliteSettings.exe") { Rename-Item "$ScriptDir\EliteSettings.exe" "EliteSettings_old_$Suffix.exe" -Force -ErrorAction SilentlyContinue }
    if (Test-Path "$ScriptDir\EliteSettings.cpl") { Rename-Item "$ScriptDir\EliteSettings.cpl" "EliteSettings_old_$Suffix.cpl" -Force -ErrorAction SilentlyContinue }
    if (Test-Path "$ScriptDir\EliteEverything.exe") { Rename-Item "$ScriptDir\EliteEverything.exe" "EliteEverything_old_$Suffix.exe" -Force -ErrorAction SilentlyContinue }
    if (Test-Path "$ScriptDir\EliteDLLScanner.exe") { Rename-Item "$ScriptDir\EliteDLLScanner.exe" "EliteDLLScanner_old_$Suffix.exe" -Force -ErrorAction SilentlyContinue }
...
    if (Test-Path "$ScriptDir\Win32Explorer.exe") { Rename-Item "$ScriptDir\Win32Explorer.exe" "Win32Explorer_old_$Suffix.exe" -Force -ErrorAction SilentlyContinue }
```
These renamed files remain in the workspace root after compilation completes.

---

## 2. Logic Chain

### Why multiple Win32Explorer windows open:
1. The Settings UI is a Property Sheet dialog containing 7 active tabs/pages (initialized using `CreatePropertySheetPageW`).
2. When the user clicks the "Apply" or "Okay" button, Windows dispatches `PSN_APPLY` to all initialized pages in the property sheet synchronously on the GUI thread.
3. Every page's dialog proc handles `PSN_APPLY` and subsequently calls `NotifySettingsChange()`.
4. As a result, `NotifySettingsChange()` is called multiple times (once per initialized page) in rapid succession.
5. Each call to `NotifySettingsChange()` spawns a new background thread executing `BroadcastSettingsChangeThread`.
6. These threads execute concurrently, running parallel `powershell.exe` command instances.
7. The concurrent PowerShell instances execute `Stop-Process` and `Start-Process` commands almost simultaneously. Due to the race condition, multiple instances of `Win32Explorer.exe` are launched.
8. While `EliteTaskbar.exe` limits itself to a single instance via mutex locks (`EliteTaskbar_Instance_Mutex` in `main.cpp`), `Win32Explorer.exe` acts as a multi-window shell explorer/file manager and opens a new browser window for each invocation, causing multiple Win32Explorer windows to open simultaneously.

### Solution for multiple windows:
- Debounce `NotifySettingsChange()` using a static variable to track the last trigger time (using `GetTickCount64()`). If subsequent settings change calls occur less than 1000ms apart, ignore them. Since the synchronous `PSN_APPLY` notifications to all pages occur within microseconds, only the first page will successfully trigger the reload/restart, and the subsequent pages will be safely ignored.

### Solution for cleaning up old executables:
- In `build.ps1`, the processes are terminated prior to compilation, meaning the file locks are released during build/copy. Once the new binaries are successfully migrated, any `*old*.exe`, `*Old*.exe`, `*old*.cpl`, and `*Old*.cpl` files left in `$ScriptDir` (workspace root), `$BuildDir` (`BuildOutput`), and `$BuildDirx86` (`BuildOutputx86`) are unlocked and can be safely deleted. Adding a wildcard deletion step at the end of the script before completion is safe and robust.

---

## 3. Caveats

- We assume `GetTickCount64()` is fully supported on the client machine. Since the minimum target OS is Windows Vista / Windows 7, this is fully satisfied (`GetTickCount64` is available on Vista and newer).
- Wildcard file deletion will target any files matching `*old*.exe` or `*Old*.exe` or `*old*.cpl` or `*Old*.cpl` in the workspace root and build output folders. Ensure no critical files are manually named with this pattern inside these directories.

---

## 4. Conclusion

### Fix Strategy

#### Step 1: Debounce `NotifySettingsChange()` in `TaskbarProperties.cpp`
Modify the `NotifySettingsChange` implementation in both:
1. `SourceFiles/TaskbarProperties.cpp`
2. `Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/TaskbarProperties.cpp`

**Proposed Code Change:**
```cpp
void NotifySettingsChange() {
    static ULONGLONG lastTriggerTime = 0;
    ULONGLONG currentTime = GetTickCount64();
    if (currentTime - lastTriggerTime < 1000) {
        return; // Ignore duplicate/rapidly firing notifications (debounce)
    }
    lastTriggerTime = currentTime;

    HANDLE hThread = CreateThread(NULL, 0, BroadcastSettingsChangeThread, NULL, 0, NULL);
    if (hThread) {
        CloseHandle(hThread);
    }
}
```

#### Step 2: Auto-Cleanup of Old Executables in `build.ps1`
Add the following cleanup loop near the end of `build.ps1` (before the final `Write-Host "Done!"` message around line 208):

**Proposed Code Change:**
```powershell
    # Automatically clean up all old / Old stubs and binaries in root and build folders
    Write-Host "Cleaning up old executables..." -ForegroundColor Cyan
    $OldFiles = Get-ChildItem -Path $ScriptDir, $BuildDir, $BuildDirx86 -File -ErrorAction SilentlyContinue | 
        Where-Object { $_.Name -like "*old*.exe" -or $_.Name -like "*Old*.exe" -or $_.Name -like "*old*.cpl" -or $_.Name -like "*Old*.cpl" }
    foreach ($file in $OldFiles) {
        Remove-Item $file.FullName -Force -ErrorAction SilentlyContinue
    }
```

---

## 5. Verification Method

### 1. Verification of the Debounce
1. Build the updated project using `.\build.ps1`.
2. Open `EliteSettings.exe` or apply the control panel settings.
3. Modify settings and click the "Apply" or "Okay" button.
4. Verify that the taskbar and file browser reload, and **exactly one** instance/window of `Win32Explorer.exe` is opened.

### 2. Verification of the Cleanup
1. Run `.\build.ps1`.
2. Check the workspace root directory (`C:\Users\Administrator\Desktop\Elite-TaskBar`), `BuildOutput`, and `BuildOutputx86`.
3. Verify that no `*_old_*.exe` or `*_old_*.cpl` files remain.
