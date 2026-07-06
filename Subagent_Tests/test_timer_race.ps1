# test_timer_race.ps1
$ErrorActionPreference = 'Stop'
$ScriptDir = "C:\Users\Administrator\Desktop\Elite-TaskBar"

# Helper Win32 functions
$code = @"
using System;
using System.Text;
using System.Runtime.InteropServices;

public class Win32 {
    [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    public static extern int GetClassNameW(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);

    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

    [DllImport("user32.dll")]
    public static extern bool PostMessageW(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
    public const uint WM_DISPLAYCHANGE = 0x007E;

    public static IntPtr FindProcessWindow(int processId, string className) {
        IntPtr found = IntPtr.Zero;
        EnumWindows((hWnd, lParam) => {
            uint pid;
            GetWindowThreadProcessId(hWnd, out pid);
            if (pid == processId) {
                StringBuilder sbClass = new StringBuilder(260);
                GetClassNameW(hWnd, sbClass, sbClass.Capacity);
                if (sbClass.ToString() == className) {
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

function Stop-EliteProcesses {
    Get-Process -Name EliteTaskbar, EST* -ErrorAction SilentlyContinue | Stop-Process -Force
    Start-Sleep -Seconds 1
}

Stop-EliteProcesses

# 1. Setup registry to slideshow enabled, interval 3
$regPath = "HKCU:\Software\EliteSoftware\Win32Explorer\Advanced"
Set-ItemProperty -Path $regPath -Name "DesktopWallpaperEnabled" -Value 1 -Type DWord
Set-ItemProperty -Path $regPath -Name "DesktopSlideshowEnabled" -Value 1 -Type DWord
Set-ItemProperty -Path $regPath -Name "DesktopSlideshowInterval" -Value 3 -Type DWord
Set-ItemProperty -Path $regPath -Name "DesktopThemePath" -Value "C:\Users\Administrator\Desktop\Elite-TaskBar\Subagent_Tests\test_theme" -Type String
Set-ItemProperty -Path $regPath -Name "DesktopReplacementEnabled" -Value 1 -Type DWord
Set-ItemProperty -Path $regPath -Name "ForceProgmanAllDisplays" -Value 1 -Type DWord

# 2. Launch EliteTaskbar.exe
Write-Host "Launching EliteTaskbar..."
$proc = Start-Process -FilePath (Join-Path $ScriptDir "EliteTaskbar.exe") -ArgumentList "-allowMultiple" -PassThru

# Poll for window creation
$hwndProgman = [IntPtr]::Zero
for ($i = 0; $i -lt 20; $i++) {
    $hwndProgman = [Win32]::FindProcessWindow($proc.Id, "Progman")
    if ($hwndProgman -ne [IntPtr]::Zero) { break }
    Start-Sleep -Milliseconds 500
}

Write-Host "Found custom Progman window HWND: $hwndProgman"

if ($hwndProgman -eq [IntPtr]::Zero) {
    Write-Host "Error: Custom Progman window was not created within timeout."
    Stop-EliteProcesses
    exit 1
}

# 3. Change interval to 4 in registry to force mismatch
Write-Host "Changing interval in registry to 4 to force timer reset..."
Set-ItemProperty -Path $regPath -Name "DesktopSlideshowInterval" -Value 4 -Type DWord

# 4. Send WM_DISPLAYCHANGE to trigger repaint and re-evaluate registry settings
Write-Host "Sending WM_DISPLAYCHANGE to Progman..."
[Win32]::PostMessageW($hwndProgman, [Win32]::WM_DISPLAYCHANGE, [IntPtr]32, [IntPtr]0) | Out-Null
Start-Sleep -Seconds 2

# 5. Check locks
$img1Path = "C:\Users\Administrator\Desktop\Elite-TaskBar\Subagent_Tests\test_theme\img1.jpg"
$img2Path = "C:\Users\Administrator\Desktop\Elite-TaskBar\Subagent_Tests\test_theme\img2.jpg"

function Is-FileLocked($path) {
    try {
        $file = [System.IO.File]::Open($path, 'Open', 'Write', 'None')
        $file.Close()
        return $false
    } catch {
        return $true
    }
}

Write-Host "Checking lock status over 10 seconds..."
for ($i = 0; $i -lt 20; $i++) {
    $l1 = Is-FileLocked $img1Path
    $l2 = Is-FileLocked $img2Path
    Write-Host "  -> img1 locked: $l1, img2 locked: $l2"
    Start-Sleep -Milliseconds 500
}

Stop-EliteProcesses
