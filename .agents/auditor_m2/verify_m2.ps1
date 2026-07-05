# verify_m2.ps1
# Forensic audit verification script for Milestone 2

$ErrorActionPreference = 'Stop'
$TaskbarDir = "C:\Users\Administrator\Desktop\Elite-TaskBar"

Write-Host "=== Forensic Auditor: Milestone 2 Verification ===" -ForegroundColor Cyan

# 1. Binary Existence check
Write-Host "`n[Check 1] Verifying compiled binary paths..." -ForegroundColor Yellow
$Binaries = @(
    "BuildOutput\EliteSettings.exe",
    "BuildOutput\EliteSettings.cpl",
    "BuildOutputx86\EliteSettings_x86.exe",
    "BuildOutputx86\EliteSettings_x86.cpl",
    "BuildOutput\EliteTaskbar.exe",
    "Win32Explorer.exe"
)

$allExist = $true
foreach ($bin in $Binaries) {
    $fullPath = Join-Path $TaskbarDir $bin
    if (Test-Path $fullPath) {
        $size = (Get-Item $fullPath).Length
        Write-Host "  [PASS] Found: $bin ($size bytes)" -ForegroundColor Green
    } else {
        Write-Host "  [FAIL] Missing: $bin" -ForegroundColor Red
        $allExist = $false
    }
}

# 2. Native C++ Verification (No PS2EXE/PowerShell wrappers)
Write-Host "`n[Check 2] Inspecting EliteSettings.exe / EliteSettings.cpl for PowerShell/PS2EXE dependencies..." -ForegroundColor Yellow

$settingsExe = Join-Path $TaskbarDir "BuildOutput\EliteSettings.exe"
$settingsCpl = Join-Path $TaskbarDir "BuildOutput\EliteSettings.cpl"

# Scan the binaries for signature text "PS2EXE" or "powershell"
if (Test-Path $settingsExe) {
    $exeContent = [System.IO.File]::ReadAllBytes($settingsExe)
    # Convert first 500000 bytes to string to inspect (or search raw bytes)
    $exeString = [System.Text.Encoding]::ASCII.GetString($exeContent, 0, [Math]::Min($exeContent.Length, 100000))
    if ($exeString -match "PS2EXE" -or $exeString -match "powershell\.exe") {
        Write-Host "  [FAIL] EliteSettings.exe contains references to PS2EXE or PowerShell!" -ForegroundColor Red
    } else {
        Write-Host "  [PASS] EliteSettings.exe has no PS2EXE or PowerShell strings in header area." -ForegroundColor Green
    }
}

if (Test-Path $settingsCpl) {
    $cplContent = [System.IO.File]::ReadAllBytes($settingsCpl)
    $cplString = [System.Text.Encoding]::ASCII.GetString($cplContent, 0, [Math]::Min($cplContent.Length, 100000))
    if ($cplString -match "PS2EXE" -or $cplString -match "powershell\.exe") {
        Write-Host "  [FAIL] EliteSettings.cpl contains references to PS2EXE or PowerShell!" -ForegroundColor Red
    } else {
        Write-Host "  [PASS] EliteSettings.cpl has no PS2EXE or PowerShell strings in header area." -ForegroundColor Green
    }
}

# Verify source files
Write-Host "`n[Check 3] Analyzing EliteSettingsStub.cpp and TaskbarProperties.cpp for powershell/ps1 scripts..." -ForegroundColor Yellow
$stubSrc = Join-Path $TaskbarDir "SourceFiles\EliteSettingsStub.cpp"
$propsSrc = Join-Path $TaskbarDir "SourceFiles\TaskbarProperties.cpp"

if (Test-Path $stubSrc) {
    $stubContent = Get-Content $stubSrc -Raw
    if ($stubContent -match "\.ps1" -or $stubContent -match "powershell") {
        Write-Host "  [FAIL] EliteSettingsStub.cpp references .ps1 or powershell." -ForegroundColor Red
    } else {
        Write-Host "  [PASS] EliteSettingsStub.cpp is clean." -ForegroundColor Green
    }
}

if (Test-Path $propsSrc) {
    $propsContent = Get-Content $propsSrc -Raw
    if ($propsContent -match "\.ps1" -or $propsContent -match "powershell") {
        Write-Host "  [FAIL] TaskbarProperties.cpp references .ps1 or powershell." -ForegroundColor Red
    } else {
        Write-Host "  [PASS] TaskbarProperties.cpp is clean." -ForegroundColor Green
    }
}

# 3. System Tray integration verification
Write-Host "`n[Check 4] Checking System Tray Integration in TaskbarWindow.cpp..." -ForegroundColor Yellow
$taskbarSrc = Join-Path $TaskbarDir "SourceFiles\TaskbarWindow.cpp"
if (Test-Path $taskbarSrc) {
    $tbContent = Get-Content $taskbarSrc -Raw
    
    # Verify TrayNotifyProc
    if ($tbContent -match "TrayNotifyProc") {
        Write-Host "  [PASS] Found TrayNotifyProc registration." -ForegroundColor Green
    } else {
        Write-Host "  [FAIL] TrayNotifyProc registration not found." -ForegroundColor Red
    }

    # Verify WM_COPYDATA signature
    if ($tbContent -match "0x34753423") {
        Write-Host "  [PASS] Found WM_COPYDATA signature 0x34753423 for tray notifications." -ForegroundColor Green
    } else {
        Write-Host "  [FAIL] WM_COPYDATA signature 0x34753423 not found." -ForegroundColor Red
    }

    # Verify Overflow modes (Vista Inline vs Win7 Flyout)
    if ($tbContent -match "TrayOverflowMode::Win7Flyout" -and $tbContent -match "TrayExpanded") {
        Write-Host "  [PASS] Found overflow modes Vista Inline and Win7 Flyout logic." -ForegroundColor Green
    } else {
        Write-Host "  [FAIL] Overflow modes not implemented correctly." -ForegroundColor Red
    }
}

# 4. Custom Icon Theming verification
Write-Host "`n[Check 5] Checking Custom Icon Theming in Win32ResourceLoader.cpp..." -ForegroundColor Yellow
$loaderSrc = Join-Path $TaskbarDir "Remaining_Shell\Win32Explorer_26.0.3.0\App_Source\Win32ResourceLoader.cpp"
if (Test-Path $loaderSrc) {
    $loaderContent = Get-Content $loaderSrc -Raw
    if ($loaderContent -match "CustomThemePath" -and $loaderContent -match "GetIconName") {
        Write-Host "  [PASS] Found CustomThemePath and GetIconName custom icon loader logic." -ForegroundColor Green
    } else {
        Write-Host "  [FAIL] CustomThemePath or GetIconName not found in Win32ResourceLoader.cpp." -ForegroundColor Red
    }
}

Write-Host "`n=== Forensic Audit Verification Done ===" -ForegroundColor Cyan
