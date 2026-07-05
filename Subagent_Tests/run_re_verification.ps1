# run_re_verification.ps1
# Automated Empirical Verification for Challenger Turn
$ErrorActionPreference = 'Stop'
$ScriptDir = "C:\Users\Administrator\Desktop\Elite-TaskBar"
$MyDir = "C:\Users\Administrator\Desktop\Elite-TaskBar\.agents\challenger_m3_re_2"

Write-Host "=== Win32Explorer Empirical Verification Script ===" -ForegroundColor Cyan

# Load Win32 Helper definitions
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

$explorerExe = Join-Path $ScriptDir "Win32Explorer.exe"
$xmlPath = Join-Path $ScriptDir "config.xml"

# Registry paths
$regSettingsPath = "HKCU:\Software\Win32Explorer\Settings"
$regSettingsBackupName = "Settings_Backup_Re"
$regSettingsBackupPath = "HKCU:\Software\Win32Explorer\Settings_Backup_Re"

$regAdvancedPath = "HKCU:\Software\EliteSoftware\Win32Explorer\Advanced"

function Stop-ExplorerProcesses {
    Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue | Stop-Process -Force
    Start-Sleep -Seconds 1
    if (Test-Path $xmlPath) { Remove-Item $xmlPath -Force | Out-Null }
}

function Clear-RegistrySettings {
    if (Test-Path $regSettingsPath) {
        Remove-Item $regSettingsPath -Recurse -Force -ErrorAction SilentlyContinue | Out-Null
    }
}

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
    "OptionsToggleRegistry" = "FAIL"
    "OptionsToggleXML" = "FAIL"
}

# Back up original settings
Stop-ExplorerProcesses
Backup-RegistrySettings

# Save original Advanced value for EnablePortableMirror if it exists
$origMirror = $null
if (Test-Path $regAdvancedPath) {
    $origMirror = (Get-ItemProperty -Path $regAdvancedPath -Name "EnablePortableMirror" -ErrorAction SilentlyContinue).EnablePortableMirror
}

try {
    # ----------------- TEST 1: Small Icon Tiles View Mode -----------------
    Write-Host "`n[TEST 1] Verifying 'Small Icon Tiles' View Mode (ViewMode 12)..." -ForegroundColor Yellow
    Clear-RegistrySettings
    Stop-ExplorerProcesses

    # Setup registry with default/empty, set confirm close tabs to 0
    if (!(Test-Path $regSettingsPath)) { New-Item -Path $regSettingsPath -Force | Out-Null }
    Set-ItemProperty -Path $regSettingsPath -Name "ConfirmCloseTabs" -Value 0 -Type DWord
    if (Test-Path $regAdvancedPath) {
        Set-ItemProperty -Path $regAdvancedPath -Name "EnablePortableMirror" -Value 0 -Type DWord
    }

    # Launch using PsExec in session 1
    & psexec64 -i 1 -d $explorerExe "C:\Windows"

    # Poll for process launch
    $proc1 = $null
    for ($i = 0; $i -lt 30; $i++) {
        $proc1 = Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue
        if ($null -ne $proc1) { break }
        Start-Sleep -Milliseconds 500
    }
    if ($null -eq $proc1) { throw "Win32Explorer failed to launch." }
    $pid1 = $proc1.Id

    # Wait for window
    $hwndMain1 = [IntPtr]::Zero
    for ($i = 0; $i -lt 30; $i++) {
        $hwndMain1 = [Win32Helper]::FindProcessWindow($pid1, "Win32Explorer", "Windows")
        if ($hwndMain1 -ne [IntPtr]::Zero) { break }
        Start-Sleep -Milliseconds 500
    }

    if ($hwndMain1 -eq [IntPtr]::Zero) {
        throw "Could not find Win32Explorer window with 'Windows' title."
    }

    $hwndLV1 = [Win32Helper]::FindChildByClass($hwndMain1, "SysListView32")
    if ($hwndLV1 -eq [IntPtr]::Zero) {
        throw "Could not find SysListView32 child control."
    }

    # Programmatically switch view mode to Small Icon Tiles using command 60018 (IDM_VIEW_SMALLICONTILES)
    Write-Host "Sending IDM_VIEW_SMALLICONTILES (60018) command..." -ForegroundColor Cyan
    [Win32Helper]::SendMessage($hwndMain1, 0x0111, [IntPtr]60018, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Seconds 2

    # Wait for view mode to settle to LV_VIEW_TILE (4)
    $view = 0
    for ($i = 0; $i -lt 20; $i++) {
        $view = [Win32Helper]::SendMessage($hwndLV1, 0x108F, [IntPtr]::Zero, [IntPtr]::Zero) # LVM_GETVIEW
        if ($view -eq 4) { break }
        Start-Sleep -Milliseconds 500
    }
    Write-Host "LVM_GETVIEW: $view (Expected: 4 = LV_VIEW_TILE)" -ForegroundColor Cyan

    # Normal ImageList check
    $himlNormal = [Win32Helper]::SendMessage($hwndLV1, 0x1002, [IntPtr]0, [IntPtr]::Zero) # LVM_GETIMAGELIST (LVSIL_NORMAL = 0)
    Write-Host "LVSIL_NORMAL ImageList: $himlNormal" -ForegroundColor Cyan

    # In Small Icon Tiles view, LVSIL_NORMAL should be populated with the small image list.
    if ($view -eq 4 -and $himlNormal -ne [IntPtr]::Zero) {
        $results["SmallIconTilesView"] = "PASS"
        Write-Host "[PASS] Small Icon Tiles view works at runtime." -ForegroundColor Green
    } else {
        Write-Host "[FAIL] Small Icon Tiles view failed runtime checks." -ForegroundColor Red
    }

    # Close main window
    [Win32Helper]::SendMessage($hwndMain1, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
    
    # Wait for exit
    for ($i = 0; $i -lt 20; $i++) {
        $p = Get-Process -Id $pid1 -ErrorAction SilentlyContinue
        if ($null -eq $p) { break }
        Start-Sleep -Milliseconds 500
    }

    # Check persistence
    if (Test-Path $regSettingsPath) {
        $viewPersist = (Get-ItemProperty -Path $regSettingsPath -Name "ViewModeGlobal" -ErrorAction SilentlyContinue).ViewModeGlobal
        Write-Host "ViewModeGlobal in registry: $viewPersist (Expected: 12)" -ForegroundColor Cyan
        if ($viewPersist -eq 12) {
            Write-Host "[PASS] ViewModeGlobal persistence verified." -ForegroundColor Green
        } else {
            $results["SmallIconTilesView"] = "FAIL (No persist)"
            Write-Host "[FAIL] ViewModeGlobal did not persist." -ForegroundColor Red
        }
    } else {
        Write-Host "[FAIL] Registry settings key does not exist after close." -ForegroundColor Red
    }

    # ----------------- TEST 2: Default Group by Type on First Run -----------------
    Write-Host "`n[TEST 2] Verifying folders default to 'Group by Type' on first launch..." -ForegroundColor Yellow
    Clear-RegistrySettings
    Stop-ExplorerProcesses

    if (!(Test-Path $regSettingsPath)) { New-Item -Path $regSettingsPath -Force | Out-Null }
    Set-ItemProperty -Path $regSettingsPath -Name "ConfirmCloseTabs" -Value 0 -Type DWord

    # Launch using PsExec in session 1
    & psexec64 -i 1 -d $explorerExe "C:\Windows"

    # Poll for process launch
    $proc2 = $null
    for ($i = 0; $i -lt 30; $i++) {
        $proc2 = Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue
        if ($null -ne $proc2) { break }
        Start-Sleep -Milliseconds 500
    }
    if ($null -eq $proc2) { throw "Win32Explorer failed to launch." }
    $pid2 = $proc2.Id

    # Wait for window
    $hwndMain2 = [IntPtr]::Zero
    for ($i = 0; $i -lt 30; $i++) {
        $hwndMain2 = [Win32Helper]::FindProcessWindow($pid2, "Win32Explorer", "Windows")
        if ($hwndMain2 -ne [IntPtr]::Zero) { break }
        Start-Sleep -Milliseconds 500
    }

    if ($hwndMain2 -eq [IntPtr]::Zero) {
        throw "Could not find Win32Explorer window."
    }

    $hwndLV2 = [Win32Helper]::FindChildByClass($hwndMain2, "SysListView32")
    if ($hwndLV2 -eq [IntPtr]::Zero) {
        throw "Could not find SysListView32 child control."
    }

    # Wait for group view to be enabled (1)
    $groupEnabled = 0
    for ($i = 0; $i -lt 20; $i++) {
        $groupEnabled = [Win32Helper]::SendMessage($hwndLV2, 0x10AF, [IntPtr]::Zero, [IntPtr]::Zero) # LVM_ISGROUPVIEWENABLED
        if ($groupEnabled -eq 1) { break }
        Start-Sleep -Milliseconds 500
    }
    Write-Host "LVM_ISGROUPVIEWENABLED: $groupEnabled (Expected: 1)" -ForegroundColor Cyan

    # Close window to flush settings
    [Win32Helper]::SendMessage($hwndMain2, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
    
    # Wait for exit
    for ($i = 0; $i -lt 20; $i++) {
        $p = Get-Process -Id $pid2 -ErrorAction SilentlyContinue
        if ($null -eq $p) { break }
        Start-Sleep -Milliseconds 500
    }

    # Check registry default Group by Type
    if (Test-Path $regSettingsPath) {
        $valGroup = (Get-ItemProperty -Path $regSettingsPath -Name "EnableDefaultGroupByType" -ErrorAction SilentlyContinue).EnableDefaultGroupByType
        Write-Host "EnableDefaultGroupByType registry value: $valGroup (Expected: 1)" -ForegroundColor Cyan

        if ($groupEnabled -eq 1 -and $valGroup -eq 1) {
            $results["DefaultGroupByType"] = "PASS"
            Write-Host "[PASS] Default Group by Type on first run verified." -ForegroundColor Green
        } else {
            Write-Host "[FAIL] Default Group by Type on first run failed." -ForegroundColor Red
        }
    } else {
        Write-Host "[FAIL] Registry settings key does not exist after close." -ForegroundColor Red
    }

    # ----------------- TEST 3: General Options Checkbox and Registry Save -----------------
    Write-Host "`n[TEST 3] Verifying Options dialog toggle and Registry synchronization..." -ForegroundColor Yellow
    Clear-RegistrySettings
    Stop-ExplorerProcesses

    if (!(Test-Path $regSettingsPath)) { New-Item -Path $regSettingsPath -Force | Out-Null }
    Set-ItemProperty -Path $regSettingsPath -Name "EnableDefaultGroupByType" -Value 1 -Type DWord
    Set-ItemProperty -Path $regSettingsPath -Name "ConfirmCloseTabs" -Value 0 -Type DWord

    # Launch using PsExec in session 1
    & psexec64 -i 1 -d $explorerExe "C:\Windows"

    # Poll for process launch
    $proc3 = $null
    for ($i = 0; $i -lt 30; $i++) {
        $proc3 = Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue
        if ($null -ne $proc3) { break }
        Start-Sleep -Milliseconds 500
    }
    if ($null -eq $proc3) { throw "Win32Explorer failed to launch." }
    $pid3 = $proc3.Id

    $hwndMain3 = [IntPtr]::Zero
    for ($i = 0; $i -lt 30; $i++) {
        $hwndMain3 = [Win32Helper]::FindProcessWindow($pid3, "Win32Explorer", "Windows")
        if ($hwndMain3 -ne [IntPtr]::Zero) { break }
        Start-Sleep -Milliseconds 500
    }

    if ($hwndMain3 -eq [IntPtr]::Zero) { throw "Could not find main window." }

    # Open Options dialog (WM_COMMAND 40101)
    [Win32Helper]::SendMessage($hwndMain3, 0x0111, [IntPtr]40101, [IntPtr]::Zero) | Out-Null

    $hwndOpt3 = [IntPtr]::Zero
    for ($i = 0; $i -lt 20; $i++) {
        $hwndOpt3 = [Win32Helper]::FindProcessWindow($pid3, "#32770", "Options")
        if ($hwndOpt3 -ne [IntPtr]::Zero) { break }
        Start-Sleep -Milliseconds 500
    }

    if ($hwndOpt3 -eq [IntPtr]::Zero) { throw "Could not find Options dialog." }

    # Find IDC_OPTION_DEFAULTGROUPBYTYPE checkbox (ID 1382)
    $hwndChk3 = [Win32Helper]::FindChildById($hwndOpt3, 1382)
    if ($hwndChk3 -eq [IntPtr]::Zero) { throw "Could not find checkbox (ID 1382)." }

    # Uncheck it
    [Win32Helper]::SendMessage($hwndChk3, 0x00F1, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
    $hwndParent3 = [Win32Helper]::GetParent($hwndChk3)
    [Win32Helper]::SendMessage($hwndParent3, 0x0111, [IntPtr]1382, $hwndChk3) | Out-Null
    Start-Sleep -Milliseconds 200

    # Save and close (IDOK = 1)
    [Win32Helper]::SendMessage($hwndOpt3, 0x0111, [IntPtr]1, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Seconds 2

    # Close main window
    [Win32Helper]::SendMessage($hwndMain3, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
    
    # Wait for exit
    for ($i = 0; $i -lt 20; $i++) {
        $p = Get-Process -Id $pid3 -ErrorAction SilentlyContinue
        if ($null -eq $p) { break }
        Start-Sleep -Milliseconds 500
    }

    # Check registry value updated to 0
    if (Test-Path $regSettingsPath) {
        $valGroup3 = (Get-ItemProperty -Path $regSettingsPath -Name "EnableDefaultGroupByType" -ErrorAction SilentlyContinue).EnableDefaultGroupByType
        Write-Host "Registry EnableDefaultGroupByType after change: $valGroup3 (Expected: 0)" -ForegroundColor Cyan

        if ($valGroup3 -eq 0) {
            $results["OptionsToggleRegistry"] = "PASS"
            Write-Host "[PASS] Checkbox successfully updated registry setting." -ForegroundColor Green
        } else {
            Write-Host "[FAIL] Checkbox failed to update registry setting." -ForegroundColor Red
        }
    } else {
        Write-Host "[FAIL] Registry settings key does not exist after close." -ForegroundColor Red
    }

    # ----------------- TEST 4: General Options Checkbox and XML Save (Portable Mirror) -----------------
    Write-Host "`n[TEST 4] Verifying Options dialog toggle and XML synchronization (Portable Mirror)..." -ForegroundColor Yellow
    Clear-RegistrySettings
    Stop-ExplorerProcesses

    # Enable Portable Mirror in Advanced key
    if (!(Test-Path $regAdvancedPath)) { New-Item -Path $regAdvancedPath -Force | Out-Null }
    Set-ItemProperty -Path $regAdvancedPath -Name "EnablePortableMirror" -Value 1 -Type DWord

    if (!(Test-Path $regSettingsPath)) { New-Item -Path $regSettingsPath -Force | Out-Null }
    Set-ItemProperty -Path $regSettingsPath -Name "EnableDefaultGroupByType" -Value 1 -Type DWord
    Set-ItemProperty -Path $regSettingsPath -Name "ConfirmCloseTabs" -Value 0 -Type DWord

    # Launch using PsExec in session 1
    & psexec64 -i 1 -d $explorerExe "C:\Windows"

    # Poll for process launch
    $proc4 = $null
    for ($i = 0; $i -lt 30; $i++) {
        $proc4 = Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue
        if ($null -ne $proc4) { break }
        Start-Sleep -Milliseconds 500
    }
    if ($null -eq $proc4) { throw "Win32Explorer failed to launch." }
    $pid4 = $proc4.Id

    $hwndMain4 = [IntPtr]::Zero
    for ($i = 0; $i -lt 30; $i++) {
        $hwndMain4 = [Win32Helper]::FindProcessWindow($pid4, "Win32Explorer", "Windows")
        if ($hwndMain4 -ne [IntPtr]::Zero) { break }
        Start-Sleep -Milliseconds 500
    }

    if ($hwndMain4 -eq [IntPtr]::Zero) { throw "Could not find main window." }

    # Open Options dialog (WM_COMMAND 40101)
    [Win32Helper]::SendMessage($hwndMain4, 0x0111, [IntPtr]40101, [IntPtr]::Zero) | Out-Null

    $hwndOpt4 = [IntPtr]::Zero
    for ($i = 0; $i -lt 20; $i++) {
        $hwndOpt4 = [Win32Helper]::FindProcessWindow($pid4, "#32770", "Options")
        if ($hwndOpt4 -ne [IntPtr]::Zero) { break }
        Start-Sleep -Milliseconds 500
    }

    if ($hwndOpt4 -eq [IntPtr]::Zero) { throw "Could not find Options dialog." }

    # Find IDC_OPTION_DEFAULTGROUPBYTYPE checkbox (ID 1382)
    $hwndChk4 = [Win32Helper]::FindChildById($hwndOpt4, 1382)
    if ($hwndChk4 -eq [IntPtr]::Zero) { throw "Could not find checkbox (ID 1382)." }

    # Uncheck it
    [Win32Helper]::SendMessage($hwndChk4, 0x00F1, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
    $hwndParent4 = [Win32Helper]::GetParent($hwndChk4)
    [Win32Helper]::SendMessage($hwndParent4, 0x0111, [IntPtr]1382, $hwndChk4) | Out-Null
    Start-Sleep -Milliseconds 200

    # Save and close (IDOK = 1)
    [Win32Helper]::SendMessage($hwndOpt4, 0x0111, [IntPtr]1, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Seconds 2

    # Close main window
    [Win32Helper]::SendMessage($hwndMain4, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
    
    # Wait for exit
    for ($i = 0; $i -lt 20; $i++) {
        $p = Get-Process -Id $pid4 -ErrorAction SilentlyContinue
        if ($null -eq $p) { break }
        Start-Sleep -Milliseconds 500
    }

    # Check XML content
    if (Test-Path $xmlPath) {
        $xmlContent = Get-Content $xmlPath -Raw
        Write-Host "config.xml was successfully created." -ForegroundColor Cyan
        
        if ($xmlContent -match '<Setting\s+name="EnableDefaultGroupByType">no</Setting>') {
            $results["OptionsToggleXML"] = "PASS"
            Write-Host "[PASS] Checkbox successfully updated XML setting." -ForegroundColor Green
        } else {
            Write-Host "[FAIL] XML setting did not update. XML content: $xmlContent" -ForegroundColor Red
        }
    } else {
        Write-Host "[FAIL] config.xml was not created." -ForegroundColor Red
    }

} catch {
    Write-Host "[FATAL ERROR] Test crashed: $_" -ForegroundColor Red
} finally {
    Stop-ExplorerProcesses
    Restore-RegistrySettings
    
    # Restore original advanced value
    if ($null -ne $origMirror) {
        Set-ItemProperty -Path $regAdvancedPath -Name "EnablePortableMirror" -Value $origMirror -Type DWord
    } else {
        if (Test-Path $regAdvancedPath) {
            Remove-ItemProperty -Path $regAdvancedPath -Name "EnablePortableMirror" -ErrorAction SilentlyContinue
        }
    }
}

# Print Results
Write-Host "`n=== Test Results Summary ===" -ForegroundColor Cyan
$overall = "PASS"
foreach ($key in $results.Keys) {
    $res = $results[$key]
    if ($res -eq "PASS") {
        Write-Host "  $key : [PASS]" -ForegroundColor Green
    } else {
        Write-Host "  $key : [$res]" -ForegroundColor Red
        $overall = "FAIL"
    }
}
$color = if ($overall -eq "PASS") { "Green" } else { "Red" }
Write-Host "Overall Verdict: $overall" -ForegroundColor $color

# Save final verdict
$overall | Out-File -FilePath (Join-Path $MyDir "verdict.txt") -Force
$results | Out-String | Out-File -FilePath (Join-Path $MyDir "test_results.txt") -Force

$exitVal = if ($overall -eq "PASS") { 0 } else { 1 }
exit $exitVal
