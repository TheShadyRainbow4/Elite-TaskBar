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
    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount);

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

$regSettingsPath = 'HKCU:\Software\Win32Explorer\Settings'
if (!(Test-Path $regSettingsPath)) { New-Item -Path $regSettingsPath -Force | Out-Null }
Set-ItemProperty -Path $regSettingsPath -Name "ViewModeGlobal" -Value 12 -Type DWord
Set-ItemProperty -Path $regSettingsPath -Name "ShowInGroupsGlobal" -Value 0 -Type DWord
Set-ItemProperty -Path $regSettingsPath -Name "ConfirmCloseTabs" -Value 0 -Type DWord
Set-ItemProperty -Path $regSettingsPath -Name "EnableShellBagsSupport" -Value 0 -Type DWord
Set-ItemProperty -Path $regSettingsPath -Name "EnablePortableMirror" -Value 0 -Type DWord

$proc = Start-Process -FilePath "C:\Users\Administrator\Desktop\Elite-TaskBar\Win32Explorer_26.0.3.0\Win32Explorer.exe" -ArgumentList "C:\Windows" -PassThru
Start-Sleep -Seconds 7

$hwndMain = [Win32Helper]::FindWindowGlobal("Win32Explorer")
Write-Host "HWND: $hwndMain"

if ($hwndMain -ne [IntPtr]::Zero) {
    # Open Options dialog (WM_COMMAND, 40101)
    [Win32Helper]::SendMessage($hwndMain, 0x0111, [IntPtr]40101, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Seconds 2

    $hwndOpt = [Win32Helper]::FindWindowGlobal("#32770")
    Write-Host "Options HWND: $hwndOpt"

    if ($hwndOpt -ne [IntPtr]::Zero) {
        $hwndTree = [Win32Helper]::FindChildByClass($hwndOpt, "SysTreeView32")
        Write-Host "TreeView HWND: $hwndTree"

        # Send Down Arrow key to TreeView to select the second page ("Default Settings")
        [Win32Helper]::SendMessage($hwndTree, 0x0100, [IntPtr]0x28, [IntPtr]::Zero) | Out-Null # WM_KEYDOWN, VK_DOWN
        Start-Sleep -Milliseconds 500

        # Find the ComboBox
        $hwndCombo = [Win32Helper]::FindChildByClass($hwndOpt, "ComboBox")
        Write-Host "ComboBox HWND: $hwndCombo"

        # Query selected index (CB_GETCURSEL = 0x0147)
        $sel = [Win32Helper]::SendMessage($hwndCombo, 0x0147, [IntPtr]::Zero, [IntPtr]::Zero)
        Write-Host "ComboBox Selected Index: $sel"

        # Close Options
        [Win32Helper]::SendMessage($hwndOpt, 0x0111, [IntPtr]2, [IntPtr]::Zero) | Out-Null # Cancel
    }
}

# Close it
[Win32Helper]::SendMessage($hwndMain, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
