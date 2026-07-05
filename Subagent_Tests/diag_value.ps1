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

    public static IntPtr FindProcessWindow(int processId, string className) {
        IntPtr found = IntPtr.Zero;
        EnumWindows((hWnd, lParam) => {
            uint pid;
            GetWindowThreadProcessId(hWnd, out pid);
            if (pid == processId) {
                System.Text.StringBuilder sbClass = new System.Text.StringBuilder(260);
                GetClassName(hWnd, sbClass, sbClass.Capacity);
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

$proc = Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue
if (!$proc) {
    Write-Host "Win32Explorer is not running. Launching it..."
    $proc = Start-Process -FilePath "C:\Users\Administrator\Desktop\Elite-TaskBar\Win32Explorer_26.0.3.0\Win32Explorer.exe" -ArgumentList "C:\Windows" -PassThru
    Start-Sleep -Seconds 5
} else {
    $proc = $proc[0]
}

$hwndMain = [Win32Helper]::FindProcessWindow($proc.Id, "Win32Explorer")
Write-Host "HWND: $hwndMain"

# Get the Main Menu
$hMenu = [Win32Helper]::GetMenu($hwndMain)
Write-Host "hMenu: $hMenu"

# View Menu is usually at index 1 (File=0, Edit=1, View=2 or similar)
# Let's loop and find which submenu contains the View modes
for ($i = 0; $i -lt 10; $i++) {
    $hSub = [Win32Helper]::GetSubMenu($hMenu, $i)
    if ($hSub -ne [IntPtr]::Zero) {
        $stateSmallIconTiles = [Win32Helper]::GetMenuState($hSub, 60018, 0)
        $stateTiles = [Win32Helper]::GetMenuState($hSub, 60016, 0)
        $stateIcons = [Win32Helper]::GetMenuState($hSub, 60003, 0)
        $stateDetails = [Win32Helper]::GetMenuState($hSub, 60006, 0)
        
        if ($stateSmallIconTiles -ne 0xFFFFFFFF) {
            Write-Host "Submenu index $i is the View/Program menu!"
            Write-Host "  SmallIconTiles (60018) state: $stateSmallIconTiles (MF_CHECKED is 0x8)"
            Write-Host "  Tiles (60016) state: $stateTiles"
            Write-Host "  Icons (60003) state: $stateIcons"
            Write-Host "  Details (60006) state: $stateDetails"
        }
    }
}
