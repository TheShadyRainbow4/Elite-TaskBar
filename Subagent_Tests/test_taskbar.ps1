Add-Type -TypeDefinition @"
using System;
using System.Text;
using System.Runtime.InteropServices;
public class Win32Helper {
    [DllImport("user32.dll")]
    public static extern IntPtr SendMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll")]
    public static extern bool EnumChildWindows(IntPtr hwndParent, EnumChildProc lpEnumFunc, IntPtr lParam);
    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern int GetClassName(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);
    public delegate bool EnumChildProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

    public static IntPtr FindWindowGlobal(string className) {
        IntPtr found = IntPtr.Zero;
        EnumWindows((hWnd, lParam) => {
            StringBuilder sbClass = new StringBuilder(260);
            GetClassName(hWnd, sbClass, sbClass.Capacity);
            if (sbClass.ToString() == className) {
                found = hWnd;
                return false;
            }
            return true;
        }, IntPtr.Zero);
        return found;
    }
}
"@

Stop-Process -Name Win32Explorer, EliteTaskbar -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 3

# Delete the ENTIRE registry key for Win32Explorer
$regRootPath = 'HKCU:\Software\Win32Explorer'
if (Test-Path $regRootPath) {
    Remove-Item $regRootPath -Recurse -Force -ErrorAction SilentlyContinue | Out-Null
}

$regSettingsPath = 'HKCU:\Software\Win32Explorer\Settings'
if (!(Test-Path $regSettingsPath)) { New-Item -Path $regSettingsPath -Force | Out-Null }
Set-ItemProperty -Path $regSettingsPath -Name "EnableEliteTaskbar" -Value 1 -Type DWord
Set-ItemProperty -Path $regSettingsPath -Name "ConfirmCloseTabs" -Value 0 -Type DWord
Set-ItemProperty -Path $regSettingsPath -Name "EnablePortableMirror" -Value 0 -Type DWord

Write-Host "Launching Win32Explorer..."
$proc = Start-Process -FilePath "C:\Users\Administrator\Desktop\Elite-TaskBar\Win32Explorer_26.0.3.0\Win32Explorer.exe" -ArgumentList "C:\Windows" -PassThru
Start-Sleep -Seconds 7

$explProc = Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue
$taskbarProc = Get-Process -Name EliteTaskbar -ErrorAction SilentlyContinue
Write-Host "Before close:"
Write-Host "  Win32Explorer running: $($explProc -ne $null)"
Write-Host "  EliteTaskbar running: $($taskbarProc -ne $null)"

$hwndMain = [Win32Helper]::FindWindowGlobal("Win32Explorer")
Write-Host "HWND: $hwndMain"

if ($hwndMain -ne [IntPtr]::Zero) {
    Write-Host "Closing browser window..."
    [Win32Helper]::SendMessage($hwndMain, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Seconds 6
}

$explProcAfter = Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue
$taskbarProcAfter = Get-Process -Name EliteTaskbar -ErrorAction SilentlyContinue
Write-Host "After close:"
Write-Host "  Win32Explorer running: $($explProcAfter -ne $null)"
Write-Host "  EliteTaskbar running: $($taskbarProcAfter -ne $null)"
