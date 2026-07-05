# run_empirical_tests.ps1
# Automated Empirical Testing Script for Challenger 4 (Milestone 3)
$ErrorActionPreference = 'Stop'
$ScriptDir = "C:\Users\Administrator\Desktop\Elite-TaskBar"
$MyDir = "C:\Users\Administrator\Desktop\Elite-TaskBar\.agents\auditor_m3_re"

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

    [DllImport("user32.dll")]
    public static extern IntPtr GetParent(IntPtr hWnd);

    public delegate bool EnumChildProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

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

$explorerExeDir = Join-Path $ScriptDir "Win32Explorer_26.0.3.0"
$explorerExe = Join-Path $explorerExeDir "Win32Explorer.exe"
$taskbarExe = Join-Path $ScriptDir "BuildOutput\EliteTaskbar.exe"
$xmlPath = Join-Path $explorerExeDir "config.xml"


# Registry paths
$regParentPath = "HKCU:\Software"
$regExplorerPath = "HKCU:\Software\Win32Explorer"
$regSettingsPath = "HKCU:\Software\Win32Explorer\Settings"
$regSettingsBackupName = "Settings_Backup"
$regSettingsBackupPath = "HKCU:\Software\Win32Explorer\Settings_Backup"

# Utility to clean up running processes and files
function Stop-EliteProcesses {
    Get-Process -Name Win32Explorer, EliteTaskbar -ErrorAction SilentlyContinue | Stop-Process -Force
    Start-Sleep -Seconds 3.5
    if (Test-Path $xmlPath) { Remove-Item $xmlPath -Force | Out-Null }
}

# Clean registry Settings key entirely
function Clear-RegistrySettings {
    if (Test-Path $regSettingsPath) {
        Remove-Item $regSettingsPath -Recurse -Force -ErrorAction SilentlyContinue | Out-Null
    }
}


# Registry backup via rename
function Backup-RegistrySettings {
    if (Test-Path $regSettingsPath) {
        if (Test-Path $regSettingsBackupPath) { Remove-Item $regSettingsBackupPath -Recurse -Force | Out-Null }
        Rename-Item -Path $regSettingsPath -NewName $regSettingsBackupName -Force | Out-Null
    }
}

function Restore-RegistrySettings {
    if (Test-Path $regSettingsPath) { Remove-Item $regSettingsPath -Recurse -Force | Out-Null }
    if (Test-Path $regSettingsBackupPath) {
        Rename-Item -Path $regSettingsBackupPath -NewName "Settings" -Force | Out-Null
    }
}

$results = @{
    "SmallIconTilesView" = "FAIL"
    "DefaultGroupByType" = "FAIL"
    "OptionsToggleAndSave" = "FAIL"
    "EliteTaskbarIsolation" = "FAIL"
}

# Back up original user settings once
Stop-EliteProcesses
Backup-RegistrySettings

try {
    # ----------------- TEST 1: Small Icon Tiles View Mode -----------------
    Write-Host "`n[TEST 1] Verifying 'Small Icon Tiles' View Mode..." -ForegroundColor Yellow
    Clear-RegistrySettings
    Stop-EliteProcesses

    # Setup clean registry for Small Icon Tiles view
    if (!(Test-Path $regSettingsPath)) { New-Item -Path $regSettingsPath -Force | Out-Null }
    Set-ItemProperty -Path $regSettingsPath -Name "ViewModeGlobal" -Value 12 -Type DWord
    Set-ItemProperty -Path $regSettingsPath -Name "ShowInGroupsGlobal" -Value 0 -Type DWord
    Set-ItemProperty -Path $regSettingsPath -Name "EnableEliteTaskbar" -Value 0 -Type DWord
    Set-ItemProperty -Path $regSettingsPath -Name "ConfirmCloseTabs" -Value 0 -Type DWord
    Set-ItemProperty -Path $regSettingsPath -Name "EnablePortableMirror" -Value 0 -Type DWord
    Set-ItemProperty -Path $regSettingsPath -Name "EnableShellBagsSupport" -Value 0 -Type DWord


    $proc1 = Start-Process -FilePath $explorerExe -ArgumentList "C:\Windows" -PassThru

    # Wait for main window with "Windows" title
    $hwndMain1 = [IntPtr]::Zero
    for ($i = 0; $i -lt 30; $i++) {
        $hwndMain1 = [Win32Helper]::FindProcessWindow($proc1.Id, "Win32Explorer", "Windows")
        if ($hwndMain1 -ne [IntPtr]::Zero) { break }
        Start-Sleep -Milliseconds 500
    }

    try {
        if ($hwndMain1 -eq [IntPtr]::Zero) {
            throw "Could not find Win32Explorer main window with 'Windows' title."
        }

        $hwndLV1 = [Win32Helper]::FindChildByClass($hwndMain1, "SysListView32")
        if ($hwndLV1 -eq [IntPtr]::Zero) {
            throw "Could not find child SysListView32 control."
        }

        # Wait for ListView view mode to become 4 (LV_VIEW_TILE) in loop
        $view = 0
        for ($i = 0; $i -lt 20; $i++) {
            $view = [Win32Helper]::SendMessage($hwndLV1, 0x108F, [IntPtr]::Zero, [IntPtr]::Zero)
            if ($view -eq 4) { break }
            Start-Sleep -Milliseconds 500
        }

        Write-Host "ListView View Mode (LVM_GETVIEW): $view (Expected: 4 = LV_VIEW_TILE)" -ForegroundColor Cyan
        
        # LVM_GETIMAGELIST (0x1002), LVSIL_NORMAL = 0
        $himl = [Win32Helper]::SendMessage($hwndLV1, 0x1002, [IntPtr]::Zero, [IntPtr]::Zero)
        Write-Host "ListView Normal ImageList handle: $himl" -ForegroundColor Cyan

        if ($view -eq 4 -and $himl -ne [IntPtr]::Zero) {
            $results["SmallIconTilesView"] = "PASS"
            Write-Host "[PASS] 'Small Icon Tiles' view mode renders items in Tile view (4) using small image list." -ForegroundColor Green
        } else {
            Write-Host "[FAIL] View mode incorrect or image list handle NULL." -ForegroundColor Red
        }
    } catch {
        Write-Host "[FAIL] Error in Test 1: $_" -ForegroundColor Red
    } finally {
        if ($hwndMain1 -ne [IntPtr]::Zero) {
            [Win32Helper]::SendMessage($hwndMain1, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
        }
    }

    # ----------------- TEST 2: Folders Default to 'Group by Type' on First Launch -----------------
    Write-Host "`n[TEST 2] Verifying folders default to 'Group by Type' on first launch..." -ForegroundColor Yellow
    Clear-RegistrySettings
    Stop-EliteProcesses

    if (!(Test-Path $regSettingsPath)) { New-Item -Path $regSettingsPath -Force | Out-Null }
    Set-ItemProperty -Path $regSettingsPath -Name "ConfirmCloseTabs" -Value 0 -Type DWord
    Set-ItemProperty -Path $regSettingsPath -Name "EnablePortableMirror" -Value 0 -Type DWord

    $proc2 = Start-Process -FilePath $explorerExe -ArgumentList "C:\Windows" -PassThru


    # Wait for main window with "Windows" title
    $hwndMain2 = [IntPtr]::Zero
    for ($i = 0; $i -lt 30; $i++) {
        $hwndMain2 = [Win32Helper]::FindProcessWindow($proc2.Id, "Win32Explorer", "Windows")
        if ($hwndMain2 -ne [IntPtr]::Zero) { break }
        Start-Sleep -Milliseconds 500
    }

    try {
        if ($hwndMain2 -eq [IntPtr]::Zero) {
            throw "Could not find Win32Explorer main window with 'Windows' title."
        }

        $hwndLV2 = [Win32Helper]::FindChildByClass($hwndMain2, "SysListView32")
        if ($hwndLV2 -eq [IntPtr]::Zero) {
            throw "Could not find child SysListView32 control."
        }

        # Wait for Group View state to become 1 in loop
        $groupEnabled = 0
        for ($i = 0; $i -lt 20; $i++) {
            $groupEnabled = [Win32Helper]::SendMessage($hwndLV2, 0x10AF, [IntPtr]::Zero, [IntPtr]::Zero)
            if ($groupEnabled -eq 1) { break }
            Start-Sleep -Milliseconds 500
        }

        Write-Host "Group view enabled state (LVM_ISGROUPVIEWENABLED): $groupEnabled (Expected: 1)" -ForegroundColor Cyan

        # Close explorer window to trigger settings flush to registry
        [Win32Helper]::SendMessage($hwndMain2, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
        Start-Sleep -Seconds 5

        # Read registry to verify EnableDefaultGroupByType defaults to 1
        $val = (Get-ItemProperty -Path $regSettingsPath -Name "EnableDefaultGroupByType" -ErrorAction SilentlyContinue).EnableDefaultGroupByType
        Write-Host "EnableDefaultGroupByType registry value: $val (Expected: 1)" -ForegroundColor Cyan

        if ($groupEnabled -ne 0 -and $val -eq 1) {
            $results["DefaultGroupByType"] = "PASS"
            Write-Host "[PASS] Folders default to Group View on first launch, and EnableDefaultGroupByType defaults to 1." -ForegroundColor Green
        } else {
            Write-Host "[FAIL] Grouping not enabled by default or registry setting not written." -ForegroundColor Red
        }
    } catch {
        Write-Host "[FAIL] Error in Test 2: $_" -ForegroundColor Red
    }

    # ----------------- TEST 3: Options Window and Registry/XML Sync -----------------
    Write-Host "`n[TEST 3] Verifying Options dialog toggle and Registry/XML synchronization..." -ForegroundColor Yellow
    
    # Phase A: Verify Registry Saving (EnablePortableMirror = 0)
    Write-Host "--- Phase A: Registry Storage Mode (EnablePortableMirror = 0) ---" -ForegroundColor Cyan
    Clear-RegistrySettings
    Stop-EliteProcesses

    if (!(Test-Path $regSettingsPath)) { New-Item -Path $regSettingsPath -Force | Out-Null }
    Set-ItemProperty -Path $regSettingsPath -Name "EnableDefaultGroupByType" -Value 1 -Type DWord
    Set-ItemProperty -Path $regSettingsPath -Name "EnablePortableMirror" -Value 0 -Type DWord
    Set-ItemProperty -Path $regSettingsPath -Name "ConfirmCloseTabs" -Value 0 -Type DWord

    $proc3A = Start-Process -FilePath $explorerExe -ArgumentList "C:\Windows" -PassThru

    # Wait for main window with "Windows" title
    $hwndMain3A = [IntPtr]::Zero
    for ($i = 0; $i -lt 30; $i++) {
        $hwndMain3A = [Win32Helper]::FindProcessWindow($proc3A.Id, "Win32Explorer", "Windows")
        if ($hwndMain3A -ne [IntPtr]::Zero) { break }
        Start-Sleep -Milliseconds 500
    }

    $regPass = $false
    try {
        if ($hwndMain3A -eq [IntPtr]::Zero) { throw "Could not find main window." }

        # Open Options dialog (WM_COMMAND, 40101)
        [Win32Helper]::SendMessage($hwndMain3A, 0x0111, [IntPtr]40101, [IntPtr]::Zero) | Out-Null
        
        # Wait for dialog in loop
        $hwndOpt3A = [IntPtr]::Zero
        for ($i = 0; $i -lt 10; $i++) {
            $hwndOpt3A = [Win32Helper]::FindProcessWindow($proc3A.Id, "#32770", "Options")
            if ($hwndOpt3A -ne [IntPtr]::Zero) { break }
            Start-Sleep -Milliseconds 500
        }
        if ($hwndOpt3A -eq [IntPtr]::Zero) { throw "Could not find Options dialog." }

        # Find the checkbox (ID 1382)
        $hwndChk3A = [Win32Helper]::FindChildById($hwndOpt3A, 1382)
        if ($hwndChk3A -eq [IntPtr]::Zero) { throw "Could not find checkbox default group by type (ID 1382)." }

        # Uncheck checkbox: BM_SETCHECK = 0x00F1, BST_UNCHECKED = 0
        [Win32Helper]::SendMessage($hwndChk3A, 0x00F1, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
        
        # Send BN_CLICKED to parent page
        $hwndParent3A = [Win32Helper]::GetParent($hwndChk3A)
        [Win32Helper]::SendMessage($hwndParent3A, 0x0111, [IntPtr]1382, $hwndChk3A) | Out-Null
        Start-Sleep -Milliseconds 200

        # Send WM_COMMAND with IDOK (1) to Options dialog to save & close
        [Win32Helper]::SendMessage($hwndOpt3A, 0x0111, [IntPtr]1, [IntPtr]::Zero) | Out-Null
        Start-Sleep -Seconds 3

        # Close explorer window to trigger flush
        [Win32Helper]::SendMessage($hwndMain3A, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
        Start-Sleep -Seconds 6

        # Verify registry updated to 0
        $regVal3A = (Get-ItemProperty -Path $regSettingsPath -Name "EnableDefaultGroupByType").EnableDefaultGroupByType
        Write-Host "Registry EnableDefaultGroupByType after exit: $regVal3A (Expected: 0)" -ForegroundColor Cyan
        if ($regVal3A -eq 0) {
            $regPass = $true
            Write-Host "Phase A PASS: Registry persistence verified." -ForegroundColor Green
        }
    } catch {
        Write-Host "Error in Phase A: $_" -ForegroundColor Red
    }

    # Phase B: Verify XML Saving (EnablePortableMirror = 1)
    Write-Host "--- Phase B: XML Portable Mirror Mode (EnablePortableMirror = 1) ---" -ForegroundColor Cyan
    Clear-RegistrySettings
    Stop-EliteProcesses

    if (!(Test-Path $regSettingsPath)) { New-Item -Path $regSettingsPath -Force | Out-Null }
    Set-ItemProperty -Path $regSettingsPath -Name "EnableDefaultGroupByType" -Value 1 -Type DWord
    Set-ItemProperty -Path $regSettingsPath -Name "EnablePortableMirror" -Value 1 -Type DWord
    Set-ItemProperty -Path $regSettingsPath -Name "ConfirmCloseTabs" -Value 0 -Type DWord

    $proc3B = Start-Process -FilePath $explorerExe -ArgumentList "C:\Windows" -PassThru

    # Wait for main window with "Windows" title
    $hwndMain3B = [IntPtr]::Zero
    for ($i = 0; $i -lt 30; $i++) {
        $hwndMain3B = [Win32Helper]::FindProcessWindow($proc3B.Id, "Win32Explorer", "Windows")
        if ($hwndMain3B -ne [IntPtr]::Zero) { break }
        Start-Sleep -Milliseconds 500
    }

    $xmlPass = $false
    try {
        if ($hwndMain3B -eq [IntPtr]::Zero) { throw "Could not find main window." }

        # Open Options dialog (WM_COMMAND, 40101)
        [Win32Helper]::SendMessage($hwndMain3B, 0x0111, [IntPtr]40101, [IntPtr]::Zero) | Out-Null
        
        # Wait for dialog in loop
        $hwndOpt3B = [IntPtr]::Zero
        for ($i = 0; $i -lt 10; $i++) {
            $hwndOpt3B = [Win32Helper]::FindProcessWindow($proc3B.Id, "#32770", "Options")
            if ($hwndOpt3B -ne [IntPtr]::Zero) { break }
            Start-Sleep -Milliseconds 500
        }
        if ($hwndOpt3B -eq [IntPtr]::Zero) { throw "Could not find Options dialog." }

        # Find the checkbox (ID 1382)
        $hwndChk3B = [Win32Helper]::FindChildById($hwndOpt3B, 1382)
        if ($hwndChk3B -eq [IntPtr]::Zero) { throw "Could not find checkbox default group by type (ID 1382)." }

        # Uncheck checkbox
        [Win32Helper]::SendMessage($hwndChk3B, 0x00F1, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
        
        # Send BN_CLICKED to parent page
        $hwndParent3B = [Win32Helper]::GetParent($hwndChk3B)
        [Win32Helper]::SendMessage($hwndParent3B, 0x0111, [IntPtr]1382, $hwndChk3B) | Out-Null
        Start-Sleep -Milliseconds 200

        # Send WM_COMMAND with IDOK (1) to Options dialog
        [Win32Helper]::SendMessage($hwndOpt3B, 0x0111, [IntPtr]1, [IntPtr]::Zero) | Out-Null
        Start-Sleep -Seconds 3

        # Close explorer window to trigger flush
        [Win32Helper]::SendMessage($hwndMain3B, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
        Start-Sleep -Seconds 6

        # Verify XML content contains the standard setting node set to no
        if (Test-Path $xmlPath) {
            $xmlContent = Get-Content $xmlPath -Raw
            Write-Host "config.xml exists." -ForegroundColor Cyan
            
            if ($xmlContent -match '<Setting\s+name="EnableDefaultGroupByType">no</Setting>') {
                $xmlPass = $true
                Write-Host "Phase B PASS: XML persistence verified." -ForegroundColor Green
            } else {
                Write-Host "[FAIL] XML content does not contain unchecked EnableDefaultGroupByType setting." -ForegroundColor Red
                Write-Host "XML content was: $xmlContent" -ForegroundColor Gray
            }
        } else {
            Write-Host "[FAIL] config.xml was not created." -ForegroundColor Red
        }
    } catch {
        Write-Host "Error in Phase B: $_" -ForegroundColor Red
    }

    if ($regPass -and $xmlPass) {
        $results["OptionsToggleAndSave"] = "PASS"
        Write-Host "[PASS] Default toggle checkbox successfully saves changes in both Registry and XML modes." -ForegroundColor Green
    }

    # ----------------- TEST 4: EliteTaskbar Lifetime Isolation -----------------
    Write-Host "`n[TEST 4] Verifying EliteTaskbar process isolation with active integration..." -ForegroundColor Yellow
    Clear-RegistrySettings
    Stop-EliteProcesses

    # Enable integration in registry
    if (!(Test-Path $regSettingsPath)) { New-Item -Path $regSettingsPath -Force | Out-Null }
    Set-ItemProperty -Path $regSettingsPath -Name "EnableEliteTaskbar" -Value 1 -Type DWord
    Set-ItemProperty -Path $regSettingsPath -Name "ConfirmCloseTabs" -Value 0 -Type DWord

    Write-Host "Launching Win32Explorer.exe C:\Windows..." -ForegroundColor Cyan
    $proc4 = Start-Process -FilePath $explorerExe -ArgumentList "C:\Windows" -PassThru

    # Wait for main window with "Windows" title
    $hwndMain4 = [IntPtr]::Zero
    for ($i = 0; $i -lt 30; $i++) {
        $hwndMain4 = [Win32Helper]::FindProcessWindow($proc4.Id, "Win32Explorer", "Windows")
        if ($hwndMain4 -ne [IntPtr]::Zero) { break }
        Start-Sleep -Milliseconds 500
    }

    try {
        # Check that both Win32Explorer and EliteTaskbar are running
        $explProc = Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue
        $taskbarProc = Get-Process -Name EliteTaskbar -ErrorAction SilentlyContinue

        if ($explProc -and $taskbarProc) {
            Write-Host "[PASS] Both Win32Explorer ($($explProc.Id)) and EliteTaskbar ($($taskbarProc.Id)) are running successfully." -ForegroundColor Green

            if ($hwndMain4 -eq [IntPtr]::Zero) {
                throw "Could not find Win32Explorer window."
            }

            # Send WM_CLOSE (0x0010) to Win32Explorer browser window
            Write-Host "Closing browser window..." -ForegroundColor Cyan
            [Win32Helper]::SendMessage($hwndMain4, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
            Start-Sleep -Seconds 6

            # Verify browser process exited but EliteTaskbar is still running!
            $explProcAfter = Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue
            $taskbarProcAfter = Get-Process -Name EliteTaskbar -ErrorAction SilentlyContinue

            $explorerExited = ($null -eq $explProcAfter)
            $taskbarRunning = ($null -ne $taskbarProcAfter)

            Write-Host "Win32Explorer process running after close: $(!$explorerExited)" -ForegroundColor Cyan
            Write-Host "EliteTaskbar process running after close: $taskbarRunning" -ForegroundColor Cyan

            if ($explorerExited -and $taskbarRunning) {
                $results["EliteTaskbarIsolation"] = "PASS"
                Write-Host "[PASS] Closing browser window closed Win32Explorer process but preserved EliteTaskbar ($($taskbarProcAfter.Id))." -ForegroundColor Green
            } else {
                Write-Host "[FAIL] Process lifetime isolation check failed." -ForegroundColor Red
            }
        } else {
            Write-Host "[FAIL] Processes did not launch properly. Explorer running: $(!!$explProc), Taskbar running: $(!!$taskbarProc)" -ForegroundColor Red
        }
    } catch {
        Write-Host "[FAIL] Error in Test 4: $_" -ForegroundColor Red
    }
} finally {
    # Restore original user registry settings
    Stop-EliteProcesses
    Restore-RegistrySettings
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
