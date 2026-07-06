# run_comprehensive_e2e.ps1
# Automated Empirical Verification & E2E Testing Script for Challenger 1 (Milestone 6)
$ErrorActionPreference = 'Stop'
$ScriptDir = "C:\Users\Administrator\Desktop\Elite-TaskBar"
$MyDir = "C:\Users\Administrator\Desktop\Elite-TaskBar\.agents\challenger_m6_1"

Write-Host "=== EliteTaskbar Milestone 6 Comprehensive E2E Verification ===" -ForegroundColor Cyan

# Load Win32 helper definitions
$code = @"
using System;
using System.Text;
using System.Runtime.InteropServices;

public class Win32Helper {
    [DllImport("user32.dll", SetLastError = true)]
    public static extern IntPtr FindWindowEx(IntPtr hwndParent, IntPtr hwndChildAfter, string lpszClass, string lpszWindow);

    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern IntPtr SendMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern bool EnumChildWindows(IntPtr hwndParent, EnumChildProc lpEnumFunc, IntPtr lParam);

    [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true)]
    public static extern int GetClassName(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);

    [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true)]
    public static extern int GetWindowTextW(IntPtr hWnd, StringBuilder lpString, int nMaxCount);

    [DllImport("user32.dll")]
    public static extern int GetDlgCtrlID(IntPtr hwnd);

    [DllImport("user32.dll")]
    public static extern IntPtr GetParent(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

    [DllImport("user32.dll")]
    public static extern bool GetClientRect(IntPtr hWnd, out RECT lpRect);

    [DllImport("user32.dll")]
    public static extern int MapWindowPoints(IntPtr hWndFrom, IntPtr hWndTo, ref RECT lpPoints, uint cPoints);

    [DllImport("user32.dll")]
    public static extern bool IsWindowVisible(IntPtr hWnd);

    public delegate bool EnumChildProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }

    public static IntPtr FindChildByClass(IntPtr parent, string className) {
        IntPtr found = IntPtr.Zero;
        EnumChildWindows(parent, (hWnd, lParam) => {
            StringBuilder sb = new StringBuilder(260);
            GetClassName(hWnd, sb, sb.Capacity);
            if (sb.ToString() == className) {
                found = hWnd;
                return false;
            }
            return true;
        }, IntPtr.Zero);
        return found;
    }

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

    public static IntPtr FindProcessWindow(int processId, string className, string title = null) {
        IntPtr found = IntPtr.Zero;
        EnumWindows((hWnd, lParam) => {
            uint pid;
            GetWindowThreadProcessId(hWnd, out pid);
            if (pid == processId) {
                StringBuilder sbClass = new StringBuilder(260);
                GetClassName(hWnd, sbClass, sbClass.Capacity);
                if (className == null || sbClass.ToString() == className) {
                    if (title != null) {
                        StringBuilder sbTitle = new StringBuilder(260);
                        GetWindowTextW(hWnd, sbTitle, sbTitle.Capacity);
                        if (sbTitle.ToString().Contains(title)) {
                            found = hWnd;
                            return false;
                        }
                    } else {
                        found = hWnd;
                        return false;
                    }
                }
            }
            return true;
        }, IntPtr.Zero);
        return found;
    }
}
"@
Add-Type -TypeDefinition $code -ErrorAction SilentlyContinue

$settingsExe = Join-Path $ScriptDir "EliteSettings.exe"

function Stop-EliteProcesses {
    Get-Process -Name EliteTaskbar, EliteSettings, EliteEverything, EliteDLLScanner, Win32Explorer -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 1
}

$results = @{
    "BuildCleanup" = "FAIL"
    "StartMenuTabNoHover" = "FAIL"
    "AboutDialogResizeNoClip" = "FAIL"
    "ApplyDebounceNoMultiSpawn" = "FAIL"
}

# Ensure clean state
Stop-EliteProcesses

try {
    # ----------------- TEST 1: Build Cleanup -----------------
    Write-Host "`n[TEST 1] Verifying build output old files cleanup..." -ForegroundColor Yellow
    
    # Create dummy old files in root, BuildOutput, and BuildOutputx86
    $dummyFiles = @(
        (Join-Path $ScriptDir "dummy_old_test.exe"),
        (Join-Path $ScriptDir "dummy_Old_test.cpl"),
        (Join-Path $ScriptDir "BuildOutput\dummy_old_test.exe"),
        (Join-Path $ScriptDir "BuildOutputx86\dummy_old_test.cpl")
    )
    
    foreach ($file in $dummyFiles) {
        "dummy" | Out-File -FilePath $file -Force
        Write-Host "Created dummy file: $file" -ForegroundColor Gray
    }
    
    # Run the build
    Write-Host "Running build.ps1 to trigger cleanup..." -ForegroundColor Cyan
    $env:ELITE_AUDITOR_RUN = "1"
    
    # Run the build.ps1 using standard terminal call
    $buildProc = Start-Process -FilePath "powershell.exe" -ArgumentList "-File", (Join-Path $ScriptDir "build.ps1") -NoNewWindow -Wait -PassThru
    
    if ($buildProc.ExitCode -ne 0) {
        throw "build.ps1 execution failed with exit code $($buildProc.ExitCode)"
    }
    
    # Check if dummy files are deleted
    $allClean = $true
    foreach ($file in $dummyFiles) {
        if (Test-Path $file) {
            Write-Host "[FAIL] Dummy old file was not deleted: $file" -ForegroundColor Red
            $allClean = $false
        } else {
            Write-Host "[PASS] Dummy old file successfully cleaned: $file" -ForegroundColor Green
        }
    }
    
    if ($allClean) {
        $results["BuildCleanup"] = "PASS"
        Write-Host "[PASS] build.ps1 cleanly removes all old files matching *old*.exe and *old*.cpl." -ForegroundColor Green
    }

    # ----------------- TEST 2: Start Menu Tab (No Hover) -----------------
    Write-Host "`n[TEST 2] Verifying Start Menu settings tab layout..." -ForegroundColor Yellow
    
    # Launch EliteSettings.exe
    $proc = Start-Process -FilePath $settingsExe -PassThru
    Start-Sleep -Seconds 2
    
    $hwndSettings = [Win32Helper]::FindProcessWindow($proc.Id, "#32770", "EliteTaskbar Properties")
    if ($hwndSettings -eq [IntPtr]::Zero) {
        throw "Could not find EliteSettings main window."
    }
    Write-Host "Found Settings sheet HWND: $hwndSettings" -ForegroundColor DarkCyan
    
    # Select tab index 1 (Start Menu) via PSM_SETCURSEL (0x0468)
    [Win32Helper]::SendMessage($hwndSettings, 0x0468, [IntPtr]1, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Milliseconds 500
    
    # Check properties of controls in the Start Menu page dialog
    # Search for controls: IDC_FALLBACK_STARTMENU_ENABLED (293), IDC_MIGRATE_START_MENU_SETTINGS (295) and EliteDynScrollArea
    $hwndFallback = [Win32Helper]::FindChildById($hwndSettings, 293)
    $hwndMigrate = [Win32Helper]::FindChildById($hwndSettings, 295)
    $hwndScroll = [Win32Helper]::FindChildByClass($hwndSettings, "EliteDynScrollArea")
    
    if ($hwndFallback -eq [IntPtr]::Zero -or $hwndMigrate -eq [IntPtr]::Zero -or $hwndScroll -eq [IntPtr]::Zero) {
        throw "Could not find necessary controls (Fallback check: $hwndFallback, Migrate check: $hwndMigrate, Scroll: $hwndScroll)"
    }
    
    # Check visibility
    $visFallback = [Win32Helper]::IsWindowVisible($hwndFallback)
    $visMigrate = [Win32Helper]::IsWindowVisible($hwndMigrate)
    $visScroll = [Win32Helper]::IsWindowVisible($hwndScroll)
    Write-Host "Controls visibility: Fallback=$visFallback, Migrate=$visMigrate, Scroll=$visScroll" -ForegroundColor Cyan
    
    # Check rectangles to verify no overlapping
    $rectScroll = New-Object Win32Helper+RECT
    $rectFallback = New-Object Win32Helper+RECT
    $rectMigrate = New-Object Win32Helper+RECT
    
    [Win32Helper]::GetWindowRect($hwndScroll, [ref]$rectScroll) | Out-Null
    [Win32Helper]::GetWindowRect($hwndFallback, [ref]$rectFallback) | Out-Null
    [Win32Helper]::GetWindowRect($hwndMigrate, [ref]$rectMigrate) | Out-Null
    
    Write-Host "Scroll Area rect: L=$($rectScroll.Left) T=$($rectScroll.Top) R=$($rectScroll.Right) B=$($rectScroll.Bottom)" -ForegroundColor DarkCyan
    Write-Host "Fallback Check rect: L=$($rectFallback.Left) T=$($rectFallback.Top) R=$($rectFallback.Right) B=$($rectFallback.Bottom)" -ForegroundColor DarkCyan
    Write-Host "Migrate Check rect: L=$($rectMigrate.Left) T=$($rectMigrate.Top) R=$($rectMigrate.Right) B=$($rectMigrate.Bottom)" -ForegroundColor DarkCyan
    
    # The scroll area bottom MUST be at or above the checkboxes top to prevent painting overlap and hover bug
    if ($rectScroll.Bottom -le $rectFallback.Top -and $rectScroll.Bottom -le $rectMigrate.Top) {
        if ($visFallback -and $visMigrate -and $visScroll) {
            $results["StartMenuTabNoHover"] = "PASS"
            Write-Host "[PASS] Scroll container bounds do not overlap checkbox controls; controls are fully visible without hover." -ForegroundColor Green
        } else {
            Write-Host "[FAIL] Checkboxes are not visible." -ForegroundColor Red
        }
    } else {
        Write-Host "[FAIL] Scroll area overlaps checkboxes. Bottom of scroll area ($($rectScroll.Bottom)) is below top of Fallback ($($rectFallback.Top)) or Migrate ($($rectMigrate.Top))." -ForegroundColor Red
    }

    # ----------------- TEST 3: About Dialog Resizing and Clipping -----------------
    Write-Host "`n[TEST 3] Verifying About dialog More Info/Less Info resizing and clipping..." -ForegroundColor Yellow
    
    # Open About dialog by sending WM_COMMAND with wParam=40002 (About menu item ID) to settings sheet
    Write-Host "Triggering About dialog..." -ForegroundColor Cyan
    [Win32Helper]::SendMessage($hwndSettings, 0x0111, [IntPtr]40002, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Seconds 1
    
    # Find the About dialog window
    $hwndAbout = [Win32Helper]::FindProcessWindow($proc.Id, "#32770", "About EliteTaskbar")
    if ($hwndAbout -eq [IntPtr]::Zero) {
        throw "Could not find About dialog window."
    }
    Write-Host "Found About dialog HWND: $hwndAbout" -ForegroundColor DarkCyan
    
    # Check rect before expansion
    $rectAboutBefore = New-Object Win32Helper+RECT
    [Win32Helper]::GetWindowRect($hwndAbout, [ref]$rectAboutBefore) | Out-Null
    $heightBefore = $rectAboutBefore.Bottom - $rectAboutBefore.Top
    Write-Host "About dialog height before expansion: $heightBefore" -ForegroundColor Cyan
    
    # Send WM_COMMAND with control ID 282 (IDC_ABOUT_EXPAND) to toggle expand
    Write-Host "Expanding About dialog..." -ForegroundColor Cyan
    [Win32Helper]::SendMessage($hwndAbout, 0x0111, [IntPtr]282, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Milliseconds 500
    
    # Check rect after expansion
    $rectAboutAfter = New-Object Win32Helper+RECT
    [Win32Helper]::GetWindowRect($hwndAbout, [ref]$rectAboutAfter) | Out-Null
    $heightAfter = $rectAboutAfter.Bottom - $rectAboutAfter.Top
    Write-Host "About dialog height after expansion: $heightAfter" -ForegroundColor Cyan
    
    # Verify expand height is significantly larger (should grow by roughly 82 DUs mapped to pixels, usually > 100 pixels)
    $heightDiff = $heightAfter - $heightBefore
    Write-Host "Height difference: $heightDiff" -ForegroundColor Cyan
    
    # Get positions of "Less Info" button and OK button in expanded state
    $hwndExpand = [Win32Helper]::FindChildById($hwndAbout, 282)
    $hwndOk = [Win32Helper]::FindChildById($hwndAbout, 1) # IDOK
    $hwndMoreInfo = [Win32Helper]::FindChildById($hwndAbout, 283)
    
    $rectExpand = New-Object Win32Helper+RECT
    $rectOk = New-Object Win32Helper+RECT
    $rectMoreInfo = New-Object Win32Helper+RECT
    $rectAboutClient = New-Object Win32Helper+RECT
    
    [Win32Helper]::GetWindowRect($hwndExpand, [ref]$rectExpand) | Out-Null
    [Win32Helper]::GetWindowRect($hwndOk, [ref]$rectOk) | Out-Null
    [Win32Helper]::GetWindowRect($hwndMoreInfo, [ref]$rectMoreInfo) | Out-Null
    [Win32Helper]::GetClientRect($hwndAbout, [ref]$rectAboutClient) | Out-Null
    
    # Verify controls do not overlap or clip outside the window client area
    $clientRectScreen = $rectAboutAfter
    # Check if expand button is inside about client boundaries
    $isExpandInside = ($rectExpand.Left -ge $rectAboutAfter.Left -and $rectExpand.Right -le $rectAboutAfter.Right -and $rectExpand.Bottom -le $rectAboutAfter.Bottom)
    $isOkInside = ($rectOk.Left -ge $rectAboutAfter.Left -and $rectOk.Right -le $rectAboutAfter.Right -and $rectOk.Bottom -le $rectAboutAfter.Bottom)
    
    # Verify no overlap between Ok button and Expand button
    $noButtonsOverlap = ($rectExpand.Right -le $rectOk.Left -or $rectOk.Right -le $rectExpand.Left)
    
    Write-Host "Expand inside boundary: $isExpandInside, Ok inside boundary: $isOkInside, Buttons don't overlap: $noButtonsOverlap" -ForegroundColor Cyan
    
    # Collapse back
    Write-Host "Collapsing About dialog..." -ForegroundColor Cyan
    [Win32Helper]::SendMessage($hwndAbout, 0x0111, [IntPtr]282, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Milliseconds 500
    
    $rectAboutCollapsed = New-Object Win32Helper+RECT
    [Win32Helper]::GetWindowRect($hwndAbout, [ref]$rectAboutCollapsed) | Out-Null
    $heightCollapsed = $rectAboutCollapsed.Bottom - $rectAboutCollapsed.Top
    Write-Host "About dialog height after collapsing: $heightCollapsed" -ForegroundColor Cyan
    
    # Dismiss dialog
    [Win32Helper]::SendMessage($hwndAbout, 0x0111, [IntPtr]1, [IntPtr]::Zero) | Out-Null # Send OK
    Start-Sleep -Milliseconds 500
    
    if ($heightDiff -gt 80 -and $isExpandInside -and $isOkInside -and $noButtonsOverlap -and $heightCollapsed -eq $heightBefore) {
        $results["AboutDialogResizeNoClip"] = "PASS"
        Write-Host "[PASS] About dialog resizes cleanly, buttons reposition properly and do not clip or overlap." -ForegroundColor Green
    } else {
        Write-Host "[FAIL] Height change invalid, or buttons overlap/clip." -ForegroundColor Red
    }

    # ----------------- TEST 4: Apply Button Debounce (No Multi-Spawn) -----------------
    Write-Host "`n[TEST 4] Verifying Apply button debounce and Win32Explorer process count..." -ForegroundColor Yellow
    
    # Kill any running Win32Explorer instances first to get a clean baseline
    Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue | Stop-Process -Force
    Start-Sleep -Seconds 1
    
    # Rapidly click Apply button (ID 0x3021 / 12321) 3 times
    Write-Host "Sending rapid Apply commands..." -ForegroundColor Cyan
    [Win32Helper]::SendMessage($hwndSettings, 0x0111, [IntPtr]0x3021, [IntPtr]::Zero) | Out-Null
    [Win32Helper]::SendMessage($hwndSettings, 0x0111, [IntPtr]0x3021, [IntPtr]::Zero) | Out-Null
    [Win32Helper]::SendMessage($hwndSettings, 0x0111, [IntPtr]0x3021, [IntPtr]::Zero) | Out-Null
    
    Write-Host "Waiting 6 seconds for shell relaunch script to complete..." -ForegroundColor Cyan
    Start-Sleep -Seconds 6
    
    # Check number of Win32Explorer processes running
    $explProcesses = Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue
    $count = ($explProcesses | Measure-Object).Count
    Write-Host "Number of Win32Explorer processes running: $count" -ForegroundColor Cyan
    
    if ($count -eq 1) {
        $results["ApplyDebounceNoMultiSpawn"] = "PASS"
        Write-Host "[PASS] Click Apply multiple times does not spawn multiple Win32Explorer instances (Exactly 1 is running)." -ForegroundColor Green
    } else {
        Write-Host "[FAIL] Found $count running Win32Explorer instances. Debounce failed or processes didn't start." -ForegroundColor Red
        if ($count -gt 0) {
            Write-Host "Active PIDs: $(($explProcesses | Select-Object -ExpandProperty Id) -join ', ')" -ForegroundColor Gray
        }
    }

} catch {
    Write-Host "[FAIL] E2E verification failed with error: $_" -ForegroundColor Red
} finally {
    # Close EliteSettings dialog
    if ($hwndSettings -and $hwndSettings -ne [IntPtr]::Zero) {
        [Win32Helper]::SendMessage($hwndSettings, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null # WM_CLOSE
    }
    Stop-EliteProcesses
}

# ----------------- FINAL REPORT -----------------
Write-Host "`n=== Comprehensive E2E Verification Summary ===" -ForegroundColor Cyan
$overallPass = $true
foreach ($key in $results.Keys) {
    $res = $results[$key]
    if ($res -eq "PASS") {
        Write-Host "  $key : [PASS]" -ForegroundColor Green
    } else {
        Write-Host "  $key : [FAIL]" -ForegroundColor Red
        $overallPass = $false
    }
}

if ($overallPass) {
    Write-Host "`nFINAL VERDICT: PASS" -ForegroundColor Green
    "PASS" | Out-File -FilePath (Join-Path $MyDir "verdict.txt") -Force
} else {
    Write-Host "`nFINAL VERDICT: FAIL" -ForegroundColor Red
    "FAIL" | Out-File -FilePath (Join-Path $MyDir "verdict.txt") -Force
}

$results | Out-String | Out-File -FilePath (Join-Path $MyDir "test_results.txt") -Force
