# Subagent_Tests\run_challenger_m6_verification.ps1
# Automated empirical verification script for Challenger M6-2

$ErrorActionPreference = 'Stop'
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Resolve-Path (Join-Path $ScriptDir "..")

Write-Host "==========================================================" -ForegroundColor Green
Write-Host "  ELITE TASKBAR MILESTONE 6 EMPIRICAL CHALLENGER TESTS" -ForegroundColor Green
Write-Host "==========================================================" -ForegroundColor Green

# 1. Compile Win32 Helper APIs
$code = @"
using System;
using System.Text;
using System.Runtime.InteropServices;
using System.Collections.Generic;

public class ChallengerHelper {
    [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    public static extern IntPtr FindWindowW(string lpClassName, string lpWindowName);

    [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    public static extern IntPtr FindWindowExW(IntPtr hWndParent, IntPtr hWndChildAfter, string lpszClass, string lpszWindow);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetClassNameW(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetWindowTextW(IntPtr hWnd, StringBuilder lpString, int nMaxCount);

    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

    [DllImport("user32.dll")]
    public static extern bool GetClientRect(IntPtr hWnd, out RECT lpRect);

    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

    [DllImport("user32.dll")]
    public static extern bool EnumChildWindows(IntPtr hwndParent, EnumChildProc lpEnumFunc, IntPtr lParam);
    public delegate bool EnumChildProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern IntPtr SendMessageW(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern bool PostMessageW(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern int GetDlgCtrlID(IntPtr hwnd);

    [DllImport("user32.dll")]
    public static extern IntPtr GetParent(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool IsWindowVisible(IntPtr hWnd);

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
        public int Width { get { return Right - Left; } }
        public int Height { get { return Bottom - Top; } }
    }

    public static List<IntPtr> GetChildWindows(IntPtr parent) {
        List<IntPtr> result = new List<IntPtr>();
        EnumChildWindows(parent, (hWnd, lParam) => {
            result.Add(hWnd);
            return true;
        }, IntPtr.Zero);
        return result;
    }

    public static IntPtr FindPropSheetWindow(int processId) {
        IntPtr found = IntPtr.Zero;
        EnumWindows((hWnd, lParam) => {
            uint pid;
            GetWindowThreadProcessId(hWnd, out pid);
            if (pid == processId) {
                StringBuilder sbClass = new StringBuilder(260);
                GetClassNameW(hWnd, sbClass, sbClass.Capacity);
                StringBuilder sbTitle = new StringBuilder(260);
                GetWindowTextW(hWnd, sbTitle, sbTitle.Capacity);
                if (sbClass.ToString() == "#32770" && sbTitle.ToString().Contains("Properties")) {
                    found = hWnd;
                    return false;
                }
            }
            return true;
        }, IntPtr.Zero);
        return found;
    }

    public static IntPtr FindAboutDialogWindow(int processId) {
        IntPtr found = IntPtr.Zero;
        EnumWindows((hWnd, lParam) => {
            uint pid;
            GetWindowThreadProcessId(hWnd, out pid);
            if (pid == processId) {
                StringBuilder sbClass = new StringBuilder(260);
                GetClassNameW(hWnd, sbClass, sbClass.Capacity);
                StringBuilder sbTitle = new StringBuilder(260);
                GetWindowTextW(hWnd, sbTitle, sbTitle.Capacity);
                if (sbClass.ToString() == "#32770" && sbTitle.ToString().Contains("About")) {
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
Add-Type -TypeDefinition $code -ErrorAction SilentlyContinue

# Target Registry settings
$regHKCU = "HKCU:\Software\EliteSoftware\Win32Explorer\Advanced"

# Helpers
function Clean-Environment {
    Get-Process -Name EliteSettings, EliteTaskbar, Win32Explorer -ErrorAction SilentlyContinue | Stop-Process -Force
    Start-Sleep -Seconds 2
}

# Clean start
Clean-Environment

# Back up current registry keys
$backupReg = @{}
if (Test-Path $regHKCU) {
    foreach ($name in @("MigrateStartMenuSettings", "FallbackStartMenuEnabled")) {
        $val = (Get-ItemProperty -Path $regHKCU -Name $name -ErrorAction SilentlyContinue).$name
        if ($null -ne $val) { $backupReg[$name] = $val }
    }
} else {
    New-Item -Path $regHKCU -Force | Out-Null
}

# Force MigrateStartMenuSettings = 1 to expose the dynamic scroll controls in the Start Menu tab
Set-ItemProperty -Path $regHKCU -Name "MigrateStartMenuSettings" -Value 1 -Type DWord
Set-ItemProperty -Path $regHKCU -Name "FallbackStartMenuEnabled" -Value 1 -Type DWord

$verdictStartMenu = "FAIL"
$verdictAboutDialog = "FAIL"
$verdictApplyMultiSpawn = "FAIL"
$verdictBuildClean = "FAIL"

try {
    # -------------------------------------------------------------
    # 1. Test: Running the build cleanly removes any *old*.exe and *old*.cpl files.
    # -------------------------------------------------------------
    Write-Host "--- Test 1: Old Files Cleanup ---" -ForegroundColor Yellow
    # Check if any old files remain
    $oldFiles = Get-ChildItem -Path $ProjectRoot -File | Where-Object {
        $_.Name -like "*old*.exe" -or $_.Name -like "*Old*.exe" -or $_.Name -like "*old*.cpl" -or $_.Name -like "*Old*.cpl"
    }
    $oldFilesOut = Get-ChildItem -Path (Join-Path $ProjectRoot "BuildOutput") -File -ErrorAction SilentlyContinue | Where-Object {
        $_.Name -like "*old*.exe" -or $_.Name -like "*Old*.exe" -or $_.Name -like "*old*.cpl" -or $_.Name -like "*Old*.cpl"
    }
    $oldFilesOutx86 = Get-ChildItem -Path (Join-Path $ProjectRoot "BuildOutputx86") -File -ErrorAction SilentlyContinue | Where-Object {
        $_.Name -like "*old*.exe" -or $_.Name -like "*Old*.exe" -or $_.Name -like "*old*.cpl" -or $_.Name -like "*Old*.cpl"
    }

    $totalOld = ($oldFiles -and $oldFiles.Count -or 0) + ($oldFilesOut -and $oldFilesOut.Count -or 0) + ($oldFilesOutx86 -and $oldFilesOutx86.Count -or 0)
    if ($totalOld -eq 0) {
        Write-Host "[PASS] No stale old executables or control panels left." -ForegroundColor Green
        $verdictBuildClean = "PASS"
    } else {
        Write-Host "[FAIL] Found $totalOld stale old files." -ForegroundColor Red
    }

    # -------------------------------------------------------------
    # Launch Settings for UI Verification
    # -------------------------------------------------------------
    $settingsPath = Join-Path $ProjectRoot "EliteSettings.exe"
    Write-Host "Launching EliteSettings from $settingsPath..." -ForegroundColor Cyan
    $proc = Start-Process -FilePath $settingsPath -PassThru
    Start-Sleep -Seconds 3

    if ($proc.HasExited) {
        throw "EliteSettings exited immediately with code $($proc.ExitCode)"
    }

    $hPropSheet = [ChallengerHelper]::FindPropSheetWindow($proc.Id)
    if ($hPropSheet -eq [IntPtr]::Zero) {
        throw "Could not find EliteSettings properties sheet window."
    }
    Write-Host "Found properties sheet HWND: $hPropSheet" -ForegroundColor Green

    # -------------------------------------------------------------
    # 2. Test: Start Menu settings tab works without requiring hover
    # -------------------------------------------------------------
    Write-Host "`n--- Test 2: Start Menu Settings Tab ---" -ForegroundColor Yellow
    # Switch to Start Menu tab (index 1)
    # PSM_SETCURSEL = 0x0465 (1125 decimal, or 0x0400 + 101)
    [ChallengerHelper]::SendMessageW($hPropSheet, 0x0465, [IntPtr]1, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Seconds 1

    # Locate scroll area control
    $childWindows = [ChallengerHelper]::GetChildWindows($hPropSheet)
    $hScroll = [IntPtr]::Zero
    foreach ($hChild in $childWindows) {
        $sbClass = New-Object System.Text.StringBuilder 260
        [ChallengerHelper]::GetClassNameW($hChild, $sbClass, $sbClass.Capacity) | Out-Null
        if ($sbClass.ToString() -eq "EliteDynScrollArea") {
            $hScroll = $hChild
            break
        }
    }

    if ($hScroll -eq [IntPtr]::Zero) {
        Write-Host "[FAIL] EliteDynScrollArea scroll container not found." -ForegroundColor Red
    } else {
        Write-Host "Found scroll container HWND: $hScroll" -ForegroundColor Green
        # Check dynamic child controls inside the scroll container
        $scrollChildren = [ChallengerHelper]::GetChildWindows($hScroll)
        $comboCount = 0
        $staticCount = 0
        foreach ($hChild in $scrollChildren) {
            $sbClass = New-Object System.Text.StringBuilder 260
            [ChallengerHelper]::GetClassNameW($hChild, $sbClass, $sbClass.Capacity) | Out-Null
            $cls = $sbClass.ToString()
            if ($cls -eq "ComboBox") { $comboCount++ }
            elseif ($cls -eq "Static") { $staticCount++ }
        }
        
        Write-Host "Found $comboCount ComboBox controls and $staticCount Static controls in the dynamic scroll container." -ForegroundColor Green
        if ($comboCount -ge 3 -and $staticCount -ge 4) {
            Write-Host "[PASS] Start Menu tab dynamic controls populated and rendered correctly without hover." -ForegroundColor Green
            $verdictStartMenu = "PASS"
        } else {
            Write-Host "[FAIL] Start Menu tab controls underpopulated. ComboBoxes: $comboCount, Statics: $staticCount" -ForegroundColor Red
        }
    }

    # -------------------------------------------------------------
    # 3. Test: About dialog More Info/Less Info resizes correctly and doesn't clip
    # -------------------------------------------------------------
    Write-Host "`n--- Test 3: About Dialog Layout and Clipping ---" -ForegroundColor Yellow
    # Summon About Dialog by posting Command 40002 to PropSheet
    [ChallengerHelper]::PostMessageW($hPropSheet, 0x0111, [IntPtr]40002, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Seconds 2

    $hAbout = [ChallengerHelper]::FindAboutDialogWindow($proc.Id)
    if ($hAbout -eq [IntPtr]::Zero) {
        Write-Host "[FAIL] About Dialog not found." -ForegroundColor Red
    } else {
        Write-Host "Found About Dialog HWND: $hAbout" -ForegroundColor Green
        
        # Check Initial/Collapsed Rect
        $rectColl = New-Object ChallengerHelper+RECT
        [ChallengerHelper]::GetClientRect($hAbout, [ref]$rectColl) | Out-Null
        Write-Host "Collapsed client height: $($rectColl.Height) px (width: $($rectColl.Width) px)" -ForegroundColor Cyan

        # Get control handles
        $hExpand = [IntPtr]::Zero
        $hOk = [IntPtr]::Zero
        $hMoreInfo = [IntPtr]::Zero
        $aboutChildren = [ChallengerHelper]::GetChildWindows($hAbout)
        foreach ($hChild in $aboutChildren) {
            $ctrlId = [ChallengerHelper]::GetDlgCtrlID($hChild)
            if ($ctrlId -eq 282) { $hExpand = $hChild }
            elseif ($ctrlId -eq 1) { $hOk = $hChild }
            elseif ($ctrlId -eq 283) { $hMoreInfo = $hChild }
        }

        if ($hExpand -eq [IntPtr]::Zero -or $hOk -eq [IntPtr]::Zero -or $hMoreInfo -eq [IntPtr]::Zero) {
            Write-Host "[FAIL] Could not locate all expected controls in About dialog." -ForegroundColor Red
        } else {
            # Check button y-coordinates before expansion
            $rectExpBtn = New-Object ChallengerHelper+RECT
            [ChallengerHelper]::GetWindowRect($hExpand, [ref]$rectExpBtn) | Out-Null
            $rectOkBtn = New-Object ChallengerHelper+RECT
            [ChallengerHelper]::GetWindowRect($hOk, [ref]$rectOkBtn) | Out-Null

            # Click Expand
            Write-Host "Clicking 'More Info >>' button..." -ForegroundColor Cyan
            # BM_CLICK = 0x00F5
            [ChallengerHelper]::SendMessageW($hExpand, 0x00F5, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
            Start-Sleep -Seconds 1

            # Check Expanded Rect
            $rectExp = New-Object ChallengerHelper+RECT
            [ChallengerHelper]::GetClientRect($hAbout, [ref]$rectExp) | Out-Null
            Write-Host "Expanded client height: $($rectExp.Height) px" -ForegroundColor Cyan

            # Check if edit box is visible
            $isEditVisible = [ChallengerHelper]::IsWindowVisible($hMoreInfo)
            Write-Host "More Info edit visible: $isEditVisible" -ForegroundColor Cyan

            # Check new button coordinates
            $rectExpBtnNew = New-Object ChallengerHelper+RECT
            [ChallengerHelper]::GetWindowRect($hExpand, [ref]$rectExpBtnNew) | Out-Null
            $rectOkBtnNew = New-Object ChallengerHelper+RECT
            [ChallengerHelper]::GetWindowRect($hOk, [ref]$rectOkBtnNew) | Out-Null

            # Check edit box coordinates
            $rectEdit = New-Object ChallengerHelper+RECT
            [ChallengerHelper]::GetWindowRect($hMoreInfo, [ref]$rectEdit) | Out-Null

            Write-Host "More Info edit bottom Y: $($rectEdit.Bottom)" -ForegroundColor DarkCyan
            Write-Host "Expand button top Y: $($rectExpBtnNew.Top) | Ok button top Y: $($rectOkBtnNew.Top)" -ForegroundColor DarkCyan

            # Verify no clipping / overlap
            # The buttons must be strictly below the edit control
            if ($rectExp.Height -gt $rectColl.Height -and $isEditVisible -and ($rectExpBtnNew.Top -ge $rectEdit.Bottom) -and ($rectOkBtnNew.Top -ge $rectEdit.Bottom)) {
                Write-Host "[PASS] About Dialog resizes correctly and doesn't clip controls when expanded." -ForegroundColor Green
                $verdictAboutDialog = "PASS"
            } else {
                Write-Host "[FAIL] Sizing/layout error. Expanded height must be greater, and buttons must be below the edit control." -ForegroundColor Red
            }

            # Click Less Info
            Write-Host "Clicking 'Less Info <<' button..." -ForegroundColor Cyan
            [ChallengerHelper]::SendMessageW($hExpand, 0x00F5, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
            Start-Sleep -Seconds 1

            $rectColl2 = New-Object ChallengerHelper+RECT
            [ChallengerHelper]::GetClientRect($hAbout, [ref]$rectColl2) | Out-Null
            Write-Host "Collapsed client height after collapse: $($rectColl2.Height) px" -ForegroundColor Cyan

            # Close the About dialog by clicking OK
            Write-Host "Closing About Dialog..." -ForegroundColor Cyan
            [ChallengerHelper]::SendMessageW($hOk, 0x00F5, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
            Start-Sleep -Seconds 1
        }
    }

    # -------------------------------------------------------------
    # 4. Test: Clicking Apply in settings does not spawn multiple Win32Explorer instances
    # -------------------------------------------------------------
    Write-Host "`n--- Test 4: Apply Button Spawning Check ---" -ForegroundColor Yellow
    
    # Get current Win32Explorer process count
    $initProcs = Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue
    $initCount = if ($null -ne $initProcs) { $initProcs.Count } else { 0 }
    Write-Host "Initial running Win32Explorer count: $initCount" -ForegroundColor Cyan

    # Find the Apply button on the property sheet
    $hApply = [IntPtr]::Zero
    $childWindows = [ChallengerHelper]::GetChildWindows($hPropSheet)
    foreach ($hChild in $childWindows) {
        $ctrlId = [ChallengerHelper]::GetDlgCtrlID($hChild)
        if ($ctrlId -eq 0x3021) {
            $hApply = $hChild
            break
        }
    }

    if ($hApply -eq [IntPtr]::Zero) {
        Write-Host "[FAIL] Apply button not found on properties sheet." -ForegroundColor Red
    } else {
        Write-Host "Found Apply button HWND: $hApply. Clicking..." -ForegroundColor Cyan
        [ChallengerHelper]::SendMessageW($hApply, 0x00F5, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
        
        Write-Host "Waiting 5 seconds for Apply logic to run..." -ForegroundColor Cyan
        Start-Sleep -Seconds 5

        $afterProcs = Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue
        $afterCount = if ($null -ne $afterProcs) { $afterProcs.Count } else { 0 }
        Write-Host "Win32Explorer count after settings Apply: $afterCount" -ForegroundColor Cyan

        # The count of Win32Explorer processes should be <= 1 (should not spawn multiple)
        if ($afterCount -le 1) {
            Write-Host "[PASS] Clicking Apply does not spawn multiple Win32Explorer instances." -ForegroundColor Green
            $verdictApplyMultiSpawn = "PASS"
        } else {
            Write-Host "[FAIL] Spawning bug detected! Multiple Win32Explorer instances are running: $afterCount" -ForegroundColor Red
        }
    }

} catch {
    Write-Host "EXCEPTION in verification: $_" -ForegroundColor Red
} finally {
    # Tear down
    Clean-Environment

    # Restore registry defaults
    if ($backupReg.ContainsKey("MigrateStartMenuSettings")) {
        Set-ItemProperty -Path $regHKCU -Name "MigrateStartMenuSettings" -Value $backupReg["MigrateStartMenuSettings"] -Type DWord
    } else {
        Remove-ItemProperty -Path $regHKCU -Name "MigrateStartMenuSettings" -ErrorAction SilentlyContinue
    }
    if ($backupReg.ContainsKey("FallbackStartMenuEnabled")) {
        Set-ItemProperty -Path $regHKCU -Name "FallbackStartMenuEnabled" -Value $backupReg["FallbackStartMenuEnabled"] -Type DWord
    } else {
        Remove-ItemProperty -Path $regHKCU -Name "FallbackStartMenuEnabled" -ErrorAction SilentlyContinue
    }

    $cBuild = if ($verdictBuildClean -eq "PASS") { "Green" } else { "Red" }
    $cStart = if ($verdictStartMenu -eq "PASS") { "Green" } else { "Red" }
    $cAbout = if ($verdictAboutDialog -eq "PASS") { "Green" } else { "Red" }
    $cApply = if ($verdictApplyMultiSpawn -eq "PASS") { "Green" } else { "Red" }

    Write-Host "`n==========================================================" -ForegroundColor Green
    Write-Host "  TESTS RESULTS" -ForegroundColor Green
    Write-Host "==========================================================" -ForegroundColor Green
    Write-Host "1. Old Files Clean-Up    : $verdictBuildClean" -ForegroundColor $cBuild
    Write-Host "2. Start Menu Tab Layout : $verdictStartMenu" -ForegroundColor $cStart
    Write-Host "3. About Dialog Resizing : $verdictAboutDialog" -ForegroundColor $cAbout
    Write-Host "4. Apply Multi-Spawn     : $verdictApplyMultiSpawn" -ForegroundColor $cApply
}

$overall = "FAIL"
if ($verdictBuildClean -eq "PASS" -and $verdictStartMenu -eq "PASS" -and $verdictAboutDialog -eq "PASS" -and $verdictApplyMultiSpawn -eq "PASS") {
    $overall = "PASS"
}
$cOverall = if ($overall -eq "PASS") { "Green" } else { "Red" }
Write-Host "`nOVERALL VERDICT: $overall" -ForegroundColor $cOverall

# Save output files for audit trail
$overall | Set-Content (Join-Path $ScriptDir "verdict.txt")
"1. Old Files Clean-Up    : $verdictBuildClean`r`n2. Start Menu Tab Layout : $verdictStartMenu`r`n3. About Dialog Resizing : $verdictAboutDialog`r`n4. Apply Multi-Spawn     : $verdictApplyMultiSpawn" | Set-Content (Join-Path $ScriptDir "test_results.txt")

$exitCode = if ($overall -eq "PASS") { 0 } else { 1 }
exit $exitCode
