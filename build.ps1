param(
    [string]$Configuration = "Release"
)

# Elite-TaskBar Build Script
$ErrorActionPreference = 'Stop'
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition

Write-Host "Triggering pre-build backup..." -ForegroundColor Cyan
$BackupScript = Join-Path $ScriptDir "backup.ps1"
if (Test-Path $BackupScript) {
    & $BackupScript
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
$BuildDirx86 = Join-Path $BuildDir "x86"

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
$compileCmd64 = "cl.exe /EHsc /Zi /MTd /D_DEBUG /Fe`"$BuildDir\EliteTaskbar.exe`" `"$SourceDir\main.cpp`" `"$SourceDir\Logger.cpp`" `"$SourceDir\TaskbarWindow.cpp`" `"$SourceDir\StartButton.cpp`" `"$SourceDir\ClockWidget.cpp`" `"$SourceDir\TaskbarProperties.cpp`" `"$BuildDir\resources.res`" user32.lib advapi32.lib shell32.lib gdi32.lib dwmapi.lib comctl32.lib gdiplus.lib ole32.lib uxtheme.lib comdlg32.lib /link /MANIFEST:EMBED /MANIFESTINPUT:`"$SourceDir\app.manifest`""
$compileCmd86 = "cl.exe /EHsc /Zi /MTd /D_DEBUG /Fe`"$BuildDirx86\EliteTaskbar_x86.exe`" `"$SourceDir\main.cpp`" `"$SourceDir\Logger.cpp`" `"$SourceDir\TaskbarWindow.cpp`" `"$SourceDir\StartButton.cpp`" `"$SourceDir\ClockWidget.cpp`" `"$SourceDir\TaskbarProperties.cpp`" `"$BuildDirx86\resources.res`" user32.lib advapi32.lib shell32.lib gdi32.lib dwmapi.lib comctl32.lib gdiplus.lib ole32.lib uxtheme.lib comdlg32.lib /link /MANIFEST:EMBED /MANIFESTINPUT:`"$SourceDir\app.manifest`""

# Ensure fresh copy of the .ico files
$rootIconPath = Join-Path $ScriptDir "EliteTaskbar.ico"
$targetIconPath = Join-Path $ResourcesDir "elite_icon.ico"
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
    # Fallback to PREFERENCES.ico if missing
    if (-not (Test-Path $targetSettingsIcon1)) {
        Copy-Item $targetSettingsIcon2 -Destination $targetSettingsIcon1 -Force
    }
}

$stubCompileCmd64 = "cl.exe /EHsc /Zi /MTd /D_DEBUG /Fe`"$BuildDir\EliteSettings.exe`" `"$SourceDir\EliteSettingsStub.cpp`" `"$BuildDir\stub_resources.res`" user32.lib shell32.lib shlwapi.lib /link"
$stubCompileCmd86 = "cl.exe /EHsc /Zi /MTd /D_DEBUG /Fe`"$BuildDirx86\EliteSettings_x86.exe`" `"$SourceDir\EliteSettingsStub.cpp`" `"$BuildDirx86\stub_resources.res`" user32.lib shell32.lib shlwapi.lib /link"

Write-Host "Compiling x64 Resources and C++..." -ForegroundColor Cyan
cmd.exe /c "cd /d `"$BuildDir`" && call `"$vsDevCmd`" -arch=x64 && rc.exe /fo `"$BuildDir\resources.res`" `"$SourceDir\resources.rc`" && rc.exe /fo `"$BuildDir\stub_resources.res`" `"$SourceDir\EliteSettingsStub.rc`" && $compileCmd64 && $stubCompileCmd64"
$exit64 = $LASTEXITCODE

Write-Host "Compiling x86 Resources and C++..." -ForegroundColor Cyan
cmd.exe /c "cd /d `"$BuildDirx86`" && call `"$vsDevCmd`" -arch=x86 && rc.exe /fo `"$BuildDirx86\resources.res`" `"$SourceDir\resources.rc`" && rc.exe /fo `"$BuildDirx86\stub_resources.res`" `"$SourceDir\EliteSettingsStub.rc`" && $compileCmd86 && $stubCompileCmd86"
$exit86 = $LASTEXITCODE

if ($exit64 -ne 0 -or $exit86 -ne 0) {
    Write-Host "Build failed! Cleaning up recent backup..." -ForegroundColor Red
    $latestBackup = Get-ChildItem -Path (Join-Path $ScriptDir "Backups") -Filter *.cab | Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if ($latestBackup -and (New-TimeSpan -Start $latestBackup.LastWriteTime -End (Get-Date)).TotalMinutes -lt 5) {
        Remove-Item $latestBackup.FullName -Force
        Write-Host "Deleted backup ($($latestBackup.Name)) because the build failed." -ForegroundColor Yellow
    }
    Write-Error "Compilation failed!"
    exit 1
}

Write-Host "Build finished. Copying binaries to root..." -ForegroundColor Green
Copy-Item "$BuildDir\EliteTaskbar.exe" -Destination "$ScriptDir\EliteTaskbar.exe" -Force
Copy-Item "$BuildDir\EliteSettings.exe" -Destination "$ScriptDir\EliteSettings.exe" -Force

Write-Host "Auto-committing and pushing to repository..." -ForegroundColor Cyan
Set-Location -Path $ScriptDir
git add .
git commit -m "Auto-commit after successful build (build.ps1)"
git push origin master
Write-Host "Done!" -ForegroundColor Green
