# run_empirical_tests.ps1
# Automated Empirical Testing Script for Challenger 4 (Milestone 3)
$ErrorActionPreference = 'Stop'
$ScriptDir = "C:\Users\Administrator\Desktop\Elite-TaskBar"
$MyDir = "C:\Users\Administrator\Desktop\Elite-TaskBar\.agents\challenger_m3_2"

Write-Host "=== Win32Explorer and EliteTaskbar View Modes & Integration Empirical Tests ===" -ForegroundColor Cyan

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

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetWindowTextW(IntPtr hWnd, StringBuilder lpString, int nMaxCount);

    [DllImport("user32.dll")]
    public static extern int GetDlgCtrlID(IntPtr hwnd);

    [DllImport("comctl32.dll", SetLastError = true)]
    public static extern bool ImageList_GetIconSize(IntPtr himl, out int cx, out int cy);

    public delegate bool EnumChildProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

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
}
"@
Add-Type -TypeDefinition $code -ErrorAction SilentlyContinue

$explorerExe = Join-Path $ScriptDir "Win32Explorer.exe"
$taskbarExe = Join-Path $ScriptDir "EliteTaskbar.exe"

# Utility to clean up running processes
function Stop-EliteProcesses {
    Get-Process -Name Win32Explorer, EliteTaskbar -ErrorAction SilentlyContinue | Stop-Process -Force
    Start-Sleep -Seconds 1
}

# Registry paths
$regSettingsPath = "HKCU:\Software\Win32Explorer\Settings"
$regSettingsBackup = "HKCU:\Software\Win32Explorer\Settings_Backup"

# Backup helper
function Backup-RegistrySettings {
    if (Test-Path $regSettingsPath) {
        if (Test-Path $regSettingsBackup) { Remove-Item $regSettingsBackup -Recurse -Force | Out-Null }
        Copy-Item -Path $regSettingsPath -Destination $regSettingsBackup -Recurse -Force | Out-Null
    }
}

function Restore-RegistrySettings {
    if (Test-Path $regSettingsBackup) {
        if (Test-Path $regSettingsPath) { Remove-Item $regSettingsPath -Recurse -Force | Out-Null }
        Copy-Item -Path $regSettingsBackup -Destination $regSettingsPath -Recurse -Force | Out-Null
        Remove-Item $regSettingsBackup -Recurse -Force | Out-Null
    } else {
        if (Test-Path $regSettingsPath) { Remove-Item $regSettingsPath -Recurse -Force | Out-Null }
    }
}

$results = @{
    "SmallIconTilesView" = "FAIL"
    "DefaultGroupByType" = "FAIL"
    "OptionsToggleAndSave" = "FAIL"
    "EliteTaskbarIsolation" = "FAIL"
}

# ----------------- TEST 1: Small Icon Tiles View Mode -----------------
Write-Host "`n[TEST 1] Verifying 'Small Icon Tiles' View Mode..." -ForegroundColor Yellow
Stop-EliteProcesses
Backup-RegistrySettings

# Setup registry for Small Icon Tiles view (ViewModeGlobal = 12)
if (!(Test-Path $regSettingsPath)) { New-Item -Path $regSettingsPath -Force | Out-Null }
Set-ItemProperty -Path $regSettingsPath -Name "ViewModeGlobal" -Value 12 -Type DWord
Set-ItemProperty -Path $regSettingsPath -Name "ShowInGroupsGlobal" -Value 0 -Type DWord
Set-ItemProperty -Path $regSettingsPath -Name "EnableEliteTaskbar" -Value 0 -Type DWord

$proc1 = Start-Process -FilePath $explorerExe -PassThru
Start-Sleep -Seconds 5

try {
    $hwndMain1 = [Win32Helper]::FindProcessWindow($proc1.Id, "Win32Explorer")
    if ($hwndMain1 -eq [IntPtr]::Zero) {
        throw "Could not find Win32Explorer main window."
    }

    $hwndLV1 = [Win32Helper]::FindChildByClass($hwndMain1, "SysListView32")
    if ($hwndLV1 -eq [IntPtr]::Zero) {
        throw "Could not find child SysListView32 control."
    }

    # LVM_GETVIEW (0x108F)
    $view = [Win32Helper]::SendMessage($hwndLV1, 0x108F, [IntPtr]::Zero, [IntPtr]::Zero)
    Write-Host "ListView View Mode (LVM_GETVIEW): $view (Expected: 4 = LV_VIEW_TILE)" -ForegroundColor Cyan
    
    # LVM_GETIMAGELIST (0x1002), LVSIL_NORMAL = 0
    $himl = [Win32Helper]::SendMessage($hwndLV1, 0x1002, [IntPtr]::Zero, [IntPtr]::Zero)
    Write-Host "ListView Normal ImageList handle: $himl" -ForegroundColor Cyan

    if ($himl -ne [IntPtr]::Zero) {
        $cx = 0
        $cy = 0
        $imgRes = [Win32Helper]::ImageList_GetIconSize($himl, [ref]$cx, [ref]$cy)
        Write-Host "ImageList_GetIconSize result: $imgRes, Width: $cx, Height: $cy (Expected: 16x16)" -ForegroundColor Cyan
        
        if ($view -eq 4 -and $cx -eq 16 -and $cy -eq 16) {
            $results["SmallIconTilesView"] = "PASS"
            Write-Host "[PASS] 'Small Icon Tiles' view mode renders items with 16x16 icon in Tile view." -ForegroundColor Green
        } else {
            Write-Host "[FAIL] View mode or icon size incorrect." -ForegroundColor Red
        }
    } else {
        Write-Host "[FAIL] Normal ImageList handle is NULL." -ForegroundColor Red
    }
} catch {
    Write-Host "[FAIL] Error in Test 1: $_" -ForegroundColor Red
} finally {
    # Close window
    if ($hwndMain1 -ne [IntPtr]::Zero) {
        [Win32Helper]::SendMessage($hwndMain1, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
    }
    Stop-EliteProcesses
}

# ----------------- TEST 2: Folders Default to 'Group by Type' on First Launch -----------------
Write-Host "`n[TEST 2] Verifying folders default to 'Group by Type' on first launch..." -ForegroundColor Yellow
Stop-EliteProcesses

# Simulate first launch: delete registry and config.xml
if (Test-Path $regSettingsPath) { Remove-Item $regSettingsPath -Recurse -Force | Out-Null }
$xmlPath = Join-Path $ScriptDir "config.xml"
if (Test-Path $xmlPath) { Remove-Item $xmlPath -Force | Out-Null }

$proc2 = Start-Process -FilePath $explorerExe -PassThru
Start-Sleep -Seconds 5

try {
    $hwndMain2 = [Win32Helper]::FindProcessWindow($proc2.Id, "Win32Explorer")
    if ($hwndMain2 -eq [IntPtr]::Zero) {
        throw "Could not find Win32Explorer main window."
    }

    $hwndLV2 = [Win32Helper]::FindChildByClass($hwndMain2, "SysListView32")
    if ($hwndLV2 -eq [IntPtr]::Zero) {
        throw "Could not find child SysListView32 control."
    }

    # LVM_ISGROUPVIEWENABLED (0x10AF)
    $groupEnabled = [Win32Helper]::SendMessage($hwndLV2, 0x10AF, [IntPtr]::Zero, [IntPtr]::Zero)
    Write-Host "Group view enabled state (LVM_ISGROUPVIEWENABLED): $groupEnabled (Expected: 1)" -ForegroundColor Cyan

    # Read registry to verify EnableDefaultGroupByType defaults to 1
    $val = (Get-ItemProperty -Path $regSettingsPath -Name "EnableDefaultGroupByType" -ErrorAction SilentlyContinue).EnableDefaultGroupByType
    Write-Host "EnableDefaultGroupByType registry value: $val (Expected: 1)" -ForegroundColor Cyan

    if ($groupEnabled -ne 0 -and $val -eq 1) {
        $results["DefaultGroupByType"] = "PASS"
        Write-Host "[PASS] Folders default to Group View, and EnableDefaultGroupByType defaults to 1." -ForegroundColor Green
    } else {
        Write-Host "[FAIL] Grouping not enabled by default or registry missing." -ForegroundColor Red
    }
} catch {
    Write-Host "[FAIL] Error in Test 2: $_" -ForegroundColor Red
} finally {
    if ($hwndMain2 -ne [IntPtr]::Zero) {
        [Win32Helper]::SendMessage($hwndMain2, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
    }
    Stop-EliteProcesses
}

# ----------------- TEST 3: Options Window and Registry/XML Sync -----------------
Write-Host "`n[TEST 3] Verifying Options dialog toggle and Registry/XML synchronization..." -ForegroundColor Yellow
Stop-EliteProcesses
Restore-RegistrySettings

# Setup registry
if (!(Test-Path $regSettingsPath)) { New-Item -Path $regSettingsPath -Force | Out-Null }
Set-ItemProperty -Path $regSettingsPath -Name "EnableDefaultGroupByType" -Value 1 -Type DWord
Set-ItemProperty -Path $regSettingsPath -Name "EnablePortableMirror" -Value 1 -Type DWord
if (Test-Path $xmlPath) { Remove-Item $xmlPath -Force | Out-Null }

$proc3 = Start-Process -FilePath $explorerExe -PassThru
Start-Sleep -Seconds 5

try {
    $hwndMain3 = [Win32Helper]::FindProcessWindow($proc3.Id, "Win32Explorer")
    if ($hwndMain3 -eq [IntPtr]::Zero) {
        throw "Could not find main window."
    }

    # Open Options dialog (WM_COMMAND, 40101 = IDM_TOOLS_OPTIONS)
    [Win32Helper]::SendMessage($hwndMain3, 0x0111, [IntPtr]40101, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Seconds 2

    # Find the dialog (#32770) owned by the process with title "Options"
    $hwndOpt = [Win32Helper]::FindProcessWindow($proc3.Id, "#32770", "Options")
    if ($hwndOpt -eq [IntPtr]::Zero) {
        throw "Could not find Options dialog."
    }

    # Find the checkbox IDC_OPTION_DEFAULTGROUPBYTYPE = 1382
    $hwndChk = [Win32Helper]::FindChildById($hwndOpt, 1382)
    if ($hwndChk -eq [IntPtr]::Zero) {
        throw "Could not find checkbox default group by type (ID 1382) in dialog."
    }
    Write-Host "[PASS] Options window contains the checkbox default toggle (ID 1382)." -ForegroundColor Green

    # Verify default state of checkbox: BM_GETCHECK = 0x00F0
    $chkState = [Win32Helper]::SendMessage($hwndChk, 0x00F0, [IntPtr]::Zero, [IntPtr]::Zero)
    Write-Host "Checkbox initial check state: $chkState (Expected: 1 = Checked)" -ForegroundColor Cyan

    # Toggle checkbox to Unchecked
    # BM_SETCHECK = 0x00F1, BST_UNCHECKED = 0
    [Win32Helper]::SendMessage($hwndChk, 0x00F1, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
    # Send BN_CLICKED command notification to options page parent dialog
    # notification code 0 (BN_CLICKED) << 16 | 1382 = 1382
    [Win32Helper]::SendMessage($hwndOpt, 0x0111, [IntPtr]1382, $hwndChk) | Out-Null
    Start-Sleep -Milliseconds 200

    # Find OK button (ID = 1 = IDOK)
    $hwndOK = [Win32Helper]::FindChildById($hwndOpt, 1)
    if ($hwndOK -eq [IntPtr]::Zero) {
        throw "Could not find OK button in dialog."
    }

    # Send BM_CLICK = 0x00F5 to OK button
    [Win32Helper]::SendMessage($hwndOK, 0x00F5, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Seconds 2

    # Verify dialog closed
    $hwndOptClosed = [Win32Helper]::FindProcessWindow($proc3.Id, "#32770", "Options")
    Write-Host "Options Dialog HWND after OK: $hwndOptClosed (Expected: NULL/0)" -ForegroundColor Cyan

    # Verify registry updated to 0
    $regVal = (Get-ItemProperty -Path $regSettingsPath -Name "EnableDefaultGroupByType").EnableDefaultGroupByType
    Write-Host "Registry EnableDefaultGroupByType after click OK: $regVal (Expected: 0)" -ForegroundColor Cyan

    # Close explorer window to trigger XML flush
    [Win32Helper]::SendMessage($hwndMain3, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Seconds 3

    # Verify XML was written and contains false
    if (Test-Path $xmlPath) {
        $xmlContent = Get-Content $xmlPath -Raw
        Write-Host "config.xml exists." -ForegroundColor Cyan
        
        if ($xmlContent -match "<EnableDefaultGroupByType>false</EnableDefaultGroupByType>" -and $regVal -eq 0) {
            $results["OptionsToggleAndSave"] = "PASS"
            Write-Host "[PASS] Default toggle checkbox saves changes to both registry and XML configuration." -ForegroundColor Green
        } else {
            Write-Host "[FAIL] XML content or registry value does not reflect unchecked state." -ForegroundColor Red
        }
    } else {
        Write-Host "[FAIL] config.xml not found." -ForegroundColor Red
    }
} catch {
    Write-Host "[FAIL] Error in Test 3: $_" -ForegroundColor Red
} finally {
    Stop-EliteProcesses
}

# ----------------- TEST 4: EliteTaskbar Lifetime Isolation -----------------
Write-Host "`n[TEST 4] Verifying EliteTaskbar process isolation with active integration..." -ForegroundColor Yellow
Stop-EliteProcesses
Restore-RegistrySettings

# Enable integration in registry
if (!(Test-Path $regSettingsPath)) { New-Item -Path $regSettingsPath -Force | Out-Null }
Set-ItemProperty -Path $regSettingsPath -Name "EnableEliteTaskbar" -Value 1 -Type DWord

Write-Host "Launching Win32Explorer.exe..." -ForegroundColor Cyan
$proc4 = Start-Process -FilePath $explorerExe -PassThru
Start-Sleep -Seconds 6

try {
    # Check that both Win32Explorer and EliteTaskbar are running
    $explProc = Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue
    $taskbarProc = Get-Process -Name EliteTaskbar -ErrorAction SilentlyContinue

    if ($explProc -and $taskbarProc) {
        Write-Host "[PASS] Both Win32Explorer ($($explProc.Id)) and EliteTaskbar ($($taskbarProc.Id)) are running successfully." -ForegroundColor Green

        $hwndMain4 = [Win32Helper]::FindProcessWindow($proc4.Id, "Win32Explorer")
        if ($hwndMain4 -eq [IntPtr]::Zero) {
            throw "Could not find Win32Explorer window."
        }

        # Send WM_CLOSE (0x0010) to Win32Explorer browser window
        Write-Host "Closing browser window..." -ForegroundColor Cyan
        [Win32Helper]::SendMessage($hwndMain4, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
        Start-Sleep -Seconds 3

        # Verify browser process exited but EliteTaskbar is still running!
        $explProcAfter = Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue
        $taskbarProcAfter = Get-Process -Name EliteTaskbar -ErrorAction SilentlyContinue

        if ($null -eq $explProcAfter -and $taskbarProcAfter) {
            $results["EliteTaskbarIsolation"] = "PASS"
            Write-Host "[PASS] Closing browser window closed Win32Explorer process but preserved EliteTaskbar ($($taskbarProcAfter.Id))." -ForegroundColor Green
        } else {
            Write-Host "[FAIL] Win32Explorer still running: $(!!$explProcAfter), EliteTaskbar running: $(!!$taskbarProcAfter)" -ForegroundColor Red
        }
    } else {
        Write-Host "[FAIL] Processes did not launch properly. Explorer running: $(!!$explProc), Taskbar running: $(!!$taskbarProc)" -ForegroundColor Red
    }
} catch {
    Write-Host "[FAIL] Error in Test 4: $_" -ForegroundColor Red
} finally {
    Stop-EliteProcesses
}

# ----------------- FINAL REPORT -----------------
Write-Host "`n=== Empirical Test Summary ===" -ForegroundColor Cyan
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
    Write-Host "`nVERDICT: PASS" -ForegroundColor Green
    "PASS" | Out-File -FilePath (Join-Path $MyDir "verdict.txt") -Force
} else {
    Write-Host "`nVERDICT: FAIL" -ForegroundColor Red
    "FAIL" | Out-File -FilePath (Join-Path $MyDir "verdict.txt") -Force
}

$results | Out-String | Out-File -FilePath (Join-Path $MyDir "test_results.txt") -Force
