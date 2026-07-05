param(
    [string]$Configuration = "Release"
)

# Elite-TaskBar Build Script
$ErrorActionPreference = 'Stop'
$ScriptDir = $PSScriptRoot

Write-Host "Checking for running Elite processes..." -ForegroundColor Cyan
Get-Process -Name EliteTaskbar, EliteSettings, EliteEverything, EliteDLLScanner, Win32Explorer -ErrorAction SilentlyContinue | Stop-Process -Force
Get-CimInstance Win32_Process -Filter "Name = 'rundll32.exe'" | Where-Object CommandLine -match "EliteSettings\.cpl" | Invoke-CimMethod -MethodName Terminate | Out-Null
Start-Sleep -Seconds 1

Write-Host "Triggering pre-build backup..." -ForegroundColor Cyan
$BackupScript = Join-Path $ScriptDir "backup.ps1"
if (Test-Path $BackupScript) {
    try { & $BackupScript } catch { Write-Warning "Backup failed, but continuing build." }
} else {
    Write-Warning "Backup script not found. Skipping backup."
}

Write-Host "========================================================" -ForegroundColor Magenta
$metaFile = Join-Path $ScriptDir "Build_MetaData.txt"
if (Test-Path $metaFile) {
    Get-Content $metaFile | Write-Host -ForegroundColor Cyan
}
Write-Host "========================================================" -ForegroundColor Magenta

Write-Host "Starting build process for Elite-TaskBar..." -ForegroundColor Cyan

# Define directories
$SourceDir = Join-Path $ScriptDir "SourceFiles"
$ResourcesDir = Join-Path $ScriptDir "Resources"
$OutputDir = $ScriptDir
$BuildDir = Join-Path $ScriptDir "BuildOutput"
$BuildDirx86 = Join-Path $ScriptDir "BuildOutputx86"

if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}
if (-not (Test-Path $BuildDirx86)) {
    New-Item -ItemType Directory -Path $BuildDirx86 | Out-Null
}

# Validate SourceFiles exists
if (-not (Test-Path $SourceDir)) {
    Write-Warning "SourceFiles directory not found. Please populate source files before building."
    exit 0
}

# Setup MSVC Environment
$vsPath = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools"

if (-not $vsPath) {
    Write-Error "Visual Studio C++ build tools not found!"
    exit 1
}

$vsDevCmd = Join-Path $vsPath "Common7\Tools\VsDevCmd.bat"

# Ensure fresh copy of the .ico files
$rootIconPath = Join-Path $ScriptDir "EliteTaskbar.ico"
$targetIconPath = Join-Path $ResourcesDir "MAIN_PROGRAM.ico"
if (Test-Path $rootIconPath) {
    Write-Host "Found EliteTaskbar.ico in root. Copying to Resources..." -ForegroundColor Yellow
    Copy-Item $rootIconPath -Destination $targetIconPath -Force
} else {
    Write-Host "No EliteTaskbar.ico found in root." -ForegroundColor DarkGray
}

$rootSettingsIconPath = Join-Path $ScriptDir "EliteSettings.ico"
$targetSettingsIcon1 = Join-Path $ResourcesDir "EliteSettings.ico"
$targetSettingsIcon2 = Join-Path $ResourcesDir "PREFERENCES.ico"
if (Test-Path $rootSettingsIconPath) {
    Write-Host "Found EliteSettings.ico in root. Copying to Resources..." -ForegroundColor Yellow
    Copy-Item $rootSettingsIconPath -Destination $targetSettingsIcon1 -Force
    Copy-Item $rootSettingsIconPath -Destination $targetSettingsIcon2 -Force
} else {
    Write-Host "No EliteSettings.ico found in root. Using default fallback if missing..." -ForegroundColor DarkGray
    if (-not (Test-Path $targetSettingsIcon1)) {
        Copy-Item $targetSettingsIcon2 -Destination $targetSettingsIcon1 -Force
    }
}

$rootEverythingIconPath = Join-Path $ScriptDir "EliteEverything.ico"
$targetEverythingIcon = Join-Path $ResourcesDir "EliteEverything.ico"
if (Test-Path $rootEverythingIconPath) {
    Copy-Item $rootEverythingIconPath -Destination $targetEverythingIcon -Force
}

$rootScannerIconPath = Join-Path $ScriptDir "EliteDLLScanner.ico"
$targetScannerIcon = Join-Path $ResourcesDir "EliteDLLScanner.ico"
if (Test-Path $rootScannerIconPath) {
    Copy-Item $rootScannerIconPath -Destination $targetScannerIcon -Force
}

$failed = $false
try {
    & "$ScriptDir\build_x64.ps1" -SourceDir $SourceDir -BuildDir $BuildDir -VsDevCmd $vsDevCmd
} catch {
    Write-Error "x64 Build failed: $_"
    $failed = $true
}

if (-not $failed) {
    try {
        & "$ScriptDir\build_x86.ps1" -SourceDir $SourceDir -BuildDir $BuildDirx86 -VsDevCmd $vsDevCmd
    } catch {
        Write-Error "x86 Build failed: $_"
        $failed = $true
    }
}

if (-not $failed) {
    try {
        & "$ScriptDir\build_settings.ps1" -SourceDir $SourceDir -BuildDir $BuildDir -BuildDirx86 $BuildDirx86 -VsDevCmd $vsDevCmd
    } catch {
        Write-Error "Settings Build failed: $_"
        $failed = $true
    }
}

if ($failed) {
    Write-Host "Build failed! Cleaning up recent backup..." -ForegroundColor Red
    $latestBackup = Get-ChildItem -Path (Join-Path $ScriptDir "Backups") -Filter *.cab | Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if ($latestBackup -and (New-TimeSpan -Start $latestBackup.LastWriteTime -End (Get-Date)).TotalMinutes -lt 5) {
        Remove-Item $latestBackup.FullName -Force
        Write-Host "Deleted backup ($($latestBackup.Name)) because the build failed." -ForegroundColor Yellow
    }
    Write-Error "Compilation failed!"
    exit 1
}

if (-not $failed) {
    Copy-Item "$BuildDir\EliteTaskbar.exe" "$ScriptDir\EliteTaskbar.exe" -Force
    Copy-Item "$BuildDir\EliteSettings.exe" "$ScriptDir\EliteSettings.exe" -Force
    Copy-Item "$BuildDir\EliteSettings.cpl" "$ScriptDir\EliteSettings.cpl" -Force
    Copy-Item "$BuildDir\EliteEverything.exe" "$ScriptDir\EliteEverything.exe" -Force
    Copy-Item "$BuildDir\EliteDLLScanner.exe" "$ScriptDir\EliteDLLScanner.exe" -Force
    
    # Run the separate signing stage
    & "$ScriptDir\build_sign.ps1" -BuildDir $BuildDir -BuildDirx86 $BuildDirx86
    
    Write-Host "Building Win32Explorer..." -ForegroundColor Cyan
    $origDir = Get-Location
    
    Set-Location "$ScriptDir\Win32Explorer_26.0.3.0"
    & ".\build_Win32Explorer.ps1" -Platform "x64"
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Win32Explorer x64 build failed!"
        Set-Location $origDir
        exit 1
    }
    Start-Sleep -Seconds 2
    & ".\build_Win32Explorer.ps1" -Platform "Win32"
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Win32Explorer Win32 build failed!"
        Set-Location $origDir
        exit 1
    }
    Set-Location $origDir
    
    # Relocate x64 artifact to root for developer execution
    Copy-Item "$BuildDir\Win32Explorer.exe" "$ScriptDir\Win32Explorer.exe" -Force
    
    Write-Host "Auto-committing and pushing to repository..." -ForegroundColor Cyan
    $ErrorActionPreference = 'Continue'
    git add .
    git commit -m "Auto-commit after successful build (build.ps1)"
    # git push origin HEAD
    $ErrorActionPreference = 'Stop'
    Write-Host "Done!" -ForegroundColor Green
}

Write-Host 'Compiling EliteStartMenu...' -ForegroundColor Cyan
Invoke-ps2exe -inputFile EliteStartMenu.ps1 -outputFile BuildOutput\EliteStartMenu.exe -noConsole -STA -iconFile Resources\PREFERENCES.ico
Invoke-ps2exe -inputFile EliteStartMenu.ps1 -outputFile BuildOutputx86\EliteStartMenu.exe -noConsole -STA -iconFile Resources\PREFERENCES.ico -x86
