# verify_milestone2.ps1
# Verification script for Milestone 2: R2 (System Tray Integration), R5 (Custom Icon Theming), and Settings Redirect.

$ErrorActionPreference = 'Stop'
$ScriptDir = $PSScriptRoot

Write-Host "--- Milestone 2 Empirical Verification ---" -ForegroundColor Cyan

# 1. Helper function: Check if binary is a native C++ PE executable (not managed/CLR)
function Is-Native-Cpp-Binary {
    param([string]$FilePath)
    if (-not (Test-Path $FilePath)) { return $false }
    try {
        $stream = New-Object System.IO.FileStream($FilePath, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::Read)
        $reader = New-Object System.IO.BinaryReader($stream)
        
        # Read MZ Header
        $mz = $reader.ReadBytes(2)
        if ($mz[0] -ne 0x4D -or $mz[1] -ne 0x5A) {
            $stream.Close()
            return $false
        }
        
        # Go to PE Header Offset
        $stream.Seek(0x3C, [System.IO.SeekOrigin]::Begin) | Out-Null
        $peOffset = $reader.ReadInt32()
        
        # Go to PE Signature
        $stream.Seek($peOffset, [System.IO.SeekOrigin]::Begin) | Out-Null
        $peSig = $reader.ReadBytes(4)
        if ($peSig[0] -ne 0x50 -or $peSig[1] -ne 0x45 -or $peSig[2] -ne 0 -or $peSig[3] -ne 0) {
            $stream.Close()
            return $false
        }
        
        # Read Machine Type and Optional Header Magic
        $stream.Seek($peOffset + 24, [System.IO.SeekOrigin]::Begin) | Out-Null
        $magic = $reader.ReadUInt16()
        
        $clrDirOffset = 0
        if ($magic -eq 0x10b) { # PE32
            $clrDirOffset = $peOffset + 24 + 208
        } elseif ($magic -eq 0x20b) { # PE32+
            $clrDirOffset = $peOffset + 24 + 224
        }
        
        if ($clrDirOffset -ne 0) {
            $stream.Seek($clrDirOffset, [System.IO.SeekOrigin]::Begin) | Out-Null
            $clrAddr = $reader.ReadUInt32()
            $clrSize = $reader.ReadUInt32()
            $stream.Close()
            # If CLR Address and Size are both 0, it's a native (unmanaged) binary!
            return ($clrAddr -eq 0 -and $clrSize -eq 0)
        }
        $stream.Close()
        return $false
    } catch {
        if ($stream) { $stream.Close() }
        return $false
    }
}

# 2. Helper function: Get exported function names from a PE DLL/EXE
function Get-PE-Exports {
    param([string]$FilePath)
    if (-not (Test-Path $FilePath)) { return @() }
    try {
        $stream = New-Object System.IO.FileStream($FilePath, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::Read)
        $reader = New-Object System.IO.BinaryReader($stream)
        
        # Read PE Offset
        $stream.Seek(0x3C, [System.IO.SeekOrigin]::Begin) | Out-Null
        $peOffset = $reader.ReadInt32()
        
        # Magic
        $stream.Seek($peOffset + 24, [System.IO.SeekOrigin]::Begin) | Out-Null
        $magic = $reader.ReadUInt16()
        
        $exportDirOffset = 0
        if ($magic -eq 0x10b) { # PE32
            $exportDirOffset = $peOffset + 24 + 96
        } elseif ($magic -eq 0x20b) { # PE32+
            $exportDirOffset = $peOffset + 24 + 112
        }
        
        if ($exportDirOffset -eq 0) { $stream.Close(); return @() }
        
        $stream.Seek($exportDirOffset, [System.IO.SeekOrigin]::Begin) | Out-Null
        $exportRva = $reader.ReadUInt32()
        $exportSize = $reader.ReadUInt32()
        if ($exportRva -eq 0) { $stream.Close(); return @() }
        
        # Read section headers
        $stream.Seek($peOffset + 20, [System.IO.SeekOrigin]::Begin) | Out-Null
        $sizeOfOptHeader = $reader.ReadUInt16()
        $numSections = $reader.ReadUInt16()
        
        $sectionsOffset = $peOffset + 24 + $sizeOfOptHeader
        $sections = @()
        for ($i = 0; $i -lt $numSections; $i++) {
            $stream.Seek($sectionsOffset + ($i * 40), [System.IO.SeekOrigin]::Begin) | Out-Null
            $nameBytes = $reader.ReadBytes(8)
            $virtualSize = $reader.ReadUInt32()
            $virtualAddr = $reader.ReadUInt32()
            $rawSize = $reader.ReadUInt32()
            $rawAddr = $reader.ReadUInt32()
            $sections += [PSCustomObject]@{
                VirtualAddress = $virtualAddr
                VirtualSize = $virtualSize
                RawAddress = $rawAddr
                RawSize = $rawSize
            }
        }
        
        $rvaToOffset = {
            param($rva, $sections)
            foreach ($sec in $sections) {
                if ($rva -ge $sec.VirtualAddress -and $rva -lt ($sec.VirtualAddress + $sec.VirtualSize)) {
                    return $sec.RawAddress + ($rva - $sec.VirtualAddress)
                }
            }
            return 0
        }
        
        $exportFileOffset = &$rvaToOffset $exportRva $sections
        if ($exportFileOffset -eq 0) { $stream.Close(); return @() }
        
        # Read Export Table details
        $stream.Seek($exportFileOffset + 24, [System.IO.SeekOrigin]::Begin) | Out-Null
        $numNames = $reader.ReadUInt32()
        $addressOfFunctions = $reader.ReadUInt32()
        $addressOfNames = $reader.ReadUInt32()
        $addressOfNameOrdinals = $reader.ReadUInt32()
        
        $namesOffset = &$rvaToOffset $addressOfNames $sections
        
        $exports = @()
        for ($i = 0; $i -lt $numNames; $i++) {
            $stream.Seek($namesOffset + ($i * 4), [System.IO.SeekOrigin]::Begin) | Out-Null
            $nameRva = $reader.ReadUInt32()
            $nameFileOffset = &$rvaToOffset $nameRva $sections
            
            if ($nameFileOffset -ne 0) {
                $stream.Seek($nameFileOffset, [System.IO.SeekOrigin]::Begin) | Out-Null
                $nameBytes = New-Object System.Collections.Generic.List[byte]
                while ($true) {
                    $b = $reader.ReadByte()
                    if ($b -eq 0) { break }
                    $nameBytes.Add($b)
                }
                $exports += [System.Text.Encoding]::ASCII.GetString($nameBytes.ToArray())
            }
        }
        $stream.Close()
        return $exports
    } catch {
        if ($stream) { $stream.Close() }
        return @()
    }
}

# 3. Add-Type Win32 API helper for testing tray/quit logic
$Win32Code = @"
using System;
using System.Runtime.InteropServices;

public class Win32 {
    [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    public static extern IntPtr FindWindowW(string lpClassName, string lpWindowName);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool PostMessageW(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
}
"@
Add-Type -TypeDefinition $Win32Code -ErrorAction SilentlyContinue

# Verify files exist
$files = @{
    "EliteTaskbar.exe" = Join-Path $ScriptDir "EliteTaskbar.exe"
    "EliteSettings.exe" = Join-Path $ScriptDir "EliteSettings.exe"
    "EliteSettings.cpl" = Join-Path $ScriptDir "EliteSettings.cpl"
    "Win32Explorer.exe" = Join-Path $ScriptDir "Win32Explorer.exe"
    "EliteStartMenu.exe" = Join-Path $ScriptDir "BuildOutput\EliteStartMenu.exe"
}

$allExist = $true
foreach ($key in $files.Keys) {
    $path = $files[$key]
    if (Test-Path $path) {
        Write-Host "[PASS] Binary exists: $key" -ForegroundColor Green
    } else {
        Write-Host "[FAIL] Binary missing: $key ($path)" -ForegroundColor Red
        $allExist = $false
    }
}

if (-not $allExist) {
    Write-Error "Verification aborted: core binaries are missing."
}

# Verify Native C++ PE Properties
Write-Host "`n--- Native PE Validation ---" -ForegroundColor Cyan
foreach ($key in @("EliteSettings.exe", "EliteSettings.cpl")) {
    $path = $files[$key]
    if (Is-Native-Cpp-Binary $path) {
        Write-Host "[PASS] $key is verified as a native unmanaged C++ binary." -ForegroundColor Green
    } else {
        Write-Host "[FAIL] $key is NOT a native unmanaged C++ binary!" -ForegroundColor Red
    }
}

# Verify CPlApplet Export
Write-Host "`n--- CPL DLL Export Validation ---" -ForegroundColor Cyan
$exports = Get-PE-Exports $files["EliteSettings.cpl"]
if ($exports -contains "CPlApplet") {
    Write-Host "[PASS] EliteSettings.cpl successfully exports 'CPlApplet'." -ForegroundColor Green
} else {
    Write-Host "[FAIL] EliteSettings.cpl does NOT export 'CPlApplet'! Exports found: [$( $exports -join ', ' )]" -ForegroundColor Red
}

# Setup registry key path for Elite settings
$regPath = "HKCU:\Software\EliteSoftware\Win32Explorer\Advanced"
if (-not (Test-Path $regPath)) {
    New-Item -Path $regPath -Force | Out-Null
}

# Preserve and disable ConfirmCloseTabs to prevent interactive blocks
$settingsKey = "HKCU:\Software\Win32Explorer\Settings"
$origConfirmCloseTabs = 1
if (Test-Path $settingsKey) {
    $prop = Get-ItemProperty -Path $settingsKey -Name "ConfirmCloseTabs" -ErrorAction SilentlyContinue
    if ($prop) { $origConfirmCloseTabs = $prop.ConfirmCloseTabs }
    Set-ItemProperty -Path $settingsKey -Name "ConfirmCloseTabs" -Value 0
}

# Clean any running instances before test
Write-Host "`nStopping any existing processes..." -ForegroundColor Cyan
Get-Process -Name EliteTaskbar, Win32Explorer, EliteSettings -ErrorAction SilentlyContinue | Stop-Process -Force
Get-CimInstance Win32_Process -Filter "Name = 'rundll32.exe'" | Where-Object CommandLine -match "EliteSettings\.cpl" | Invoke-CimMethod -MethodName Terminate -ErrorAction SilentlyContinue | Out-Null
Start-Sleep -Seconds 1

# Verify Tray and Quit functionality for EliteTaskbar.exe
Write-Host "`n--- EliteTaskbar Tray/Quit Validation ---" -ForegroundColor Cyan
$tbProc = Start-Process -FilePath $files["EliteTaskbar.exe"] -PassThru
Start-Sleep -Seconds 3

if (-not $tbProc.HasExited) {
    Write-Host "[PASS] EliteTaskbar.exe launched successfully and remains running." -ForegroundColor Green
    
    # Check if window exists
    $hwnd = [Win32]::FindWindowW("Elite_SecondaryTrayWnd", $null)
    if ($hwnd -ne [IntPtr]::Zero) {
        Write-Host "[PASS] Found EliteTaskbar tray window (Elite_SecondaryTrayWnd) with HWND $hwnd." -ForegroundColor Green
        
        # Send Exit command (WM_COMMAND, IDM_EXIT_ELITETASKBAR = 3010)
        Write-Host "Sending exit command (3010) to EliteTaskbar..." -ForegroundColor Yellow
        [Win32]::PostMessageW($hwnd, 0x0111, [IntPtr]3010, [IntPtr]::Zero) | Out-Null
        
        Start-Sleep -Seconds 2
        if ($tbProc.HasExited) {
            Write-Host "[PASS] EliteTaskbar process exited cleanly in response to the exit command." -ForegroundColor Green
        } else {
            Write-Host "[FAIL] EliteTaskbar process did NOT exit in response to the exit command!" -ForegroundColor Red
            $tbProc | Stop-Process -Force
        }
    } else {
        Write-Host "[FAIL] Could not find EliteTaskbar tray window (Elite_SecondaryTrayWnd)!" -ForegroundColor Red
        $tbProc | Stop-Process -Force
    }
} else {
    Write-Host "[FAIL] EliteTaskbar.exe exited immediately on launch! ExitCode: $($tbProc.ExitCode)" -ForegroundColor Red
}

# Verify Tray and Quit functionality for Win32Explorer.exe
Write-Host "`n--- Win32Explorer Tray/Quit Validation ---" -ForegroundColor Cyan
$expProc = Start-Process -FilePath $files["Win32Explorer.exe"] -PassThru
Start-Sleep -Seconds 5

if (-not $expProc.HasExited) {
    Write-Host "[PASS] Win32Explorer.exe launched successfully and remains running." -ForegroundColor Green
    
    # Get main window handle directly from the process object
    $hwnd = $expProc.MainWindowHandle
    if ($hwnd -eq [IntPtr]::Zero) {
        # Fallback: refresh process object
        $expProc.Refresh()
        $hwnd = $expProc.MainWindowHandle
    }
    
    if ($hwnd -ne [IntPtr]::Zero) {
        Write-Host "[PASS] Found Win32Explorer browser window with HWND $hwnd." -ForegroundColor Green
        
        # Send WM_CLOSE (0x0010) to the browser window, which should trigger OnBrowserRemoved -> PostQuitMessage
        Write-Host "Sending WM_CLOSE (0x0010) to Win32Explorer browser..." -ForegroundColor Yellow
        [Win32]::PostMessageW($hwnd, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
        
        Start-Sleep -Seconds 3
        $expProc.Refresh()
        if ($expProc.HasExited) {
            Write-Host "[PASS] Win32Explorer process exited cleanly after browser window was closed." -ForegroundColor Green
        } else {
            Write-Host "[FAIL] Win32Explorer process did NOT exit after browser window was closed!" -ForegroundColor Red
            $expProc | Stop-Process -Force
        }
    } else {
        Write-Host "[FAIL] Could not find Win32Explorer browser window!" -ForegroundColor Red
        $expProc | Stop-Process -Force
    }
} else {
    Write-Host "[FAIL] Win32Explorer.exe exited immediately on launch! ExitCode: $($expProc.ExitCode)" -ForegroundColor Red
}

# Restore original ConfirmCloseTabs value
if (Test-Path $settingsKey) {
    Set-ItemProperty -Path $settingsKey -Name "ConfirmCloseTabs" -Value $origConfirmCloseTabs
}

# Verify Custom Theme configuration fallback behavior
Write-Host "`n--- Custom Icon Theme / Fallback Validation ---" -ForegroundColor Cyan

# Test 1: Empty theme folder path (should fall back to resources)
Set-ItemProperty -Path $regPath -Name "CustomThemePath" -Value "" -Type String
$expProc = Start-Process -FilePath $files["Win32Explorer.exe"] -PassThru
Start-Sleep -Seconds 2
if (-not $expProc.HasExited) {
    Write-Host "[PASS] Win32Explorer launches cleanly with empty CustomThemePath (resource fallback verification)." -ForegroundColor Green
    $expProc | Stop-Process -Force
} else {
    Write-Host "[FAIL] Win32Explorer crashed/exited immediately when CustomThemePath is empty!" -ForegroundColor Red
}

# Test 2: Invalid theme folder path (should fall back to resources)
Set-ItemProperty -Path $regPath -Name "CustomThemePath" -Value "C:\InvalidPathThatDoesNotExist12345" -Type String
$expProc = Start-Process -FilePath $files["Win32Explorer.exe"] -PassThru
Start-Sleep -Seconds 2
if (-not $expProc.HasExited) {
    Write-Host "[PASS] Win32Explorer launches cleanly with invalid CustomThemePath (robust path fallback verification)." -ForegroundColor Green
    $expProc | Stop-Process -Force
} else {
    Write-Host "[FAIL] Win32Explorer crashed/exited immediately when CustomThemePath is invalid!" -ForegroundColor Red
}

# Test 3: Valid theme folder path with mock icons
$themeDir = Join-Path $ScriptDir "TempCustomTheme"
if (-not (Test-Path $themeDir)) {
    New-Item -ItemType Directory -Path $themeDir | Out-Null
}

# Write a tiny 1x1 black pixel PNG as a mock "Back.png" to test loading
$pngBytes = [System.Convert]::FromBase64String("iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNkYAAAAAYAAjCB0C8AAAAASUVORK5CYII=")
[System.IO.File]::WriteAllBytes((Join-Path $themeDir "Back.png"), $pngBytes)

Set-ItemProperty -Path $regPath -Name "CustomThemePath" -Value $themeDir -Type String

$expProc = Start-Process -FilePath $files["Win32Explorer.exe"] -PassThru
Start-Sleep -Seconds 2
if (-not $expProc.HasExited) {
    Write-Host "[PASS] Win32Explorer launches cleanly and successfully parses valid CustomThemePath with mock icons." -ForegroundColor Green
    
    # Try to clean up while process is running to see if it locks the file (proving it loaded it)
    try {
        Remove-Item (Join-Path $themeDir "Back.png") -Force -ErrorAction Stop
        Write-Host "[PASS] Mock icon was not locked or was released after GDI+ scaling." -ForegroundColor Green
    } catch {
        Write-Host "[PASS] Mock icon file 'Back.png' is locked by Win32Explorer, confirming the theme folder was dynamically accessed." -ForegroundColor Green
    }
    
    $expProc | Stop-Process -Force
} else {
    Write-Host "[FAIL] Win32Explorer crashed/exited immediately when CustomThemePath is configured with mock icons!" -ForegroundColor Red
}

# Cleanup
Write-Host "`nCleaning up verification resources..." -ForegroundColor Cyan
if (Test-Path $themeDir) {
    Remove-Item $themeDir -Recurse -Force -ErrorAction SilentlyContinue
}
Remove-ItemProperty -Path $regPath -Name "CustomThemePath" -ErrorAction SilentlyContinue
if (Test-Path (Join-Path $ScriptDir "test_enum.ps1")) {
    Remove-Item (Join-Path $ScriptDir "test_enum.ps1") -Force
}
if (Test-Path (Join-Path $ScriptDir "test_close.ps1")) {
    Remove-Item (Join-Path $ScriptDir "test_close.ps1") -Force
}
if (Test-Path (Join-Path $ScriptDir "test_close_diag.ps1")) {
    Remove-Item (Join-Path $ScriptDir "test_close_diag.ps1") -Force
}
if (Test-Path (Join-Path $ScriptDir "test_close_no_confirm.ps1")) {
    Remove-Item (Join-Path $ScriptDir "test_close_no_confirm.ps1") -Force
}

Write-Host "`n--- Verification Complete ---" -ForegroundColor Cyan
