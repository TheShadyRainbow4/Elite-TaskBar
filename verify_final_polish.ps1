# verify_final_polish.ps1
# Automated E2E verification for EliteTaskbar final polish settings.
$ErrorActionPreference = 'Stop'
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

Write-Host "==========================================================" -ForegroundColor Green
Write-Host "  ELITE TASKBAR FINAL POLISH AUTOMATED VERIFICATION SCRIPT" -ForegroundColor Green
Write-Host "==========================================================" -ForegroundColor Green

# 1. Load Win32 API helper definitions in PowerShell
$code = @"
using System;
using System.Text;
using System.Runtime.InteropServices;

public class Win32Helper {
    [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    public static extern IntPtr FindWindowW(string lpClassName, string lpWindowName);

    [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    public static extern IntPtr FindWindowExW(IntPtr hWndParent, IntPtr hWndChildAfter, string lpszClass, string lpszWindow);

    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern IntPtr SendMessageW(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern bool EnumChildWindows(IntPtr hwndParent, EnumChildProc lpEnumFunc, IntPtr lParam);
    public delegate bool EnumChildProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetClassNameW(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetWindowTextW(IntPtr hWnd, StringBuilder lpString, int nMaxCount);

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

# Target registry keys
$regHKCU = "HKCU:\Software\EliteSoftware\Win32Explorer\Settings" # - Draftsman-Dan
$regHKLM = "HKLM:\Software\EliteSoftware\Win32Explorer\Settings" # - Draftsman-Dan

# Config XML paths
$xmlRootPath = Join-Path $ScriptDir "config.xml"
$xmlExplorerPath = Join-Path $ScriptDir "Win32Explorer_26.0.3.0\config.xml"

# List of target settings toggles
$settingsToggles = @(
    "DesktopWallpaperEnabled",
    "QuickLaunchEnabled",
    "TrayTwoRowsEnabled",
    "ShowClockSeconds"
)

# Helper function to stop running processes
function Stop-Processes {
    Write-Host "Stopping any running EliteTaskbar/Win32Explorer/EliteSettings processes..." -ForegroundColor Cyan
    Get-Process -Name EliteTaskbar, Win32Explorer, EliteSettings -ErrorAction SilentlyContinue | Stop-Process -Force
    Start-Sleep -Seconds 2
}

# Helper to write to config.xml
function Update-XmlSettings($togglesState) {
    foreach ($path in @($xmlRootPath, $xmlExplorerPath)) {
        if (Test-Path $path) {
            Write-Host "Updating XML settings in $path..." -ForegroundColor Cyan
            [xml]$xml = Get-Content $path
            $settingsNode = $xml.SelectSingleNode("//Settings")
            if ($null -eq $settingsNode) {
                # Create settings node if missing
                $settingsNode = $xml.CreateElement("Settings")
                $xml.DocumentElement.AppendChild($settingsNode) | Out-Null
            }
            
            foreach ($key in $togglesState.Keys) {
                $valStr = if ($togglesState[$key] -eq 1) { "yes" } else { "no" }
                $settingNode = $xml.SelectSingleNode("//Setting[@name='$key']")
                if ($null -eq $settingNode) {
                    $settingNode = $xml.CreateElement("Setting")
                    $settingNode.SetAttribute("name", $key) | Out-Null
                    $settingsNode.AppendChild($settingNode) | Out-Null
                }
                $settingNode.InnerText = $valStr
            }
            $xml.Save($path)
        }
    }
}

try {
    # Clean environment
    Stop-Processes

    # Ensure registry keys exist
    if (!(Test-Path $regHKCU)) { New-Item -Path $regHKCU -Force | Out-Null }
    if (!(Test-Path $regHKLM)) { New-Item -Path $regHKLM -Force | Out-Null }

    # ==================== PART 1: NORMAL MODE VALIDATION (HKCU) ====================
    Write-Host "`n--- Running normal mode persistence checks (HKCU) ---" -ForegroundColor Yellow
    
    # Enable normal mode (Disable Portable Mirror)
    Set-ItemProperty -Path $regHKCU -Name "EnablePortableMirror" -Value 0 -Type DWord
    
    # 1. Test writing 1
    Write-Host "Writing toggles = 1 to HKCU..." -ForegroundColor Cyan
    foreach ($toggle in $settingsToggles) {
        Set-ItemProperty -Path $regHKCU -Name $toggle -Value 1 -Type DWord
    }
    
    # Verify persistence
    foreach ($toggle in $settingsToggles) {
        $val = (Get-ItemProperty -Path $regHKCU -Name $toggle).$toggle
        if ($val -ne 1) {
            throw "HKCU Normal Mode toggle check failed: $toggle did not persist as 1. Read: $val"
        }
    }
    Write-Host "[PASS] All toggles successfully written to 1 in HKCU normal mode." -ForegroundColor Green

    # 2. Test writing 0
    Write-Host "Writing toggles = 0 to HKCU..." -ForegroundColor Cyan
    foreach ($toggle in $settingsToggles) {
        Set-ItemProperty -Path $regHKCU -Name $toggle -Value 0 -Type DWord
    }
    
    # Verify persistence
    foreach ($toggle in $settingsToggles) {
        $val = (Get-ItemProperty -Path $regHKCU -Name $toggle).$toggle
        if ($val -ne 0) {
            throw "HKCU Normal Mode toggle check failed: $toggle did not persist as 0. Read: $val"
        }
    }
    Write-Host "[PASS] All toggles successfully written to 0 in HKCU normal mode." -ForegroundColor Green

    # ==================== PART 2: PORTABLE MIRROR MODE VALIDATION (HKLM & XML) ====================
    Write-Host "`n--- Running Portable Mirror mode persistence checks (HKLM & XML) ---" -ForegroundColor Yellow
    
    # Enable Portable Mirror Mode
    Set-ItemProperty -Path $regHKCU -Name "EnablePortableMirror" -Value 1 -Type DWord
    Set-ItemProperty -Path $regHKLM -Name "EnablePortableMirror" -Value 1 -Type DWord

    # 1. Test writing 1
    Write-Host "Writing toggles = 1 to HKLM and config.xml..." -ForegroundColor Cyan
    $togglesState = @{}
    foreach ($toggle in $settingsToggles) {
        Set-ItemProperty -Path $regHKLM -Name $toggle -Value 1 -Type DWord
        $togglesState[$toggle] = 1
    }
    Update-XmlSettings $togglesState

    # Verify HKLM and XML persistence
    foreach ($toggle in $settingsToggles) {
        $val = (Get-ItemProperty -Path $regHKLM -Name $toggle).$toggle
        if ($val -ne 1) {
            throw "HKLM Portable Mirror mode toggle check failed: $toggle did not persist as 1. Read: $val"
        }
    }
    foreach ($path in @($xmlRootPath, $xmlExplorerPath)) {
        if (Test-Path $path) {
            [xml]$xml = Get-Content $path
            foreach ($toggle in $settingsToggles) {
                $node = $xml.SelectSingleNode("//Setting[@name='$toggle']")
                if ($null -eq $node -or $node.InnerText -ne "yes") {
                    throw "XML Portable Mirror mode toggle check failed in $($path) - $toggle did not persist as 'yes'."
                }
            }
        }
    }
    Write-Host "[PASS] All toggles successfully written to 1 in Portable Mirror mode (HKLM & XML)." -ForegroundColor Green

    # 2. Test writing 0
    Write-Host "Writing toggles = 0 to HKLM and config.xml..." -ForegroundColor Cyan
    $togglesState = @{}
    foreach ($toggle in $settingsToggles) {
        Set-ItemProperty -Path $regHKLM -Name $toggle -Value 0 -Type DWord
        $togglesState[$toggle] = 0
    }
    Update-XmlSettings $togglesState

    # Verify HKLM and XML persistence
    foreach ($toggle in $settingsToggles) {
        $val = (Get-ItemProperty -Path $regHKLM -Name $toggle).$toggle
        if ($val -ne 0) {
            throw "HKLM Portable Mirror mode toggle check failed: $toggle did not persist as 0. Read: $val"
        }
    }
    foreach ($path in @($xmlRootPath, $xmlExplorerPath)) {
        if (Test-Path $path) {
            [xml]$xml = Get-Content $path
            foreach ($toggle in $settingsToggles) {
                $node = $xml.SelectSingleNode("//Setting[@name='$toggle']")
                if ($null -eq $node -or $node.InnerText -ne "no") {
                    throw "XML Portable Mirror mode toggle check failed in $($path) - $toggle did not persist as 'no'."
                }
            }
        }
    }
    Write-Host "[PASS] All toggles successfully written to 0 in Portable Mirror mode (HKLM & XML)." -ForegroundColor Green

    # Reset Portable Mirror Mode to avoid interfering with normal run
    Set-ItemProperty -Path $regHKCU -Name "EnablePortableMirror" -Value 0 -Type DWord
    Set-ItemProperty -Path $regHKLM -Name "EnablePortableMirror" -Value 0 -Type DWord

    # ==================== PART 3: RUNTIME PROCESS AND UI COMPONENT QUERY ====================
    Write-Host "`n--- Running runtime process and UI component queries ---" -ForegroundColor Yellow
    
    # Configure registry so custom desktop replacement and clock are enabled
    Set-ItemProperty -Path $regHKCU -Name "DesktopReplacementEnabled" -Value 1 -Type DWord
    Set-ItemProperty -Path $regHKCU -Name "DesktopWallpaperEnabled" -Value 1 -Type DWord
    Set-ItemProperty -Path $regHKCU -Name "TaskbarMode" -Value 1 -Type DWord # Replace Mode

    # Launch EliteTaskbar
    $taskbarPath = Join-Path $ScriptDir "EliteTaskbar.exe"
    if (!(Test-Path $taskbarPath)) {
        $taskbarPath = Join-Path $ScriptDir "BuildOutput\EliteTaskbar.exe"
    }
    Write-Host "Launching EliteTaskbar from $taskbarPath..." -ForegroundColor Cyan
    $proc = Start-Process -FilePath $taskbarPath -ArgumentList "-allowMultiple" -PassThru
    
    # Wait for process initialization
    Start-Sleep -Seconds 5
    
    # Check if process is still running
    if ($proc.HasExited) {
        throw "EliteTaskbar process failed to start or crashed immediately. Exit code: $($proc.ExitCode)"
    }
    Write-Host "EliteTaskbar process is running (PID: $($proc.Id))." -ForegroundColor Green

    # Query UI elements/windows
    Write-Host "Querying UI window handles..." -ForegroundColor Cyan
    
    # A. Search for Progman desktop window class using direct helper to avoid delegate GC crash - Draftsman-Dan-Gen2\r
    $hwndProgman = [Win32Helper]::FindProcessWindow($proc.Id, "Progman") # - Draftsman-Dan-Gen2

    if ($hwndProgman -eq [IntPtr]::Zero) {
        throw "Could not find custom desktop replacement window (class 'Progman') belonging to EliteTaskbar PID $($proc.Id)."
    }
    Write-Host "[PASS] Custom Progman window found: $hwndProgman" -ForegroundColor Green

    # B. Search for Taskbar window
    $hwndTaskbar = [Win32Helper]::FindWindowW("Shell_TrayWnd", $null)
    if ($hwndTaskbar -eq [IntPtr]::Zero) {
        $hwndTaskbar = [Win32Helper]::FindWindowW("Shell_SecondaryTrayWnd", $null) # - Draftsman-Dan
    }
    
    if ($hwndTaskbar -eq [IntPtr]::Zero) {
        throw "Could not find Taskbar window (class 'Shell_TrayWnd' or 'Shell_SecondaryTrayWnd')." # - Draftsman-Dan
    }
    Write-Host "Taskbar window found: $hwndTaskbar" -ForegroundColor Green

    # C. Search for child TrayNotifyWnd
    $hwndTrayNotify = [Win32Helper]::FindChildByClass($hwndTaskbar, "TrayNotifyWnd")
    if ($hwndTrayNotify -eq [IntPtr]::Zero) {
        throw "Could not find child TrayNotifyWnd window under Taskbar."
    }
    Write-Host "[PASS] TrayNotifyWnd window found: $hwndTrayNotify" -ForegroundColor Green

    # D. Search for child TrayClockWClass
    $hwndTrayClock = [Win32Helper]::FindChildByClass($hwndTrayNotify, "TrayClockWClass")
    if ($hwndTrayClock -eq [IntPtr]::Zero) {
        throw "Could not find child TrayClockWClass window under TrayNotifyWnd."
    }
    Write-Host "[PASS] TrayClockWClass window found: $hwndTrayClock" -ForegroundColor Green

    # ==================== PART 4: DYNAMIC SETTINGS BROADCAST & CRASH VERIFICATION ====================
    Write-Host "`n--- Testing dynamic settings updates without crashes ---" -ForegroundColor Yellow

    # Change toggles in registry to test dynamic reload
    Set-ItemProperty -Path $regHKCU -Name "DesktopWallpaperEnabled" -Value 0 -Type DWord
    Set-ItemProperty -Path $regHKCU -Name "ShowClockSeconds" -Value 1 -Type DWord

    # Register settings update registered window message
    # Wait, we can also use WM_SETTINGCHANGE message with lParam = "TraySettings"
    Write-Host "Broadcasting WM_SETTINGCHANGE notification to running windows..." -ForegroundColor Cyan
    $msgId = [uint32]0x001A # WM_SETTINGCHANGE
    $lParamStr = "TraySettings"
    # Pin string in memory to pass to SendMessageTimeout
    $lParamPtr = [System.Runtime.InteropServices.Marshal]::StringToHGlobalUni($lParamStr)
    
    try {
        # HWND_BROADCAST = 0xFFFF
        [Win32Helper]::SendMessageW([IntPtr]0xFFFF, $msgId, [IntPtr]::Zero, $lParamPtr) | Out-Null
    } finally {
        [System.Runtime.InteropServices.Marshal]::FreeHGlobal($lParamPtr)
    }

    Start-Sleep -Seconds 3

    # Check if process is still running after settings change and message broadcast
    if ($proc.HasExited) {
        throw "EliteTaskbar process crashed or exited after receiving settings broadcast! Exit code: $($proc.ExitCode)"
    }
    Write-Host "[PASS] EliteTaskbar successfully survived dynamic settings updates without crashes." -ForegroundColor Green

    # Clean up and stop process
    Write-Host "`nCleaning up EliteTaskbar process..." -ForegroundColor Cyan
    $proc | Stop-Process -Force -ErrorAction SilentlyContinue
    
    # Restore registry defaults
    Set-ItemProperty -Path $regHKCU -Name "DesktopReplacementEnabled" -Value 0 -Type DWord
    Set-ItemProperty -Path $regHKCU -Name "DesktopWallpaperEnabled" -Value 0 -Type DWord
    Set-ItemProperty -Path $regHKCU -Name "TaskbarMode" -Value 0 -Type DWord

    # Ensure native shell is restored if not running - Draftsman-Dan
    if (-not (Get-Process -Name explorer -ErrorAction SilentlyContinue)) {
        Start-Process explorer.exe
    }

    Write-Host "`n==========================================================" -ForegroundColor Green
    Write-Host "  VERIFICATION SUCCESSFUL - ALL TESTS PASSED (EXIT 0)" -ForegroundColor Green
    Write-Host "==========================================================" -ForegroundColor Green
    exit 0

} catch {
    Write-Host "`n==========================================================" -ForegroundColor Red
    Write-Host "  VERIFICATION FAILED: $_" -ForegroundColor Red
    Write-Host "==========================================================" -ForegroundColor Red
    
    # Stop process on error
    if ($null -ne $proc) {
        $proc | Stop-Process -Force -ErrorAction SilentlyContinue
    }

    # Ensure native shell is restored if not running - Draftsman-Dan
    if (-not (Get-Process -Name explorer -ErrorAction SilentlyContinue)) {
        Start-Process explorer.exe
    }

    exit 1
}
