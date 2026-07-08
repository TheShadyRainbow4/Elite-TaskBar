# [Inspect Z-order below Progman] - Builder-Bob
Get-Process -Name Win32Explorer, EliteTaskbar -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Process -FilePath "Win32Explorer.exe"
Start-Sleep -Seconds 5
$code = @"
using System;
using System.Text;
using System.Runtime.InteropServices;

public class ZDiag2 {
    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    
    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern int GetClassName(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);
    
    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern int GetWindowTextW(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    
    [DllImport("user32.dll")]
    public static extern bool IsWindowVisible(IntPtr hWnd);
    
    [DllImport("user32.dll")]
    public static extern IntPtr GetWindow(IntPtr hWnd, uint uCmd);
    
    public static void Show() {
        IntPtr hwndDesktop = IntPtr.Zero;
        EnumWindows((hWnd, lParam) => {
            StringBuilder sbClass = new StringBuilder(260);
            GetClassName(hWnd, sbClass, sbClass.Capacity);
            if (sbClass.ToString() == "Progman" && IsWindowVisible(hWnd)) {
                hwndDesktop = hWnd;
                return false;
            }
            return true;
        }, IntPtr.Zero);
        
        if (hwndDesktop == IntPtr.Zero) {
            Console.WriteLine("Could not find visible Progman window.");
            return;
        }
        
        Console.WriteLine("Found visible Progman HWND: {0}", hwndDesktop.ToInt64().ToString("X"));
        IntPtr next = GetWindow(hwndDesktop, 2); // GW_HWNDNEXT = 2
        while (next != IntPtr.Zero) {
            StringBuilder sbClass = new StringBuilder(260);
            GetClassName(next, sbClass, sbClass.Capacity);
            string cls = sbClass.ToString();
            
            StringBuilder sbTitle = new StringBuilder(260);
            GetWindowTextW(next, sbTitle, sbTitle.Capacity);
            
            Console.WriteLine("Next HWND: {0}, Class: {1}, Title: {2}, Vis: {3}", 
                next.ToInt64().ToString("X"), cls, sbTitle.ToString(), IsWindowVisible(next));
            next = GetWindow(next, 2);
        }
    }
}
"@
Add-Type -TypeDefinition $code -ErrorAction SilentlyContinue
[ZDiag2]::Show()
Get-Process -Name Win32Explorer, EliteTaskbar -ErrorAction SilentlyContinue | Stop-Process -Force
