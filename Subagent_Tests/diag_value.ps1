Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
public class Win32Helper {
    [DllImport("user32.dll")]
    public static extern IntPtr GetMenu(IntPtr hWnd);
    [DllImport("user32.dll")]
    public static extern IntPtr GetSubMenu(IntPtr hMenu, int nPos);
    [DllImport("user32.dll")]
    public static extern uint GetMenuState(IntPtr hMenu, uint uId, uint uFlags);
    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);
    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern int GetClassName(IntPtr hWnd, System.Text.StringBuilder lpClassName, int nMaxCount);
    [DllImport("user32.dll")]
    public static extern IntPtr SendMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

    public static IntPtr FindWindowGlobal(string className) {
        IntPtr found = IntPtr.Zero;
        EnumWindows((hWnd, lParam) => {
            System.Text.StringBuilder sbClass = new System.Text.StringBuilder(260);
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

Write-Host "Starting Win32Explorer..."
$proc = Start-Process -FilePath "C:\Users\Administrator\Desktop\Elite-TaskBar\Win32Explorer_26.0.3.0\Win32Explorer.exe" -ArgumentList "C:\Windows" -PassThru
Start-Sleep -Seconds 7

$hwndMain = [Win32Helper]::FindWindowGlobal("Win32Explorer")
Write-Host "HWND: $hwndMain"

if ($hwndMain -ne [IntPtr]::Zero) {
    # Get the Main Menu
    $hMenu = [Win32Helper]::GetMenu($hwndMain)
    Write-Host "hMenu: $hMenu"

    # View sub-menus
    $hSub = [Win32Helper]::GetSubMenu($hMenu, 3) # View is index 3
    if ($hSub -ne [IntPtr]::Zero) {
        $ids = @(60000, 60001, 60002, 60003, 60004, 60005, 60006, 60007, 60008, 60009, 60017, 60018)
        foreach ($id in $ids) {
            $state = [Win32Helper]::GetMenuState($hSub, $id, 0)
            $checked = (($state -band 0x8) -eq 0x8)
            $exists = ($state -ne 0xFFFFFFFF)
            Write-Host "ID: $id, Exists: $exists, State: $state, Checked: $checked"
        }
    } else {
        Write-Host "Could not get submenu index 3."
    }
}

# Close it
[Win32Helper]::SendMessage($hwndMain, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
