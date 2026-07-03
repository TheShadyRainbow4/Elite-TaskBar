param(
    [string]$Configuration = "Release"
)

# Elite-TaskBar Build Script
$ErrorActionPreference = 'Stop'
$ScriptDir = $PSScriptRoot

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

$job1 = Start-Job -ScriptBlock { param($s, $b, $v) & "$s\..\build_x64.ps1" -SourceDir $s -BuildDir $b -VsDevCmd $v } -ArgumentList $SourceDir, $BuildDir, $vsDevCmd
$job2 = Start-Job -ScriptBlock { param($s, $b, $v) & "$s\..\build_x86.ps1" -SourceDir $s -BuildDir $b -VsDevCmd $v } -ArgumentList $SourceDir, $BuildDirx86, $vsDevCmd
$job3 = Start-Job -ScriptBlock { param($s, $b, $bx86, $v) & "$s\..\build_settings.ps1" -SourceDir $s -BuildDir $b -BuildDirx86 $bx86 -VsDevCmd $v } -ArgumentList $SourceDir, $BuildDir, $BuildDirx86, $vsDevCmd

Write-Host "Waiting for concurrent builds to finish..." -ForegroundColor Cyan
$jobs = @($job1, $job2, $job3)
Wait-Job -Job $jobs | Out-Null

$failed = $false
foreach ($job in $jobs) {
    try { Receive-Job -Job $job -ErrorAction Stop | Write-Host } catch { Write-Host $_.Exception.Message -ForegroundColor Yellow }
    if ($job.State -ne 'Completed') {
        Write-Error "Build job $($job.Id) failed!"
        $failed = $true
    }
}
Remove-Job -Job $jobs

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

Write-Host "Build finished. Copying binaries to root..." -ForegroundColor Green
Copy-Item "$BuildDir\EliteTaskbar.exe" -Destination "$ScriptDir\EliteTaskbar.exe" -Force
Copy-Item "$BuildDir\EliteSettings.exe" -Destination "$ScriptDir\EliteSettings.exe" -Force
Copy-Item "$BuildDir\EliteEverything.exe" -Destination "$ScriptDir\EliteEverything.exe" -Force
Copy-Item "$BuildDir\EliteDLLScanner.exe" -Destination "$ScriptDir\EliteDLLScanner.exe" -Force

Write-Host "Signing executables using signtool..." -ForegroundColor Cyan
$pfxPath = Join-Path $ScriptDir "Elite-EasySigner\EliteSoftware_Special.pfx"
$password = "Minecraft145!!"
$signtool = Get-ChildItem -Path "C:\Program Files (x86)\Windows Kits\10\bin" -Filter "signtool.exe" -Recurse | Where-Object { $_.FullName -match "\\x64\\" } | Select-Object -First 1 -ExpandProperty FullName

if ((Test-Path $pfxPath) -and $signtool) {
    $exeFiles = @(
        (Join-Path $ScriptDir "EliteTaskbar.exe"),
        (Join-Path $ScriptDir "EliteSettings.exe"),
        (Join-Path $ScriptDir "EliteEverything.exe"),
        (Join-Path $ScriptDir "EliteDLLScanner.exe"),
        (Join-Path $BuildDirx86 "EliteTaskbar_x86.exe"),
        (Join-Path $BuildDirx86 "EliteSettings_x86.exe"),
        (Join-Path $BuildDirx86 "EliteEverything_x86.exe"),
        (Join-Path $BuildDirx86 "EliteDLLScanner_x86.exe")
    )
    foreach ($file in $exeFiles) {
        if (Test-Path $file) {
            Write-Host "Signing $file..." -ForegroundColor Yellow
            & $signtool sign /f $pfxPath /p $password /fd SHA256 /t http://timestamp.digicert.com /v $file
        }
    }
} else {
    Write-Warning "signtool.exe or EliteSoftware_Special.pfx not found. Skipping signature."
}

Write-Host "Auto-committing and pushing to repository..." -ForegroundColor Cyan
Set-Location -Path $ScriptDir
git add .
git commit -m "Auto-commit after successful build (build.ps1)"
git push origin master
Write-Host "Done!" -ForegroundColor Green
