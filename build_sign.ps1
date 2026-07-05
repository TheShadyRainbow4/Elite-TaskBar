param(
    [string]$BuildDir,
    [string]$BuildDirx86
)

$ErrorActionPreference = 'Stop'

$signerTool = "$PSScriptRoot\Elite-EasySigner\Elite-EasySigner_x64.exe"

if (-not (Test-Path $signerTool)) {
    Write-Warning "Could not find Elite-EasySigner at $signerTool. Skipping signature."
    exit 0
}

Write-Host "Signing executables using Elite-EasySigner..." -ForegroundColor Cyan

$exeFiles = @(
    (Join-Path $PSScriptRoot "EliteTaskbar.exe"),
    (Join-Path $PSScriptRoot "EliteSettings.exe"),
    (Join-Path $PSScriptRoot "EliteSettings.cpl"),
    (Join-Path $PSScriptRoot "EliteEverything.exe"),
    (Join-Path $PSScriptRoot "EliteDLLScanner.exe"),
    (Join-Path $BuildDir "EliteTaskbar.exe"),
    (Join-Path $BuildDir "EliteSettings.exe"),
    (Join-Path $BuildDir "EliteSettings.cpl"),
    (Join-Path $BuildDir "EliteEverything.exe"),
    (Join-Path $BuildDir "EliteDLLScanner.exe"),
    (Join-Path $BuildDirx86 "EliteTaskbar_x86.exe"),
    (Join-Path $BuildDirx86 "EliteSettings_x86.exe"),
    (Join-Path $BuildDirx86 "EliteSettings_x86.cpl"),
    (Join-Path $BuildDirx86 "EliteEverything_x86.exe"),
    (Join-Path $BuildDirx86 "EliteDLLScanner_x86.exe")
)

foreach ($file in $exeFiles) {
    if (Test-Path $file) {
        Write-Host "Signing $file..." -ForegroundColor Yellow
        & $signerTool $file | Out-Null
    }
}

Write-Host "Signing stage completed!" -ForegroundColor Green
