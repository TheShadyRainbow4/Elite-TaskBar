# run_re_verification.ps1
$ErrorActionPreference = 'Stop'

$ScriptDir = Split-Path -Parent -Path $MyInvocation.MyCommand.Definition
$ProjectRoot = Split-Path -Parent -Path $ScriptDir
$explorerExe = Join-Path $ProjectRoot "Win32Explorer.exe"
$xmlPath = Join-Path $ProjectRoot "config.xml"
$userSid = "S-1-5-21-3033238256-2936349959-1177579691-500" # default fallback
try {
    $userAccount = New-Object System.Security.Principal.NTAccount("User")
    $userSid = $userAccount.Translate([System.Security.Principal.SecurityIdentifier]).Value
} catch {}

$regExplorerPath = "Registry::HKEY_USERS\$userSid\Software\Win32Explorer"
$regSettingsPath = "Registry::HKEY_USERS\$userSid\Software\Win32Explorer\Settings"
$regAdvancedPath = "Registry::HKEY_USERS\$userSid\Software\EliteSoftware\Win32Explorer\Advanced"

# Helpers for Registry Cleaning
function Clear-RegistrySettings {
    if (Test-Path $regExplorerPath) {
        Remove-Item -Path $regExplorerPath -Recurse -Force -ErrorAction SilentlyContinue | Out-Null
    }
    if (Test-Path "Registry::HKEY_USERS\$userSid\Software\EliteSoftware\Win32Explorer") {
        Remove-Item -Path "Registry::HKEY_USERS\$userSid\Software\EliteSoftware\Win32Explorer" -Recurse -Force -ErrorAction SilentlyContinue | Out-Null
    }
}

function Stop-ExplorerProcesses {
    cmd /c "taskkill /F /IM Win32Explorer.exe >nul 2>nul"
    for ($i = 0; $i -lt 10; $i++) {
        $procs = Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue
        if ($null -eq $procs) { break }
        Stop-Process -Name Win32Explorer -Force -ErrorAction SilentlyContinue
        Start-Sleep -Milliseconds 500
    }
    if (Test-Path $xmlPath) { Remove-Item $xmlPath -Force | Out-Null }
}

# Win32 API Definitions for UI interactions
$code = @"
using System;
using System.Text;
using System.Runtime.InteropServices;

public class Win32Helper {
    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

    [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true)]
    public static extern int GetClassName(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetWindowTextW(IntPtr hWnd, StringBuilder lpString, int nMaxCount);

    [DllImport("user32.dll")]
    public static extern bool EnumChildWindows(IntPtr hwndParent, EnumChildProc lpEnumFunc, IntPtr lParam);
    public delegate bool EnumChildProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern IntPtr SendMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);

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

    public static IntPtr FindChildByClassAndId(IntPtr parent, string className, int controlId) {
        IntPtr found = IntPtr.Zero;
        EnumChildWindows(parent, (hWnd, lParam) => {
            StringBuilder sb = new StringBuilder(260);
            GetClassName(hWnd, sb, sb.Capacity);
            if (sb.ToString() == className) {
                int id = GetDlgCtrlID(hWnd);
                if (id == controlId) {
                    found = hWnd;
                    return false;
                }
            }
            return true;
        }, IntPtr.Zero);
        return found;
    }

    [DllImport("user32.dll")]
    public static extern int GetDlgCtrlID(IntPtr hwnd);

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

$results = @{
    "SmallIconTilesView" = "FAIL"
    "DefaultGroupByType" = "FAIL"
    "OptionsToggleRegistry" = "FAIL"
    "OptionsToggleXML" = "FAIL"
}

Write-Host "=== Win32Explorer Empirical Verification Script ===" -ForegroundColor Cyan

# ----------------- TEST 1: Small Icon Tiles View Mode -----------------
Write-Host "`n[TEST 1] Verifying 'Small Icon Tiles' View Mode (ViewMode 12)..." -ForegroundColor Yellow
Stop-ExplorerProcesses
Clear-RegistrySettings

# Setup registry for Small Icon Tiles view
if (!(Test-Path $regSettingsPath)) { New-Item -Path $regSettingsPath -Force | Out-Null }
Set-ItemProperty -Path $regSettingsPath -Name "ViewModeGlobal" -Value 12 -Type DWord
Set-ItemProperty -Path $regSettingsPath -Name "ConfirmCloseTabs" -Value 0 -Type DWord
Set-ItemProperty -Path $regSettingsPath -Name "EnableNativeViewMode" -Value 0 -Type DWord

& psexec64 -i 1 -d $explorerExe "C:\Windows"

# Poll for process
$pid1 = $null
for ($i = 0; $i -lt 20; $i++) {
    $proc1 = Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue
    if ($null -ne $proc1) { $pid1 = $proc1 | Select-Object -First 1 -ExpandProperty Id; break }
    Start-Sleep -Milliseconds 500
}

if ($null -eq $pid1) {
    Write-Host "[FAIL] Process failed to start." -ForegroundColor Red
} else {
    # Find main window
    $hwndMain1 = [IntPtr]::Zero
    for ($i = 0; $i -lt 30; $i++) {
        $hwndMain1 = [Win32Helper]::FindProcessWindow($pid1, "Win32Explorer", "Windows")
        if ($hwndMain1 -ne [IntPtr]::Zero) { break }
        Start-Sleep -Milliseconds 500
    }

    if ($hwndMain1 -eq [IntPtr]::Zero) {
        Write-Host "[FAIL] Could not find main window." -ForegroundColor Red
    } else {
        Start-Sleep -Seconds 2
        # Find ListView child control with polling
        $hwndLV1 = [IntPtr]::Zero
        for ($i = 0; $i -lt 15; $i++) {
            $hwndLV1 = [Win32Helper]::FindChildByClass($hwndMain1, "SysListView32")
            if ($hwndLV1 -ne [IntPtr]::Zero) { break }
            Start-Sleep -Milliseconds 500
        }
        if ($hwndLV1 -eq [IntPtr]::Zero) {
            Write-Host "[FAIL] Could not find SysListView32 child control." -ForegroundColor Red
        } else {
            # Programmatically switch view mode to Small Icon Tiles using command 60018 (IDM_VIEW_SMALLICONTILES)
            Write-Host "Sending IDM_VIEW_SMALLICONTILES (60018) command..." -ForegroundColor Cyan
            [Win32Helper]::SendMessage($hwndMain1, 0x0111, [IntPtr]60018, [IntPtr]::Zero) | Out-Null
            Start-Sleep -Seconds 2

            # Query runtime properties
            $view1 = [Win32Helper]::SendMessage($hwndLV1, 0x108F, [IntPtr]::Zero, [IntPtr]::Zero) # LVM_GETVIEW
            $himl1 = [Win32Helper]::SendMessage($hwndLV1, 0x1002, [IntPtr]0, [IntPtr]::Zero) # LVM_GETIMAGELIST (LVSIL_NORMAL)
            Write-Host "LVM_GETVIEW: $view1 (Expected: 4 = LV_VIEW_TILE)"
            Write-Host "LVSIL_NORMAL ImageList: $himl1"

            $runtimePass = $false
            if ($view1 -eq 4 -and $himl1 -ne [IntPtr]::Zero) {
                Write-Host "[PASS] Small Icon Tiles runtime checks passed." -ForegroundColor Green
                $runtimePass = $true
            } else {
                Write-Host "[FAIL] Small Icon Tiles view failed runtime checks." -ForegroundColor Red
            }
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
            if ($runtimePass -and $viewPersist -eq 12) {
                $results["SmallIconTilesView"] = "PASS"
            } else {
                Write-Host "[FAIL] ViewModeGlobal did not persist or runtime checks failed." -ForegroundColor Red
                $results["SmallIconTilesView"] = "FAIL"
            }
        } else {
            Write-Host "[FAIL] Registry settings key does not exist after close." -ForegroundColor Red
            $results["SmallIconTilesView"] = "FAIL"
        }
    }
}


# ----------------- TEST 2: Default Group by Type on First Run -----------------
Write-Host "`n[TEST 2] Verifying folders default to 'Group by Type' on first launch..." -ForegroundColor Yellow
Stop-ExplorerProcesses
Clear-RegistrySettings
if (!(Test-Path $regSettingsPath)) { New-Item -Path $regSettingsPath -Force | Out-Null }
Set-ItemProperty -Path $regSettingsPath -Name "EnableNativeViewMode" -Value 0 -Type DWord

& psexec64 -i 1 -d $explorerExe "C:\Windows"

# Poll for process
$pid2 = $null
for ($i = 0; $i -lt 20; $i++) {
    $proc2 = Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue
    if ($null -ne $proc2) { $pid2 = $proc2 | Select-Object -First 1 -ExpandProperty Id; break }
    Start-Sleep -Milliseconds 500
}

if ($null -eq $pid2) {
    Write-Host "[FAIL] Process failed to start." -ForegroundColor Red
} else {
    # Find main window
    $hwndMain2 = [IntPtr]::Zero
    for ($i = 0; $i -lt 30; $i++) {
        $hwndMain2 = [Win32Helper]::FindProcessWindow($pid2, "Win32Explorer", "Windows")
        if ($hwndMain2 -ne [IntPtr]::Zero) { break }
        Start-Sleep -Milliseconds 500
    }

    if ($hwndMain2 -eq [IntPtr]::Zero) {
        Write-Host "[FAIL] Could not find main window." -ForegroundColor Red
    } else {
        Start-Sleep -Seconds 2
        $hwndLV2 = [Win32Helper]::FindChildByClass($hwndMain2, "SysListView32")
        if ($hwndLV2 -eq [IntPtr]::Zero) {
            Write-Host "[FAIL] Could not find SysListView32 child control." -ForegroundColor Red
        } else {
            Start-Sleep -Seconds 4
            # Query if grouping is enabled: LVM_ISGROUPVIEWENABLED (0x10AF)
            $isGroupEnabled = [Win32Helper]::SendMessage($hwndLV2, 0x10AF, [IntPtr]::Zero, [IntPtr]::Zero)
            Write-Host "LVM_ISGROUPVIEWENABLED: $isGroupEnabled (Expected: 1)"

            # Close main window
            [Win32Helper]::SendMessage($hwndMain2, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
            
            # Wait for exit
            for ($i = 0; $i -lt 20; $i++) {
                $p = Get-Process -Id $pid2 -ErrorAction SilentlyContinue
                if ($null -eq $p) { break }
                Start-Sleep -Milliseconds 500
            }

            # Check registry settings
            if (Test-Path $regSettingsPath) {
                $defaultGroupVal = (Get-ItemProperty -Path $regSettingsPath -Name "EnableDefaultGroupByType" -ErrorAction SilentlyContinue).EnableDefaultGroupByType
                Write-Host "EnableDefaultGroupByType registry value: $defaultGroupVal (Expected: 1)" -ForegroundColor Cyan
                if ($isGroupEnabled -eq 1 -and $defaultGroupVal -eq 1) {
                    Write-Host "[PASS] Default Group by Type on first run verified." -ForegroundColor Green
                    $results["DefaultGroupByType"] = "PASS"
                } else {
                    Write-Host "[FAIL] Default Group by Type failed." -ForegroundColor Red
                }
            } else {
                Write-Host "[FAIL] Registry settings key does not exist after close." -ForegroundColor Red
            }
        }
    }
}


# ----------------- TEST 3: Options Dialog Toggle and Registry Sync -----------------
Write-Host "`n[TEST 3] Verifying Options dialog toggle and Registry synchronization..." -ForegroundColor Yellow
Stop-ExplorerProcesses
Clear-RegistrySettings

# Create settings key with default enabled
if (!(Test-Path $regSettingsPath)) { New-Item -Path $regSettingsPath -Force | Out-Null }
Set-ItemProperty -Path $regSettingsPath -Name "EnableDefaultGroupByType" -Value 1 -Type DWord
Set-ItemProperty -Path $regSettingsPath -Name "ConfirmCloseTabs" -Value 0 -Type DWord
Set-ItemProperty -Path $regSettingsPath -Name "EnableNativeViewMode" -Value 0 -Type DWord

& psexec64 -i 1 -d $explorerExe "C:\Windows"

# Poll for process
$pid3 = $null
for ($i = 0; $i -lt 20; $i++) {
    $proc3 = Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue
    if ($null -ne $proc3) { $pid3 = $proc3 | Select-Object -First 1 -ExpandProperty Id; break }
    Start-Sleep -Milliseconds 500
}

if ($null -eq $pid3) {
    Write-Host "[FAIL] Process failed to start." -ForegroundColor Red
} else {
    # Find main window
    $hwndMain3 = [IntPtr]::Zero
    for ($i = 0; $i -lt 30; $i++) {
        $hwndMain3 = [Win32Helper]::FindProcessWindow($pid3, "Win32Explorer", "Windows")
        if ($hwndMain3 -ne [IntPtr]::Zero) { break }
        Start-Sleep -Milliseconds 500
    }

    if ($hwndMain3 -eq [IntPtr]::Zero) {
        Write-Host "[FAIL] Could not find main window." -ForegroundColor Red
    } else {
        Start-Sleep -Seconds 2
        # Open Options dialog using PostMessage so it does not block the test script thread
        [Win32Helper]::PostMessage($hwndMain3, 0x0111, [IntPtr]40101, [IntPtr]::Zero) | Out-Null
        Start-Sleep -Seconds 3

        # Find Options Dialog Window (Title: "Options")
        $hwndOptions3 = [IntPtr]::Zero
        for ($i = 0; $i -lt 20; $i++) {
            $hwndOptions3 = [Win32Helper]::FindProcessWindow($pid3, "#32770", "Options")
            if ($hwndOptions3 -ne [IntPtr]::Zero) { break }
            Start-Sleep -Milliseconds 500
        }

        if ($hwndOptions3 -eq [IntPtr]::Zero) {
            Write-Host "[FAIL] Could not find Options dialog." -ForegroundColor Red
        } else {
            # Find the "Default Group by Type" checkbox control with polling
            $hwndCheckbox3 = [IntPtr]::Zero
            for ($i = 0; $i -lt 15; $i++) {
                $hwndCheckbox3 = [Win32Helper]::FindChildByClassAndId($hwndOptions3, "Button", 1382)
                if ($hwndCheckbox3 -ne [IntPtr]::Zero) { break }
                Start-Sleep -Milliseconds 500
            }
            if ($hwndCheckbox3 -eq [IntPtr]::Zero) {
                Write-Host "[FAIL] Could not find Default Group by Type checkbox control (ID 1382)." -ForegroundColor Red
            } else {
                # Click the checkbox (toggle it) by sending BM_SETCHECK (0x00F1)
                # Note: Checkbox was loaded as Checked (1). Toggling will uncheck it (wParam = 0).
                [Win32Helper]::SendMessage($hwndCheckbox3, 0x00F1, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
                Start-Sleep -Seconds 1

                # Click "Okay" button to save and close the dialog (Okay Button ID is 1 = IDOK)
                [Win32Helper]::SetForegroundWindow($hwndOptions3) | Out-Null
                Start-Sleep -Milliseconds 200
                [Win32Helper]::SendMessage($hwndOptions3, 0x0111, [IntPtr]1, [IntPtr]::Zero) | Out-Null
                Start-Sleep -Seconds 2

                # Close main window
                [Win32Helper]::SendMessage($hwndMain3, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
                
                # Wait for exit
                for ($i = 0; $i -lt 20; $i++) {
                    $p = Get-Process -Id $pid3 -ErrorAction SilentlyContinue
                    if ($null -eq $p) { break }
                    Start-Sleep -Milliseconds 500
                }

                # Verify registry has EnableDefaultGroupByType = 0
                if (Test-Path $regSettingsPath) {
                    $defaultGroupVal = (Get-ItemProperty -Path $regSettingsPath -Name "EnableDefaultGroupByType" -ErrorAction SilentlyContinue).EnableDefaultGroupByType
                    Write-Host "Registry EnableDefaultGroupByType after change: $defaultGroupVal (Expected: 0)" -ForegroundColor Cyan
                    if ($defaultGroupVal -eq 0) {
                        Write-Host "[PASS] Checkbox successfully updated registry setting." -ForegroundColor Green
                        $results["OptionsToggleRegistry"] = "PASS"
                    } else {
                        Write-Host "[FAIL] Registry setting did not update." -ForegroundColor Red
                    }
                } else {
                    Write-Host "[FAIL] Registry settings key does not exist after close." -ForegroundColor Red
                }
            }
        }
    }
}


# ----------------- TEST 4: Options Dialog Toggle and XML Sync -----------------
Write-Host "`n[TEST 4] Verifying Options dialog toggle and XML synchronization (Portable Mirror)..." -ForegroundColor Yellow
Stop-ExplorerProcesses
Clear-RegistrySettings

# Enable Portable Mirror inside HKCU settings
if (!(Test-Path $regSettingsPath)) { New-Item -Path $regSettingsPath -Force | Out-Null }
Set-ItemProperty -Path $regSettingsPath -Name "EnablePortableMirror" -Value 1 -Type DWord
Set-ItemProperty -Path $regSettingsPath -Name "ConfirmCloseTabs" -Value 0 -Type DWord

& psexec64 -i 1 -d $explorerExe "C:\Windows"

# Poll for process
$pid4 = $null
for ($i = 0; $i -lt 20; $i++) {
    $proc4 = Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue
    if ($null -ne $proc4) { $pid4 = $proc4 | Select-Object -First 1 -ExpandProperty Id; break }
    Start-Sleep -Milliseconds 500
}

if ($null -eq $pid4) {
    Write-Host "[FAIL] Process failed to start." -ForegroundColor Red
} else {
    # Find main window
    $hwndMain4 = [IntPtr]::Zero
    for ($i = 0; $i -lt 30; $i++) {
        $hwndMain4 = [Win32Helper]::FindProcessWindow($pid4, "Win32Explorer", "Windows")
        if ($hwndMain4 -ne [IntPtr]::Zero) { break }
        Start-Sleep -Milliseconds 500
    }

    if ($hwndMain4 -eq [IntPtr]::Zero) {
        Write-Host "[FAIL] Could not find main window." -ForegroundColor Red
    } else {
        Start-Sleep -Seconds 2
        # Open Options dialog using PostMessage so it does not block the test script thread
        [Win32Helper]::PostMessage($hwndMain4, 0x0111, [IntPtr]40101, [IntPtr]::Zero) | Out-Null
        Start-Sleep -Seconds 3

        # Find Options Dialog Window
        $hwndOptions4 = [IntPtr]::Zero
        for ($i = 0; $i -lt 20; $i++) {
            $hwndOptions4 = [Win32Helper]::FindProcessWindow($pid4, "#32770", "Options")
            if ($hwndOptions4 -ne [IntPtr]::Zero) { break }
            Start-Sleep -Milliseconds 500
        }

        if ($hwndOptions4 -eq [IntPtr]::Zero) {
            Write-Host "[FAIL] Could not find Options dialog." -ForegroundColor Red
        } else {
            # Find checkbox with polling
            $hwndCheckbox4 = [IntPtr]::Zero
            for ($i = 0; $i -lt 15; $i++) {
                $hwndCheckbox4 = [Win32Helper]::FindChildByClassAndId($hwndOptions4, "Button", 1382)
                if ($hwndCheckbox4 -ne [IntPtr]::Zero) { break }
                Start-Sleep -Milliseconds 500
            }
            if ($hwndCheckbox4 -eq [IntPtr]::Zero) {
                Write-Host "[FAIL] Could not find Default Group by Type checkbox control." -ForegroundColor Red
            } else {
                # Click checkbox (toggles default setting to 0)
                [Win32Helper]::SendMessage($hwndCheckbox4, 0x00F1, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
                Start-Sleep -Seconds 1

                # Click Okay
                [Win32Helper]::SetForegroundWindow($hwndOptions4) | Out-Null
                Start-Sleep -Milliseconds 200
                [Win32Helper]::SendMessage($hwndOptions4, 0x0111, [IntPtr]1, [IntPtr]::Zero) | Out-Null
                Start-Sleep -Seconds 2

                # Close main window
                [Win32Helper]::SendMessage($hwndMain4, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
                
                # Wait for exit
                for ($i = 0; $i -lt 20; $i++) {
                    $p = Get-Process -Id $pid4 -ErrorAction SilentlyContinue
                    if ($null -eq $p) { break }
                    Start-Sleep -Milliseconds 500
                }

                # Verify XML config exists and contains EnableDefaultGroupByType="no"
                if (Test-Path $xmlPath) {
                    $xmlContent = Get-Content $xmlPath -Raw
                    Write-Host "config.xml contents contains EnableDefaultGroupByType:" -ForegroundColor Cyan
                    if ($xmlContent -match 'EnableDefaultGroupByType.*no') {
                        Write-Host "[PASS] Checkbox successfully updated config.xml." -ForegroundColor Green
                        $results["OptionsToggleXML"] = "PASS"
                    } else {
                        Write-Host "[FAIL] config.xml does not contain EnableDefaultGroupByType=\"no\"." -ForegroundColor Red
                        Write-Host "$xmlContent"
                    }
                } else {
                    Write-Host "[FAIL] config.xml was not created." -ForegroundColor Red
                }
            }
        }
    }
}


# ----------------- SUMMARY -----------------
Write-Host "`n=== Test Results Summary ===" -ForegroundColor Cyan
$results.Keys | ForEach-Object {
    Write-Host "  $_ : [$($results[$_])]"
}

$overallVerdict = "PASS"
if ($results.Values -contains "FAIL" -or $results.Values -like "FAIL*") {
    $overallVerdict = "FAIL"
}

$color = "Red"
if ($overallVerdict -eq "PASS") {
    $color = "Green"
}
Write-Host "Overall Verdict: $overallVerdict" -ForegroundColor $color

# Write results to output file
$resultsPath = Join-Path $ScriptDir "test_results.txt"
$results.Keys | ForEach-Object { "$_ : $($results[$_])" } | Out-File -FilePath $resultsPath -Encoding utf8
"Verdict : $overallVerdict" | Out-File -FilePath (Join-Path $ScriptDir "verdict.txt") -Encoding utf8

if ($overallVerdict -eq "FAIL") {
    exit 1
} else {
    exit 0
}
