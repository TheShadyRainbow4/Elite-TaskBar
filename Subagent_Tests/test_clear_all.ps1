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

Stop-Process -Name Win32Explorer -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 3

# Delete the ENTIRE registry key for Win32Explorer
$regRootPath = 'HKCU:\Software\Win32Explorer'
if (Test-Path $regRootPath) {
    Remove-Item $regRootPath -Recurse -Force -ErrorAction SilentlyContinue | Out-Null
    Write-Host "Cleared entire HKCU:\Software\Win32Explorer root registry key."
}

# Now write the Settings key and our default values
$regSettingsPath = 'HKCU:\Software\Win32Explorer\Settings'
if (!(Test-Path $regSettingsPath)) { New-Item -Path $regSettingsPath -Force | Out-Null }
Set-ItemProperty -Path $regSettingsPath -Name "ViewModeGlobal" -Value 12 -Type DWord
Set-ItemProperty -Path $regSettingsPath -Name "ShowInGroupsGlobal" -Value 0 -Type DWord
Set-ItemProperty -Path $regSettingsPath -Name "ConfirmCloseTabs" -Value 0 -Type DWord
Set-ItemProperty -Path $regSettingsPath -Name "EnableShellBagsSupport" -Value 0 -Type DWord
Set-ItemProperty -Path $regSettingsPath -Name "EnablePortableMirror" -Value 0 -Type DWord
Set-ItemProperty -Path $regSettingsPath -Name "EnableEliteTaskbar" -Value 0 -Type DWord

Write-Host "Launching fresh Win32Explorer instance..."
$proc = Start-Process -FilePath "C:\Users\Administrator\Desktop\Elite-TaskBar\Win32Explorer_26.0.3.0\Win32Explorer.exe" -ArgumentList "C:\Windows" -PassThru
Start-Sleep -Seconds 7

Write-Host "Process HasExited: $($proc.HasExited)"
if ($proc.HasExited) {
    Write-Host "ExitCode: $($proc.ExitCode)"
}

$hwndMain = [Win32Helper]::FindWindowGlobal("Win32Explorer")
Write-Host "HWND: $hwndMain"

if ($hwndMain -ne [IntPtr]::Zero) {
    $hwndLV = [Win32Helper]::FindChildByClass($hwndMain, "SysListView32")
    Write-Host "ListView HWND: $hwndLV"

    if ($hwndLV -ne [IntPtr]::Zero) {
        # Query ListView view mode using LVM_GETVIEW (0x108F)
        $view = [Win32Helper]::SendMessage($hwndLV, 0x108F, [IntPtr]::Zero, [IntPtr]::Zero)
        Write-Host "ListView View Mode (LVM_GETVIEW): $view (4 is expected for SmallIconTiles/Tile)"
    }
}

# Close the window
[Win32Helper]::SendMessage($hwndMain, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
