# print_children.ps1
$code = @"
using System;
using System.Text;
using System.Collections.Generic;
using System.Runtime.InteropServices;

public class Win32 {
    [DllImport("user32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool EnumChildWindows(IntPtr hwndParent, EnumWindowsProc lpEnumFunc, IntPtr lParam);

    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetClassNameW(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);

    [DllImport("user32.dll", SetLastError = true, EntryPoint = "GetWindowLongW")]
    public static extern int GetWindowLongW(IntPtr hWnd, int nIndex);

    public const int GWL_ID = -12;

    public static List<IntPtr> GetChildWindows(IntPtr parent) {
        List<IntPtr> result = new List<IntPtr>();
        GCHandle listHandle = GCHandle.Alloc(result);
        try {
            EnumChildWindows(parent, new EnumWindowsProc((hWnd, lParam) => {
                GCHandle gch = GCHandle.FromIntPtr(lParam);
                List<IntPtr> list = gch.Target as List<IntPtr>;
                if (list != null) {
                    list.Add(hWnd);
                }
                return true;
            }), GCHandle.ToIntPtr(listHandle));
        } finally {
            if (listHandle.IsAllocated) {
                listHandle.Free();
            }
        }
        return result;
    }
}
"@

Add-Type -TypeDefinition $code -ErrorAction SilentlyContinue

# Find all Elite_SecondaryTrayWnd windows
$hwndList = New-Object System.Collections.Generic.List[IntPtr]
$enumProc = [Win32+EnumWindowsProc]{
    param($hWnd, $lParam)
    $sb = New-Object System.Text.StringBuilder 260
    [Win32]::GetClassNameW($hWnd, $sb, $sb.Capacity) | Out-Null
    if ($sb.ToString() -eq "Elite_SecondaryTrayWnd") {
        $hwndList.Add($hWnd)
    }
    return $true
}
[Win32]::EnumChildWindows([IntPtr]::Zero, $enumProc, [IntPtr]::Zero) | Out-Null

Write-Host "Found $($hwndList.Count) secondary taskbar windows." -ForegroundColor Green

foreach ($hwnd in $hwndList) {
    Write-Host "Taskbar HWND: $hwnd" -ForegroundColor Yellow
    $children = [Win32]::GetChildWindows($hwnd)
    foreach ($c in $children) {
        $sb = New-Object System.Text.StringBuilder 260
        [Win32]::GetClassNameW($c, $sb, $sb.Capacity) | Out-Null
        $id = [Win32]::GetWindowLongW($c, -12) # GWL_ID = -12
        Write-Host "  Child HWND: $c | Class: $($sb.ToString()) | ID: $id" -ForegroundColor Gray
    }
}
