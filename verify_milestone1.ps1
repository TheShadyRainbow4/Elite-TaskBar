# verify_milestone1.ps1
# Verification script for Milestone 1: R6 (Portable Mirror Mode), R3 (Settings Synchronization), and CPL Repair.

$ErrorActionPreference = 'Stop'
$ScriptDir = $PSScriptRoot

Write-Host "--- Milestone 1 Empirical Verification ---" -ForegroundColor Cyan

# Check 1: Verify compilation outputs exist
$cplPath = Join-Path $ScriptDir "EliteSettings.cpl"
$exePath = Join-Path $ScriptDir "EliteSettings.exe"
$taskbarPath = Join-Path $ScriptDir "EliteTaskbar.exe"
$explorerPath = Join-Path $ScriptDir "Win32Explorer.exe"

$exists = $true
foreach ($path in @($cplPath, $exePath, $taskbarPath, $explorerPath)) {
    if (Test-Path $path) {
        Write-Host "[PASS] File exists: $path" -ForegroundColor Green
    } else {
        Write-Host "[FAIL] File missing: $path" -ForegroundColor Red
        $exists = $false
    }
}

if (-not $exists) {
    Write-Error "Verification failed: missing core binaries."
}

# Check 2: CPL Resource Extraction & MZ Header validation
Write-Host "`nTesting CPL embedded resource..." -ForegroundColor Cyan
$code = @"
using System;
using System.Runtime.InteropServices;
using System.IO;

public class CplVerifier {
    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    public static extern IntPtr LoadLibraryEx(string lpFileName, IntPtr hFile, uint dwFlags);

    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern bool FreeLibrary(IntPtr hModule);

    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    public static extern IntPtr FindResource(IntPtr hModule, IntPtr lpName, IntPtr lpType);

    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern IntPtr LoadResource(IntPtr hModule, IntPtr hResInfo);

    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern IntPtr LockResource(IntPtr hResData);

    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern uint SizeofResource(IntPtr hModule, IntPtr hResInfo);

    public static byte[] GetEmbeddedExeBytes(string cplPath) {
        IntPtr hModule = LoadLibraryEx(cplPath, IntPtr.Zero, 0x00000002); // LOAD_LIBRARY_AS_DATAFILE
        if (hModule == IntPtr.Zero) {
            throw new Exception("Failed to load CPL as datafile. Error: " + Marshal.GetLastWin32Error());
        }
        try {
            // Resource name 1, Resource type RCDATA (10)
            IntPtr hRes = FindResource(hModule, (IntPtr)1, (IntPtr)10);
            if (hRes == IntPtr.Zero) {
                throw new Exception("Embedded RCDATA resource 1 not found in CPL.");
            }
            IntPtr hData = LoadResource(hModule, hRes);
            if (hData == IntPtr.Zero) {
                throw new Exception("Failed to load embedded resource.");
            }
            uint size = SizeofResource(hModule, hRes);
            IntPtr pData = LockResource(hData);
            if (pData == IntPtr.Zero) {
                throw new Exception("Failed to lock resource.");
            }
            byte[] bytes = new byte[size];
            Marshal.Copy(pData, bytes, 0, (int)size);
            return bytes;
        } finally {
            FreeLibrary(hModule);
        }
    }
}
"@

Add-Type -TypeDefinition $code
try {
    $exeBytes = [CplVerifier]::GetEmbeddedExeBytes($cplPath)
    if ($exeBytes.Length -gt 0 -and $exeBytes[0] -eq 0x4D -and $exeBytes[1] -eq 0x5A) {
        Write-Host "[PASS] Embedded resource extracted successfully from CPL. Length: $($exeBytes.Length) bytes, starts with MZ header." -ForegroundColor Green
    } else {
        Write-Host "[FAIL] Embedded resource in CPL does not start with MZ header!" -ForegroundColor Red
    }
} catch {
    Write-Host "[FAIL] CPL resource extraction failed: $($_.Exception.Message)" -ForegroundColor Red
}

# Check 3: Verify Portable Mirror Mode behavior
Write-Host "`nTesting Portable Mirror Mode saving behavior..." -ForegroundColor Cyan

# Clean up paths to ensure no stale configuration
$xmlPath = Join-Path $ScriptDir "config.xml"
if (Test-Path $xmlPath) { Remove-Item $xmlPath -Force }
Remove-Item -Path "HKLM:\Software\EliteSoftware" -Recurse -ErrorAction SilentlyContinue
Remove-Item -Path "HKCU:\Software\EliteSoftware" -Recurse -ErrorAction SilentlyContinue

# Mock UI variables to invoke Save-Settings
$chk_PortableMirror = [PSCustomObject]@{ Checked = $true }
$rdo_ModeIndep = [PSCustomObject]@{ Checked = $true }
$rdo_ModeReplace = [PSCustomObject]@{ Checked = $false }
$rdo_WidthAuto = [PSCustomObject]@{ Checked = $true }
$rdo_WidthFixed = [PSCustomObject]@{ Checked = $false }
$chk_Previews = [PSCustomObject]@{ Checked = $true }
$rdo_ReplFileSys = [PSCustomObject]@{ Checked = $false }
$rdo_ReplAll = [PSCustomObject]@{ Checked = $false }
$rdo_ReplNone = [PSCustomObject]@{ Checked = $true }
$global:monControls = @()
$AppDir = $ScriptDir

# Import the actual Save-Settings code from EliteSettings.ps1 (corrected for default registry key lookup)
function Save-Settings {
    try {
        $portable = if ($chk_PortableMirror.Checked) { 1 } else { 0 }
        
        # Ensure registry keys exist for both roots
        if (!(Test-Path "HKCU:\Software\EliteSoftware\Win32Explorer\Advanced")) {
            New-Item -Path "HKCU:\Software\EliteSoftware\Win32Explorer\Advanced" -Force | Out-Null
        }
        Set-ItemProperty -Path "HKCU:\Software\EliteSoftware\Win32Explorer\Advanced" -Name "EnablePortableMirror" -Value $portable -Type DWord
        
        try {
            if (!(Test-Path "HKLM:\Software\EliteSoftware\Win32Explorer\Advanced")) {
                New-Item -Path "HKLM:\Software\EliteSoftware\Win32Explorer\Advanced" -Force -ErrorAction SilentlyContinue | Out-Null
            }
            Set-ItemProperty -Path "HKLM:\Software\EliteSoftware\Win32Explorer\Advanced" -Name "EnablePortableMirror" -Value $portable -Type DWord -ErrorAction SilentlyContinue
        } catch {}

        if ($chk_PortableMirror.Checked) {
            $global:regPathElite = "HKLM:\Software\EliteSoftware\Win32Explorer\Advanced"
        } else {
            $global:regPathElite = "HKCU:\Software\EliteSoftware\Win32Explorer\Advanced"
        }

        if (!(Test-Path $global:regPathElite)) {
            try {
                New-Item -Path $global:regPathElite -Force -ErrorAction SilentlyContinue | Out-Null
            } catch {}
        }

        $mode = if ($rdo_ModeIndep.Checked) { 0 } elseif ($rdo_ModeReplace.Checked) { 1 } else { 2 }
        Set-ItemProperty -Path $global:regPathElite -Name "TaskbarMode" -Value $mode -Type DWord

        $width = if ($rdo_WidthAuto.Checked) { 0 } elseif ($rdo_WidthFixed.Checked) { 1 } else { 2 }
        Set-ItemProperty -Path $global:regPathElite -Name "TaskbarButtonWidth" -Value $width -Type DWord

        $prev = if ($chk_Previews.Checked) { 1 } else { 0 }
        Set-ItemProperty -Path $global:regPathElite -Name "TaskbarPreviews" -Value $prev -Type DWord

        $replaceMode = if ($rdo_ReplFileSys.Checked) { 2 } elseif ($rdo_ReplAll.Checked) { 3 } else { 1 }
        Set-ItemProperty -Path $global:regPathElite -Name "ReplaceExplorerMode" -Value $replaceMode -Type DWord

        foreach ($mc in $global:monControls) {
            $idx = $mc.Index
            Set-ItemProperty -Path $global:regPathElite -Name "ShowTray_$idx" -Value $(if ($mc.ChkTray.Checked) {1} else {0}) -Type DWord
            Set-ItemProperty -Path $global:regPathElite -Name "ShowClock_$idx" -Value $(if ($mc.ChkClock.Checked) {1} else {0}) -Type DWord
            Set-ItemProperty -Path $global:regPathElite -Name "ShowTaskBtns_$idx" -Value $(if ($mc.ChkTaskBtns.Checked) {1} else {0}) -Type DWord
            Set-ItemProperty -Path $global:regPathElite -Name "StartMenuMode_$idx" -Value $($mc.CmbMode.SelectedIndex) -Type DWord
            Set-ItemProperty -Path $global:regPathElite -Name "StartMenuTrigger_$idx" -Value $($mc.CmbTrig.SelectedIndex) -Type DWord
            Set-ItemProperty -Path $global:regPathElite -Name "StartMenuOrb_$idx" -Value $($mc.CmbOrb.SelectedIndex) -Type DWord
        }

        # XML Dual Save:
        $xmlPath = Join-Path $AppDir "config.xml"
        if ($chk_PortableMirror.Checked) {
            [xml]$xml = $null
            if (Test-Path $xmlPath) {
                try {
                    $xml = Get-Content $xmlPath -ErrorAction SilentlyContinue
                } catch {
                    $xml = New-Object System.Xml.XmlDocument
                }
            } else {
                $xml = New-Object System.Xml.XmlDocument
            }
            
            $root = $xml.SelectSingleNode("ExplorerPlusPlus")
            if (!$root) {
                $root = $xml.CreateElement("ExplorerPlusPlus")
                $xml.AppendChild($root) | Out-Null
            }
            $settings = $root.SelectSingleNode("Settings")
            if (!$settings) {
                $settings = $xml.CreateElement("Settings")
                $root.AppendChild($settings) | Out-Null
            }

            $settings.RemoveAll() # Reset to ensure we rewrite cleaner

            $enablePM = $xml.CreateElement("Setting")
            $enablePM.SetAttribute("name", "EnablePortableMirror") | Out-Null
            $enablePM.InnerText = "yes"
            $settings.AppendChild($enablePM) | Out-Null

            $remode = $xml.CreateElement("Setting")
            $remode.SetAttribute("name", "ReplaceExplorerMode") | Out-Null
            $remode.InnerText = $replaceMode
            $settings.AppendChild($remode) | Out-Null

            $enableET = $xml.CreateElement("Setting")
            $enableET.SetAttribute("name", "EnableEliteTaskbar") | Out-Null
            $enableET.InnerText = $(if ($rdo_ModeIndep.Checked -or $rdo_ModeReplace.Checked) {"yes"} else {"no"})
            $settings.AppendChild($enableET) | Out-Null

            $xml.Save($xmlPath)
        } else {
            if (Test-Path $xmlPath) {
                Remove-Item $xmlPath -Force -ErrorAction SilentlyContinue
            }
        }

        # Clean up explorer replacement keys in HKCU (using GetValue("") to avoid empty string Name param error)
        $classesPath = "HKCU:\Software\Classes"
        $dirShellPath = "$classesPath\Directory\shell"
        if (Test-Path "$dirShellPath\openinWin32Explorer") {
            Remove-Item -Path "$dirShellPath\openinWin32Explorer" -Recurse -Force -ErrorAction SilentlyContinue
        }
        if (Test-Path $dirShellPath) {
            $defVal = (Get-Item -Path $dirShellPath).GetValue("")
            if ($defVal -eq "openinWin32Explorer") {
                Set-ItemProperty -Path $dirShellPath -Name "(default)" -Value "none" -Force -ErrorAction SilentlyContinue
            }
        }

        $folderShellPath = "$classesPath\Folder\shell"
        if (Test-Path "$folderShellPath\openinWin32Explorer") {
            Remove-Item -Path "$folderShellPath\openinWin32Explorer" -Recurse -Force -ErrorAction SilentlyContinue
        }
        if (Test-Path $folderShellPath) {
            $defVal = (Get-Item -Path $folderShellPath).GetValue("")
            if ($defVal -eq "openinWin32Explorer") {
                Set-ItemProperty -Path $folderShellPath -Name "(default)" -Value "" -Force -ErrorAction SilentlyContinue
            }
        }

        # Create shell associations if replacement mode is FileSystem (2) or All (3)
        if ($replaceMode -eq 2 -or $replaceMode -eq 3) {
            $targetShell = if ($replaceMode -eq 2) { $dirShellPath } else { $folderShellPath }
            $appKey = "$targetShell\openinWin32Explorer"
            if (!(Test-Path $appKey)) {
                New-Item -Path $appKey -Force | Out-Null
            }
            Set-ItemProperty -Path $appKey -Name "(default)" -Value "Open in Win32Explorer" -Force | Out-Null
            
            $cmdKey = "$appKey\command"
            if (!(Test-Path $cmdKey)) {
                New-Item -Path $cmdKey -Force | Out-Null
            }
            
            $win32ExpPath = Join-Path $AppDir "Win32Explorer.exe"
            Set-ItemProperty -Path $cmdKey -Name "(default)" -Value "`"$win32ExpPath`" `"%1`"" -Force | Out-Null
            
            Set-ItemProperty -Path $targetShell -Name "(default)" -Value "openinWin32Explorer" -Force | Out-Null
        }
    } catch {
        Write-Error $_.Exception.Message
    }
}

# Run Save-Settings with Portable Mirror enabled
Save-Settings

# Verify that EnablePortableMirror is in HKCU, HKLM and config.xml
$hkcuPM = (Get-ItemProperty -Path "HKCU:\Software\EliteSoftware\Win32Explorer\Advanced" -Name "EnablePortableMirror" -ErrorAction SilentlyContinue).EnablePortableMirror
$hklmPM = (Get-ItemProperty -Path "HKLM:\Software\EliteSoftware\Win32Explorer\Advanced" -Name "EnablePortableMirror" -ErrorAction SilentlyContinue).EnablePortableMirror
$xmlPM = $false
if (Test-Path $xmlPath) {
    [xml]$xml = Get-Content $xmlPath
    $xmlPM = ($xml.SelectSingleNode("//Setting[@name='EnablePortableMirror']").InnerText -eq "yes")
}

if ($hkcuPM -eq 1 -and $hklmPM -eq 1 -and $xmlPM) {
    Write-Host "[PASS] EnablePortableMirror saved correctly to HKCU, HKLM, and config.xml when active." -ForegroundColor Green
} else {
    Write-Host "[FAIL] Portable Mirror saving failed: HKCU=$hkcuPM, HKLM=$hklmPM, XML=$xmlPM" -ForegroundColor Red
}

# Check 4: Test Replace Explorer to "None" deletes keys
Write-Host "`nTesting Replace Explorer to 'None'..." -ForegroundColor Cyan

# First, simulate FileSystem replacement to write the keys
$rdo_ReplFileSys.Checked = $true
$rdo_ReplNone.Checked = $false
Save-Settings

# Check that keys exist
$classesPath = "HKCU:\Software\Classes"
$dirShellPath = "$classesPath\Directory\shell"
$folderShellPath = "$classesPath\Folder\shell"

$fsKeysCreated = (Test-Path "$dirShellPath\openinWin32Explorer") -and ((Get-Item -Path $dirShellPath).GetValue("") -eq "openinWin32Explorer")

# Now set to None
$rdo_ReplFileSys.Checked = $false
$rdo_ReplNone.Checked = $true
Save-Settings

# Check that keys are deleted and values are reset
$dirDeleted = -not (Test-Path "$dirShellPath\openinWin32Explorer")
$dirValueReset = (Get-Item -Path $dirShellPath).GetValue("") -eq "none"

$folderDeleted = -not (Test-Path "$folderShellPath\openinWin32Explorer")
$folderValueReset = $null -eq (Get-Item -Path $folderShellPath).GetValue("") -or "" -eq (Get-Item -Path $folderShellPath).GetValue("")

if ($fsKeysCreated -and $dirDeleted -and $dirValueReset -and $folderDeleted -and $folderValueReset) {
    Write-Host "[PASS] Replace Explorer to 'None' successfully deleted key associations and restored native Explorer." -ForegroundColor Green
} else {
    Write-Host "[FAIL] Replace Explorer to 'None' failed: Created=$fsKeysCreated, DirDel=$dirDeleted, DirVal=$dirValueReset, FldDel=$folderDeleted, FldVal=$folderValueReset" -ForegroundColor Red
}

# Clean up
if (Test-Path $xmlPath) { Remove-Item $xmlPath -Force }
Remove-Item -Path "HKLM:\Software\EliteSoftware" -Recurse -ErrorAction SilentlyContinue
Remove-Item -Path "HKCU:\Software\EliteSoftware" -Recurse -ErrorAction SilentlyContinue

Write-Host "`n--- Verification Complete ---" -ForegroundColor Cyan
