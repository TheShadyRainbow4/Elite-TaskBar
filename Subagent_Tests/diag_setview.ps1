Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
public class Win32Helper {
    [DllImport("user32.dll")]
    public static extern IntPtr SendMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll")]
    public static extern bool EnumChildWindows(IntPtr hwndParent, EnumChildProc lpEnumFunc, IntPtr lParam);
    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern int GetClassName(IntPtr hWnd, System.Text.StringBuilder lpClassName, int nMaxCount);
    public delegate bool EnumChildProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

    public static IntPtr FindChildByClass(IntPtr parent, string className) {
        IntPtr found = IntPtr.Zero;
        EnumChildWindows(parent, (hWnd, lParam) => {
            System.Text.StringBuilder sb = new System.Text.StringBuilder(260);
            GetClassName(hWnd, sb, sb.Capacity);
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
                System.Text.StringBuilder sbClass = new System.Text.StringBuilder(260);
                GetClassName(hWnd, sbClass, sbClass.Capacity);
                if (className == null || sbClass.ToString() == className) {
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

$proc = Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue
if (!$proc) {
    Write-Host "Win32Explorer is not running. Launching it..."
    $proc = Start-Process -FilePath "C:\Users\Administrator\Desktop\Elite-TaskBar\Win32Explorer_26.0.3.0\Win32Explorer.exe" -ArgumentList "C:\Windows" -PassThru
    Start-Sleep -Seconds 5
} else {
    $proc = $proc[0]
}

$hwndMain = [Win32Helper]::FindProcessWindow($proc.Id, "Win32Explorer")
$hwndLV = [Win32Helper]::FindChildByClass($hwndMain, "SysListView32")

Write-Host "ListView HWND: $hwndLV"
$viewBefore = [Win32Helper]::SendMessage($hwndLV, 0x108F, [IntPtr]::Zero, [IntPtr]::Zero)
Write-Host "View mode before SetView: $viewBefore"

# Send LVM_SETVIEW with LV_VIEW_TILE (4)
$ret = [Win32Helper]::SendMessage($hwndLV, 0x108E, [IntPtr]4, [IntPtr]::Zero)
Write-Host "LVM_SETVIEW ret: $ret"

$viewAfter = [Win32Helper]::SendMessage($hwndLV, 0x108F, [IntPtr]::Zero, [IntPtr]::Zero)
Write-Host "View mode after SetView: $viewAfter"
