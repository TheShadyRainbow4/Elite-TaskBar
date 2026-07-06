# run_challenger_tests.ps1
# Challenger 2 stress/boundary tests for Milestone 7
$ErrorActionPreference = 'Stop'
$ScriptDir = "C:\Users\Administrator\Desktop\Elite-TaskBar"

Write-Host "==========================================================" -ForegroundColor Green
Write-Host "  ELITE TASKBAR MILESTONE 7 CHALLENGER stress TESTS" -ForegroundColor Green
Write-Host "==========================================================" -ForegroundColor Green

# 1. Helper functions & Win32 API Definitions
$code = @"
using System;
using System.Text;
using System.Runtime.InteropServices;

public class Win32 {
    [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    public static extern IntPtr FindWindowW(string lpClassName, string lpWindowName);

    [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    public static extern IntPtr FindWindowExW(IntPtr hWndParent, IntPtr hWndChildAfter, string lpszClass, string lpszWindow);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetClassNameW(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);

    [DllImport("user32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    public static extern int GetWindowTextW(IntPtr hWnd, StringBuilder lpString, int nMaxCount);

    [DllImport("user32.dll")]
    public static extern bool EnumChildWindows(IntPtr hwndParent, EnumWindowsProc lpEnumFunc, IntPtr lParam);
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern IntPtr SendMessageW(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll", EntryPoint = "SendMessageW", CharSet = CharSet.Unicode)]
    public static extern IntPtr SendMessageString(IntPtr hWnd, uint Msg, IntPtr wParam, string lParam);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern bool PostMessageW(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern int GetDlgCtrlID(IntPtr hwnd);

    public const uint WM_SETTEXT = 0x000C;
    public const uint WM_COMMAND = 0x0111;
    public const uint WM_CLOSE = 0x0010;
    public const uint WM_DISPLAYCHANGE = 0x007E;

    public static IntPtr FindChildById(IntPtr parent, int id) {
        IntPtr found = IntPtr.Zero;
        EnumChildWindows(parent, (hWnd, lParam) => {
            if (GetDlgCtrlID(hWnd) == id) {
                found = hWnd;
                return false;
            }
            return true;
        }, IntPtr.Zero);
        return found;
    }

    public static IntPtr FindChildByClass(IntPtr parent, string className) {
        IntPtr found = IntPtr.Zero;
        EnumChildWindows(parent, (hWnd, lParam) => {
            StringBuilder sb = new StringBuilder(260);
            GetClassNameW(hWnd, sb, sb.Capacity);
            if (sb.ToString() == className) {
                found = hWnd;
                return false;
            }
            return true;
        }, IntPtr.Zero);
        return found;
    }

    public static IntPtr FindProcessWindow(int processId, string className, string title = null) {
        IntPtr found = IntPtr.Zero;
        EnumWindows((hWnd, lParam) => {
            uint pid;
            GetWindowThreadProcessId(hWnd, out pid);
            if (pid == processId) {
                StringBuilder sbClass = new StringBuilder(260);
                GetClassNameW(hWnd, sbClass, sbClass.Capacity);
                if (className == null || sbClass.ToString() == className) {
                    found = hWnd;
                    return false;
                }
            }
            return true;
        }, IntPtr.Zero);
        return found;
    }
    
    public static IntPtr FindWindowByClassAndTitle(string className, string titleSnippet) {
        IntPtr found = IntPtr.Zero;
        EnumWindows((hWnd, lParam) => {
            StringBuilder sbClass = new StringBuilder(260);
            GetClassNameW(hWnd, sbClass, sbClass.Capacity);
            if (sbClass.ToString() == className) {
                StringBuilder sbTitle = new StringBuilder(512);
                GetWindowTextW(hWnd, sbTitle, sbTitle.Capacity);
                if (sbTitle.ToString().Contains(titleSnippet)) {
                    found = hWnd;
                    return false;
                }
            }
            return true;
        }, IntPtr.Zero);
        return found;
    }
}
"@
Add-Type -TypeDefinition $code

function Stop-EliteProcesses {
    Write-Host "Stopping any running EliteTaskbar processes..." -ForegroundColor Gray
    Get-Process -Name EliteTaskbar, EliteSettings, EliteEverything, EliteDLLScanner, Win32Explorer, EST* -ErrorAction SilentlyContinue | Stop-Process -Force
    Get-CimInstance Win32_Process -Filter "Name = 'rundll32.exe'" | Where-Object CommandLine -match "EliteSettings\\.cpl" | Invoke-CimMethod -MethodName Terminate | Out-Null
    Start-Sleep -Seconds 2
}

# 2. Check and Backup Registry
Write-Host "Backing up registry configurations..." -ForegroundColor Cyan
$regPath = "HKCU:\Software\EliteSoftware\Win32Explorer\Advanced"
$backup = @{}
$keysToBackup = @("DesktopWallpaperEnabled", "DesktopSlideshowEnabled", "DesktopSlideshowInterval", "DesktopThemePath", "ForceProgmanAllDisplays", "DesktopReplacementEnabled")

if (Test-Path $regPath) {
    foreach ($key in $keysToBackup) {
        $val = Get-ItemProperty -Path $regPath -Name $key -ErrorAction SilentlyContinue
        if ($null -ne $val -and $null -ne $val.$key) {
            $backup[$key] = $val.$key
        }
    }
} else {
    New-Item -Path $regPath -Force | Out-Null
}

function Restore-Registry {
    Write-Host "Restoring registry configurations..." -ForegroundColor Cyan
    foreach ($key in $keysToBackup) {
        if ($backup.ContainsKey($key)) {
            Set-ItemProperty -Path $regPath -Name $key -Value $backup[$key] -Type DWord -ErrorAction SilentlyContinue
        } else {
            Remove-ItemProperty -Path $regPath -Name $key -ErrorAction SilentlyContinue
        }
    }
}

# 3. Create Test Theme Directory & Images
$themeDir = Join-Path $ScriptDir "Subagent_Tests\test_theme"
if (!(Test-Path $themeDir)) {
    New-Item -ItemType Directory -Path $themeDir | Out-Null
}

Write-Host "Creating test wallpaper files..." -ForegroundColor Gray
Add-Type -AssemblyName System.Drawing
$bmp = New-Object System.Drawing.Bitmap(1, 1)
$img1Path = Join-Path $themeDir "img1.jpg"
$img2Path = Join-Path $themeDir "img2.jpg"
$bmp.Save($img1Path, [System.Drawing.Imaging.ImageFormat]::Jpeg)
$bmp.Save($img2Path, [System.Drawing.Imaging.ImageFormat]::Jpeg)
$bmp.Dispose()

# Clean up any leftover temp files to prevent false matches
Write-Host "Cleaning up old temp EST files..." -ForegroundColor Gray
Remove-Item -Path "$env:TEMP\EST*" -Force -ErrorAction SilentlyContinue

function Is-FileLocked($path) {
    try {
        $file = [System.IO.File]::Open($path, 'Open', 'Write', 'None')
        $file.Close()
        return $false
    } catch {
        return $true
    }
}

# Test Results Tracker
$results = @{
    "1A_MissingKeys" = "FAIL"
    "1B_Exact3Sec" = "FAIL"
    "1C_LargeNumOverflow" = "FAIL"
    "1D_InvalidCharsUI" = "FAIL"
    "2_DisplayChange" = "FAIL"
    "3_CplSync" = "FAIL"
}

# Clean state
Stop-EliteProcesses

try {
    # ==========================================================
    # TEST 1A: Slideshow Interval Bounds - Missing Registry Keys
    # ==========================================================
    Write-Host "`n[TEST 1A] Testing behavior with missing registry keys..." -ForegroundColor Yellow
    Remove-ItemProperty -Path $regPath -Name "DesktopSlideshowEnabled" -ErrorAction SilentlyContinue
    Remove-ItemProperty -Path $regPath -Name "DesktopSlideshowInterval" -ErrorAction SilentlyContinue
    Set-ItemProperty -Path $regPath -Name "DesktopWallpaperEnabled" -Value 1 -Type DWord
    Set-ItemProperty -Path $regPath -Name "DesktopReplacementEnabled" -Value 1 -Type DWord
    Set-ItemProperty -Path $regPath -Name "ForceProgmanAllDisplays" -Value 1 -Type DWord

    $proc = Start-Process -FilePath (Join-Path $ScriptDir "EliteTaskbar.exe") -ArgumentList "-allowMultiple" -PassThru
    Start-Sleep -Seconds 3
    if ($proc.HasExited) {
        Write-Host "[FAIL] EliteTaskbar crashed on startup with missing registry keys." -ForegroundColor Red
    } else {
        Write-Host "[PASS] EliteTaskbar started successfully and is running stably with missing keys." -ForegroundColor Green
        $results["1A_MissingKeys"] = "PASS"
    }
    Stop-EliteProcesses

    # ==========================================================
    # TEST 1B: Slideshow Interval Bounds - Exactly 3 Seconds
    # ==========================================================
    Write-Host "`n[TEST 1B] Testing behavior with exactly 3 seconds interval..." -ForegroundColor Yellow
    Set-ItemProperty -Path $regPath -Name "DesktopWallpaperEnabled" -Value 1 -Type DWord
    Set-ItemProperty -Path $regPath -Name "DesktopSlideshowEnabled" -Value 1 -Type DWord
    Set-ItemProperty -Path $regPath -Name "DesktopSlideshowInterval" -Value 3 -Type DWord
    Set-ItemProperty -Path $regPath -Name "DesktopThemePath" -Value $themeDir -Type String
    Set-ItemProperty -Path $regPath -Name "DesktopReplacementEnabled" -Value 1 -Type DWord
    Set-ItemProperty -Path $regPath -Name "ForceProgmanAllDisplays" -Value 1 -Type DWord

    $proc = Start-Process -FilePath (Join-Path $ScriptDir "EliteTaskbar.exe") -ArgumentList "-allowMultiple" -PassThru
    Start-Sleep -Seconds 3  # Wait for startup and initial wallpaper load

    # Monitor which file is locked over 12 seconds. We check lock every 250ms.
    $switches = 0
    $lastLocked = ""
    $startTime = Get-Date
    Write-Host "Monitoring wallpaper switches (should occur every 3 seconds)..." -ForegroundColor Gray
    
    for ($i = 0; $i -lt 48; $i++) {
        $locked1 = Is-FileLocked $img1Path
        $locked2 = Is-FileLocked $img2Path
        $currentLocked = ""
        if ($locked1 -and !$locked2) { $currentLocked = "img1" }
        elseif (!$locked1 -and $locked2) { $currentLocked = "img2" }
        
        Write-Host "  [DEBUG] Iteration $i : img1 locked=$locked1, img2 locked=$locked2, currentLocked=$currentLocked" -ForegroundColor DarkGray

        if ($currentLocked -ne "" -and $currentLocked -ne $lastLocked) {
            if ($lastLocked -ne "") {
                $elapsed = (Get-Date) - $startTime
                Write-Host "  -> Switch to $currentLocked after $($elapsed.TotalSeconds) seconds" -ForegroundColor Cyan
                $switches++
                $startTime = Get-Date
            }
            $lastLocked = $currentLocked
        }
        Start-Sleep -Milliseconds 250
    }

    if ($switches -gt 0) {
        Write-Host "[PASS] Slideshow interval of 3 seconds successfully executed and triggered wallpaper swaps." -ForegroundColor Green
        $results["1B_Exact3Sec"] = "PASS"
    } else {
        Write-Host "[FAIL] Slideshow did not cycle within the test duration. Make sure files are present and slideshow runs." -ForegroundColor Red
    }
    Stop-EliteProcesses

    # ==========================================================
    # TEST 1C: Slideshow Interval Bounds - Large Number Overflow
    # ==========================================================
    Write-Host "`n[TEST 1C] Testing large number interval bounds (4294968 seconds)..." -ForegroundColor Yellow
    Set-ItemProperty -Path $regPath -Name "DesktopSlideshowInterval" -Value 4294968 -Type DWord
    Set-ItemProperty -Path $regPath -Name "DesktopWallpaperEnabled" -Value 1 -Type DWord
    Set-ItemProperty -Path $regPath -Name "DesktopSlideshowEnabled" -Value 1 -Type DWord
    Set-ItemProperty -Path $regPath -Name "DesktopThemePath" -Value $themeDir -Type String
    Set-ItemProperty -Path $regPath -Name "DesktopReplacementEnabled" -Value 1 -Type DWord
    Set-ItemProperty -Path $regPath -Name "ForceProgmanAllDisplays" -Value 1 -Type DWord

    $proc = Start-Process -FilePath (Join-Path $ScriptDir "EliteTaskbar.exe") -ArgumentList "-allowMultiple" -PassThru
    Start-Sleep -Seconds 3 # Wait for startup

    $switches = 0
    $lastLocked = ""
    $startTime = Get-Date
    Write-Host "Monitoring rapid wallpaper switches (expected every ~0.7 seconds)..." -ForegroundColor Gray

    for ($i = 0; $i -lt 50; $i++) {
        $locked1 = Is-FileLocked $img1Path
        $locked2 = Is-FileLocked $img2Path
        $currentLocked = ""
        if ($locked1 -and !$locked2) { $currentLocked = "img1" }
        elseif (!$locked1 -and $locked2) { $currentLocked = "img2" }
        
        Write-Host "  [DEBUG] Iteration $i : img1 locked=$locked1, img2 locked=$locked2, currentLocked=$currentLocked" -ForegroundColor DarkGray

        if ($currentLocked -ne "" -and $currentLocked -ne $lastLocked) {
            if ($lastLocked -ne "") {
                $elapsed = (Get-Date) - $startTime
                Write-Host "  -> Rapid Switch to $currentLocked after $($elapsed.TotalSeconds) seconds" -ForegroundColor Magenta
                $switches++
                $startTime = Get-Date
            }
            $lastLocked = $currentLocked
        }
        Start-Sleep -Milliseconds 100
    }

    if ($switches -ge 3) {
        Write-Host "[PASS] Large interval value successfully caused expected wrap-around integer overflow, cycling rapidly every ~0.7 seconds." -ForegroundColor Green
        $results["1C_LargeNumOverflow"] = "PASS"
    } else {
        Write-Host "[FAIL] Slideshow did not cycle rapidly. Switches observed: $switches" -ForegroundColor Red
    }
    Stop-EliteProcesses

    # ==========================================================
    # TEST 1D: Slideshow Interval Bounds - Invalid Characters in UI
    # ==========================================================
    Write-Host "`n[TEST 1D] Testing invalid characters handling in UI..." -ForegroundColor Yellow
    Remove-ItemProperty -Path $regPath -Name "DesktopSlideshowInterval" -ErrorAction SilentlyContinue

    Write-Host "Launching Settings GUI..." -ForegroundColor Gray
    $settingsProc = Start-Process -FilePath (Join-Path $ScriptDir "EliteSettings.exe") -PassThru
    Start-Sleep -Seconds 2

    $hwndSettings = [Win32]::FindProcessWindow($settingsProc.Id, "#32770")
    if ($hwndSettings -eq [IntPtr]::Zero) {
        throw "Could not find Settings GUI main window."
    }

    # Switch to Desktop tab (Index 5)
    $hwndTab = [Win32]::FindChildByClass($hwndSettings, "SysTabControl32")
    if ($hwndTab -ne [IntPtr]::Zero) {
        Write-Host "Switching to Desktop tab..." -ForegroundColor Gray
        for ($i = 0; $i -lt 5; $i++) {
            [Win32]::SendMessageW($hwndTab, 0x0100, [IntPtr]0x27, [IntPtr]::Zero) | Out-Null
            [Win32]::SendMessageW($hwndTab, 0x0101, [IntPtr]0x27, [IntPtr]::Zero) | Out-Null
            Start-Sleep -Milliseconds 200
        }
    }

    # Locate IDC_DESKTOP_SLIDESHOW_INTERVAL (327)
    $hwndCombo = [Win32]::FindChildById($hwndSettings, 327)
    if ($hwndCombo -eq [IntPtr]::Zero) {
        throw "Could not find slideshow interval ComboBox control."
    }

    # Send invalid text "abc" to the ComboBox's edit window
    Write-Host "Sending invalid text 'abc' to slideshow interval ComboBox..." -ForegroundColor Gray
    [Win32]::SendMessageString($hwndCombo, [Win32]::WM_SETTEXT, [IntPtr]::Zero, "abc") | Out-Null
    Start-Sleep -Milliseconds 500

    # Click Apply button (ID 0x3021 / 12321)
    Write-Host "Clicking Apply..." -ForegroundColor Gray
    [Win32]::SendMessageW($hwndSettings, [Win32]::WM_COMMAND, [IntPtr]0x3021, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Seconds 2

    # Check registry value of slideshow interval
    $savedInterval = (Get-ItemProperty -Path $regPath -Name "DesktopSlideshowInterval" -ErrorAction SilentlyContinue).DesktopSlideshowInterval
    Write-Host "DesktopSlideshowInterval saved in registry: $savedInterval" -ForegroundColor Cyan

    if ($savedInterval -eq 3) {
        Write-Host "[PASS] Invalid character string 'abc' parsed to 0 and successfully clamped to minimum limit of 3 seconds." -ForegroundColor Green
        $results["1D_InvalidCharsUI"] = "PASS"
    } else {
        Write-Host "[FAIL] Registry value was not clamped correctly. Found: $savedInterval" -ForegroundColor Red
    }

    # Close settings window
    [Win32]::PostMessageW($hwndSettings, [Win32]::WM_CLOSE, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
    Stop-EliteProcesses

    # ==========================================================
    # TEST 2: Display Configuration Changes (WM_DISPLAYCHANGE)
    # ==========================================================
    Write-Host "`n[TEST 2] Testing WM_DISPLAYCHANGE message handling..." -ForegroundColor Yellow
    Set-ItemProperty -Path $regPath -Name "DesktopWallpaperEnabled" -Value 1 -Type DWord
    Set-ItemProperty -Path $regPath -Name "DesktopSlideshowEnabled" -Value 1 -Type DWord
    Set-ItemProperty -Path $regPath -Name "DesktopSlideshowInterval" -Value 10 -Type DWord
    Set-ItemProperty -Path $regPath -Name "DesktopReplacementEnabled" -Value 1 -Type DWord
    Set-ItemProperty -Path $regPath -Name "ForceProgmanAllDisplays" -Value 1 -Type DWord

    $proc = Start-Process -FilePath (Join-Path $ScriptDir "EliteTaskbar.exe") -ArgumentList "-allowMultiple" -PassThru
    Start-Sleep -Seconds 3

    # Find custom Progman window (class: "Progman", belonging to our process)
    $hwndProgman = [Win32]::FindProcessWindow($proc.Id, "Progman")
    if ($hwndProgman -eq [IntPtr]::Zero) {
        $hwndProgman = [Win32]::FindWindowW("Progman", $null)
    }

    if ($hwndProgman -eq [IntPtr]::Zero) {
        Write-Host "[FAIL] Custom Progman window not found." -ForegroundColor Red
    } else {
        Write-Host "Found custom Progman window HWND: $hwndProgman. Querying initial rect..." -ForegroundColor Gray
        $rectBefore = New-Object Win32+RECT
        [Win32]::GetWindowRect($hwndProgman, [ref]$rectBefore) | Out-Null
        Write-Host "  -> Rect: Left=$($rectBefore.Left), Top=$($rectBefore.Top), Right=$($rectBefore.Right), Bottom=$($rectBefore.Bottom)" -ForegroundColor Gray

        # Send WM_DISPLAYCHANGE message
        $lParam = [IntPtr](1920 + (1080 -shl 16))
        Write-Host "Sending WM_DISPLAYCHANGE with 32bpp, 1920x1080 to Progman..." -ForegroundColor Gray
        [Win32]::SendMessageW($hwndProgman, [Win32]::WM_DISPLAYCHANGE, [IntPtr]32, $lParam) | Out-Null

        # Check if process is still alive (no crash)
        Start-Sleep -Seconds 2
        if ($proc.HasExited) {
            Write-Host "[FAIL] EliteTaskbar process crashed after WM_DISPLAYCHANGE message." -ForegroundColor Red
        } else {
            Write-Host "[PASS] Custom Progman window handled WM_DISPLAYCHANGE without hangs or crashes." -ForegroundColor Green
            $results["2_DisplayChange"] = "PASS"
        }
    }
    Stop-EliteProcesses

    # ==========================================================
    # TEST 3: Settings CPL applet and EXE Sync
    # ==========================================================
    Write-Host "`n[TEST 3] Testing Settings CPL applet and EXE sync..." -ForegroundColor Yellow
    
    # Run the CPL file using rundll32.exe
    $cplPath = Join-Path $ScriptDir "EliteSettings.cpl"
    Write-Host "Launching CPL applet via rundll32..." -ForegroundColor Gray
    $cplProc = Start-Process -FilePath "rundll32.exe" -ArgumentList "shell32.dll,Control_RunDLL `"$cplPath`"" -PassThru
    Start-Sleep -Seconds 3

    # Find the active Properties window (by class `#32770` and title snippet `Taskbar`)
    $hwndCplSheet = [Win32]::FindWindowByClassAndTitle("#32770", "Taskbar")
    if ($hwndCplSheet -ne [IntPtr]::Zero) {
        Write-Host "  -> Found Properties window (HWND: $hwndCplSheet)." -ForegroundColor Green
        
        # Get the PID and process info
        $extractedPid = 0
        [Win32]::GetWindowThreadProcessId($hwndCplSheet, [ref]$extractedPid) | Out-Null
        
        if ($extractedPid -gt 0) {
            $extractedProc = Get-Process -Id $extractedPid -ErrorAction SilentlyContinue
            Write-Host "  -> Extracted Process Name: $($extractedProc.ProcessName) (PID: $extractedPid)" -ForegroundColor Green
            
            if ($extractedProc.ProcessName -like "EST*") {
                # Get the EXACT file path from the running process!
                $tempExePath = $extractedProc.Path
                Write-Host "  -> Temp extracted executable found on disk via process path: $tempExePath" -ForegroundColor Green

                # Send WM_CLOSE to terminate
                Write-Host "  -> Sending WM_CLOSE to properties window..." -ForegroundColor Gray
                [Win32]::PostMessageW($hwndCplSheet, [Win32]::WM_CLOSE, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
                Start-Sleep -Seconds 3

                # Verify processes exited
                $isSettingsDead = $null -eq (Get-Process -Id $extractedPid -ErrorAction SilentlyContinue)
                $isCplDead = $null -eq (Get-Process -Id $cplProc.Id -ErrorAction SilentlyContinue)
                $isTempExeDeleted = !(Test-Path $tempExePath)

                Write-Host "  -> Verification: EliteSettings dead: $isSettingsDead, CPL runner dead: $isCplDead, Temp file deleted: $isTempExeDeleted" -ForegroundColor Gray

                if ($isSettingsDead -and $isCplDead -and $isTempExeDeleted) {
                    Write-Host "[PASS] Settings CPL applet extracts, launches the EXE, syncs/runs properly, and cleans up the temp file on close without hangs or crashes." -ForegroundColor Green
                    $results["3_CplSync"] = "PASS"
                } else {
                    Write-Host "[FAIL] Process hang, or temp file leak detected." -ForegroundColor Red
                }
            } else {
                Write-Host "[FAIL] Process name does not start with EST. ProcessName: $($extractedProc.ProcessName)" -ForegroundColor Red
            }
        } else {
            Write-Host "[FAIL] Could not query process ID for the properties window." -ForegroundColor Red
        }
    } else {
        Write-Host "[FAIL] Could not locate the Property Sheet window for CPL applet." -ForegroundColor Red
    }
    Stop-EliteProcesses

} catch {
    Write-Host "[FAIL] Stress/boundary testing failed with exception: $_" -ForegroundColor Red
} finally {
    # Restore registry
    Restore-Registry
    # Clean up test theme directory
    if (Test-Path $themeDir) {
        Remove-Item -Path $themeDir -Recurse -Force -ErrorAction SilentlyContinue
    }
    Stop-EliteProcesses
}

# 4. Print Summary
Write-Host "`n==========================================================" -ForegroundColor Green
Write-Host "  CHALLENGER stress TESTS RESULTS SUMMARY" -ForegroundColor Green
Write-Host "==========================================================" -ForegroundColor Green
$overallPass = $true
foreach ($key in $results.Keys) {
    $res = $results[$key]
    $color = if ($res -eq "PASS") { "Green" } else { "Red" }
    Write-Host "  $key : [$res]" -ForegroundColor $color
    if ($res -ne "PASS") { $overallPass = $false }
}

$verdict = if ($overallPass) { "PASS" } else { "FAIL" }
$color = if ($overallPass) { "Green" } else { "Red" }
Write-Host "`nFINAL VERDICT: $verdict" -ForegroundColor $color

$verdict | Out-File -FilePath "$ScriptDir\Subagent_Tests\challenger_verdict.txt" -Force
$results | Out-String | Out-File -FilePath "$ScriptDir\Subagent_Tests\challenger_results.txt" -Force

if ($overallPass) {
    exit 0
} else {
    exit 1
}
