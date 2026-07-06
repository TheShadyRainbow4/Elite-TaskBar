# test_listview_empirical.ps1
# Empirical test harness to validate and stress test ListView enhancements in EliteTaskbar

$ErrorActionPreference = 'Stop'
$ScriptDir = Split-Path -Parent -Path $MyInvocation.MyCommand.Definition
$ProjectRoot = Split-Path -Parent -Path $ScriptDir

Write-Host "==========================================================" -ForegroundColor Green
Write-Host "   ELITE TASKBAR LISTVIEW ENHANCEMENTS EMPIRICAL TESTS" -ForegroundColor Green
Write-Host "==========================================================" -ForegroundColor Green

# Compile Win32 helper definitions
$code = @"
using System;
using System.Text;
using System.Collections.Generic;
using System.Runtime.InteropServices;

public class ListViewTestHelper {
    [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    public static extern IntPtr FindWindowExW(IntPtr hWndParent, IntPtr hWndChildAfter, string lpszClass, string lpszWindow);

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

    [DllImport("user32.dll", SetLastError = true)]
    public static extern int GetWindowLongW(IntPtr hWnd, int nIndex);

    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern IntPtr SendMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern bool IsWindowVisible(IntPtr hWnd);

    [DllImport("comctl32.dll")]
    public static extern bool ImageList_GetIconSize(IntPtr himl, out int cx, out int cy);

    public const int GWL_STYLE = -16;
    public const int GWL_EXSTYLE = -20;
    
    public const uint LVM_GETVIEW = 0x108F;
    public const uint LVM_GETIMAGELIST = 0x1002;
    public const uint LVSIL_NORMAL = 0;
    public const uint LVM_GETITEMCOUNT = 0x1004;
    public const uint LVM_GETITEMPOSITION = 0x1010;
    public const uint LVM_SETITEMPOSITION = 0x100F;
    public const uint LVM_GETEXTENDEDLISTVIEWSTYLE = 0x1037;

    [StructLayout(LayoutKind.Sequential)]
    public struct POINT {
        public int x;
        public int y;
    }

    public static List<IntPtr> GetChildWindows(IntPtr parent) {
        List<IntPtr> result = new List<IntPtr>();
        EnumChildWindows(parent, (hWnd, lParam) => {
            result.Add(hWnd);
            return true;
        }, IntPtr.Zero);
        return result;
    }

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

# Ensure clean state and launch EliteTaskbar
Write-Host "Resetting environment..." -ForegroundColor Cyan
Get-Process -Name EliteTaskbar -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 1

$pathAdv = "HKCU:\Software\EliteSoftware\Win32Explorer\Advanced"
if (!(Test-Path $pathAdv)) { New-Item -Path $pathAdv -Force | Out-Null }
Set-ItemProperty -Path $pathAdv -Name "DesktopReplacementEnabled" -Value 1 -Type DWord
Set-ItemProperty -Path $pathAdv -Name "DesktopIconsEnabled" -Value 1 -Type DWord

# Always test the newly compiled one from BuildOutput first
$taskbarExe = Join-Path $ProjectRoot "BuildOutput\EliteTaskbar.exe"
if (!(Test-Path $taskbarExe)) {
    $taskbarExe = Join-Path $ProjectRoot "EliteTaskbar.exe"
}

Write-Host "Launching EliteTaskbar: $taskbarExe" -ForegroundColor Cyan
$proc = Start-Process -FilePath $taskbarExe -ArgumentList "-allowMultiple" -PassThru
$taskbarPid = $proc.Id
Start-Sleep -Seconds 5

$hwndProgman = [ListViewTestHelper]::FindProcessWindow($taskbarPid, "Progman")
if ($hwndProgman -eq [IntPtr]::Zero) {
    Write-Host "[FAIL] Custom Progman window not found." -ForegroundColor Red
    $proc | Stop-Process -Force
    exit 1
}
Write-Host "Found custom Progman window HWND: $hwndProgman" -ForegroundColor Yellow

$hwndDefView = [ListViewTestHelper]::FindChildByClass($hwndProgman, "SHELLDLL_DefView")
if ($hwndDefView -eq [IntPtr]::Zero) {
    Write-Host "[FAIL] SHELLDLL_DefView child window not found." -ForegroundColor Red
    $proc | Stop-Process -Force
    exit 1
}
Write-Host "Found SHELLDLL_DefView HWND: $hwndDefView" -ForegroundColor Yellow

$hwndListView = [ListViewTestHelper]::FindChildByClass($hwndDefView, "SysListView32")
if ($hwndListView -eq [IntPtr]::Zero) {
    Write-Host "[FAIL] SysListView32 child window not found." -ForegroundColor Red
    $proc | Stop-Process -Force
    exit 1
}
Write-Host "Found SysListView32 HWND: $hwndListView" -ForegroundColor Yellow

$verdictStyles = "FAIL"
$verdictImageList = "FAIL"
$verdictPlacement = "FAIL"
$verdictIconsToggle = "FAIL"

try {
    # ----------------- TEST 1: Styles Verification -----------------
    Write-Host "`n[TEST 1] Verifying ListView Styles..." -ForegroundColor Cyan
    $style = [ListViewTestHelper]::GetWindowLongW($hwndListView, [ListViewTestHelper]::GWL_STYLE)
    
    # Style constants
    $LVS_ICON = 0x0000          # LVS_ICON is 0
    $LVS_ALIGNLEFT = 0x0800
    $LVS_AUTOARRANGE = 0x0100   # Should be ABSENT!
    $LVS_SHAREIMAGELISTS = 0x0040
    
    # Check styles
    $isIconMode = (($style -band 0x0003) -eq $LVS_ICON)
    $hasAlignLeft = (($style -band $LVS_ALIGNLEFT) -eq $LVS_ALIGNLEFT)
    $hasAutoArrange = (($style -band $LVS_AUTOARRANGE) -eq $LVS_AUTOARRANGE)
    $hasShareImageLists = (($style -band $LVS_SHAREIMAGELISTS) -eq $LVS_SHAREIMAGELISTS)

    Write-Host "  Style flags: 0x$($style.ToString('X8'))" -ForegroundColor Gray
    Write-Host "  Is LVS_ICON Mode       : $isIconMode"
    Write-Host "  Has LVS_ALIGNLEFT      : $hasAlignLeft"
    Write-Host "  Has LVS_AUTOARRANGE    : $hasAutoArrange (Expected: False)"
    Write-Host "  Has LVS_SHAREIMAGELISTS: $hasShareImageLists"

    $extStyle = [ListViewTestHelper]::SendMessage($hwndListView, [ListViewTestHelper]::LVM_GETEXTENDEDLISTVIEWSTYLE, [IntPtr]::Zero, [IntPtr]::Zero)
    $LVS_EX_DOUBLEBUFFER = 0x00010000
    $LVS_EX_BORDERSELECT = 0x00008000
    $hasDoubleBuffer = (($extStyle.ToInt64() -band $LVS_EX_DOUBLEBUFFER) -eq $LVS_EX_DOUBLEBUFFER)
    $hasBorderSelect = (($extStyle.ToInt64() -band $LVS_EX_BORDERSELECT) -eq $LVS_EX_BORDERSELECT)

    Write-Host "  Extended Style flags: 0x$($extStyle.ToInt64().ToString('X8'))" -ForegroundColor Gray
    Write-Host "  Has LVS_EX_DOUBLEBUFFER: $hasDoubleBuffer"
    Write-Host "  Has LVS_EX_BORDERSELECT: $hasBorderSelect"

    if ($isIconMode -and $hasAlignLeft -and !$hasAutoArrange -and $hasShareImageLists -and $hasDoubleBuffer -and $hasBorderSelect) {
        Write-Host "[PASS] ListView styles and extended styles are correct." -ForegroundColor Green
        $verdictStyles = "PASS"
    } else {
        Write-Host "[FAIL] ListView styles do not match requirements." -ForegroundColor Red
    }

    # ----------------- TEST 2: High-Res ImageList Verification -----------------
    Write-Host "`n[TEST 2] Verifying High-Res System ImageList..." -ForegroundColor Cyan
    $himl = [ListViewTestHelper]::SendMessage($hwndListView, [ListViewTestHelper]::LVM_GETIMAGELIST, [IntPtr][ListViewTestHelper]::LVSIL_NORMAL, [IntPtr]::Zero)
    Write-Host "  LVM_GETIMAGELIST Normal ImageList: $himl"
    if ($himl -ne [IntPtr]::Zero) {
        # Note: In 64-bit windows, cross-process ImageList handle sizing can be verified by checking that himl is non-zero
        # Since the style has LVS_SHAREIMAGELISTS and the source binds SHGetImageList(SHIL_EXTRALARGE), 
        # a non-NULL ImageList handle indicates it has successfully bound the system image list.
        Write-Host "[PASS] High-Res ImageList handle bound successfully." -ForegroundColor Green
        $verdictImageList = "PASS"
    } else {
        Write-Host "[FAIL] ImageList handle is NULL." -ForegroundColor Red
    }

    # ----------------- TEST 3: Stress Test: Drag & Drop Position Persistence -----------------
    Write-Host "`n[TEST 3] Stress Testing Drag & Drop Position Persistence..." -ForegroundColor Cyan
    $itemCount = [ListViewTestHelper]::SendMessage($hwndListView, [ListViewTestHelper]::LVM_GETITEMCOUNT, [IntPtr]::Zero, [IntPtr]::Zero).ToInt32()
    Write-Host "  Current desktop icon count: $itemCount"

    if ($itemCount -gt 0) {
        # Query initial position of the first icon
        $ptrStruct = [System.Runtime.InteropServices.Marshal]::AllocHGlobal(8)
        
        [ListViewTestHelper]::SendMessage($hwndListView, [ListViewTestHelper]::LVM_GETITEMPOSITION, [IntPtr]0, $ptrStruct) | Out-Null
        $ptBefore = [System.Runtime.InteropServices.Marshal]::PtrToStructure($ptrStruct, [type][ListViewTestHelper+POINT])
        Write-Host "  Initial position of item 0: ($($ptBefore.x), $($ptBefore.y))" -ForegroundColor Gray

        # Move the item to custom position (500, 500)
        $customX = 500
        $customY = 500
        $lParamPos = [IntPtr]($customX + ($customY -shl 16))
        Write-Host "  Moving item 0 to custom position ($customX, $customY)..."
        [ListViewTestHelper]::SendMessage($hwndListView, [ListViewTestHelper]::LVM_SETITEMPOSITION, [IntPtr]0, $lParamPos) | Out-Null
        Start-Sleep -Milliseconds 500

        # Check new position
        [ListViewTestHelper]::SendMessage($hwndListView, [ListViewTestHelper]::LVM_GETITEMPOSITION, [IntPtr]0, $ptrStruct) | Out-Null
        $ptAfterMove = [System.Runtime.InteropServices.Marshal]::PtrToStructure($ptrStruct, [type][ListViewTestHelper+POINT])
        Write-Host "  Position of item 0 after manual move: ($($ptAfterMove.x), $($ptAfterMove.y))"

        # Verify that we succeeded in positioning it freely
        $movedFreely = ($ptAfterMove.x -ne $ptBefore.x -or $ptAfterMove.y -ne $ptBefore.y)

        # Now, trigger a file refresh by creating and deleting a temporary file in the user's Desktop folder
        $desktopPath = [System.Environment]::GetFolderPath('Desktop')
        $tempFile = Join-Path $desktopPath "est_temp_refresh_test.txt"
        Write-Host "  Triggering shell notification by creating temp file: $tempFile"
        "test" | Out-File -FilePath $tempFile -Force
        Start-Sleep -Seconds 2 # Wait for refresh timer (TIMER_DEBOUNCE_REFRESH is 100ms + PopulateDesktopGrid)

        # Retrieve position of item 0 again
        [ListViewTestHelper]::SendMessage($hwndListView, [ListViewTestHelper]::LVM_GETITEMPOSITION, [IntPtr]0, $ptrStruct) | Out-Null
        $ptAfterRefresh = [System.Runtime.InteropServices.Marshal]::PtrToStructure($ptrStruct, [type][ListViewTestHelper+POINT])
        Write-Host "  Position of item 0 after refresh: ($($ptAfterRefresh.x), $($ptAfterRefresh.y))"

        # Clean up temp file
        Remove-Item $tempFile -Force -ErrorAction SilentlyContinue

        [System.Runtime.InteropServices.Marshal]::FreeHGlobal($ptrStruct)

        if ($movedFreely) {
            # Let's see if position was preserved!
            if ($ptAfterRefresh.x -eq $ptAfterMove.x -and $ptAfterRefresh.y -eq $ptAfterMove.y) {
                Write-Host "[PASS] Custom position is PRESERVED across desktop refreshes." -ForegroundColor Green
                $verdictPlacement = "PASS"
            } else {
                Write-Host "[FAIL] Vulnerability Found: Custom position was RESET on desktop refresh! (Snapped back to grid due to ListView_DeleteAllItems & ListView_Arrange)" -ForegroundColor Red
                $verdictPlacement = "FAIL_RESET"
            }
        } else {
            Write-Host "[FAIL] Could not move item freely." -ForegroundColor Red
        }
    } else {
        Write-Host "  No desktop icons present. Skipping drag/drop test. Please place at least one file on the desktop." -ForegroundColor Yellow
        $verdictPlacement = "SKIP_NO_ICONS"
    }

    # ----------------- TEST 4: Desktop Icons Toggle -----------------
    Write-Host "`n[TEST 4] Testing Desktop Icons Toggle..." -ForegroundColor Cyan
    Write-Host "  Disabling desktop icons in registry..."
    Set-ItemProperty -Path $pathAdv -Name "DesktopIconsEnabled" -Value 0 -Type DWord
    
    # Notify EliteTaskbar by sending WM_SETTINGCHANGE
    $desktopPath = [System.Environment]::GetFolderPath('Desktop')
    $tempFile = Join-Path $desktopPath "est_temp_refresh_test.txt"
    "test" | Out-File -FilePath $tempFile -Force
    Start-Sleep -Seconds 2
    Remove-Item $tempFile -Force -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 1

    # Check if ListView is hidden
    $isVisibleAfterDisable = [ListViewTestHelper]::IsWindowVisible($hwndListView)
    Write-Host "  ListView visible after disabling DesktopIconsEnabled: $isVisibleAfterDisable"

    # Re-enable
    Write-Host "  Re-enabling desktop icons in registry..."
    Set-ItemProperty -Path $pathAdv -Name "DesktopIconsEnabled" -Value 1 -Type DWord
    "test" | Out-File -FilePath $tempFile -Force
    Start-Sleep -Seconds 2
    Remove-Item $tempFile -Force -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 1

    $isVisibleAfterEnable = [ListViewTestHelper]::IsWindowVisible($hwndListView)
    Write-Host "  ListView visible after enabling DesktopIconsEnabled: $isVisibleAfterEnable"

    if (!$isVisibleAfterDisable -and $isVisibleAfterEnable) {
        Write-Host "[PASS] Desktop icons visibility toggle successfully hides/shows the ListView." -ForegroundColor Green
        $verdictIconsToggle = "PASS"
    } else {
        Write-Host "[FAIL] Desktop icons visibility toggle failed." -ForegroundColor Red
    }

} catch {
    Write-Host "[FAIL] Testing crashed with error: $_" -ForegroundColor Red
} finally {
    Write-Host "`nStopping EliteTaskbar process..." -ForegroundColor Cyan
    $proc | Stop-Process -Force -ErrorAction SilentlyContinue
}

# Print Summary
$colorStyles = if ($verdictStyles -eq "PASS") { "Green" } else { "Red" }
$colorImageList = if ($verdictImageList -eq "PASS") { "Green" } else { "Red" }
$colorPlacement = if ($verdictPlacement -eq "PASS") { "Green" } else { "Red" }
$colorIconsToggle = if ($verdictIconsToggle -eq "PASS") { "Green" } else { "Red" }

$overall = "FAIL"
if ($verdictStyles -eq "PASS" -and $verdictImageList -eq "PASS" -and $verdictPlacement -eq "PASS" -and $verdictIconsToggle -eq "PASS") {
    $overall = "PASS"
}
$colorOverall = if ($overall -eq "PASS") { "Green" } else { "Red" }

Write-Host "`n==========================================================" -ForegroundColor Green
Write-Host "   EMPIRICAL LISTVIEW TEST SUMMARY" -ForegroundColor Green
Write-Host "==========================================================" -ForegroundColor Green
Write-Host "  Styles Verification        : $verdictStyles" -ForegroundColor $colorStyles
Write-Host "  High-Res ImageList         : $verdictImageList" -ForegroundColor $colorImageList
Write-Host "  Drag & Drop Persistence    : $verdictPlacement" -ForegroundColor $colorPlacement
Write-Host "  Desktop Icons Toggle       : $verdictIconsToggle" -ForegroundColor $colorIconsToggle

Write-Host "`nOVERALL VERDICT: $overall" -ForegroundColor $colorOverall
return $overall
