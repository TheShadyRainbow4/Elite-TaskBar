param(
    [string]$BuildDir,
    [string]$BuildDirx86
)

$ErrorActionPreference = 'Stop'

$signtool = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64\signtool.exe"
if (-not (Test-Path $signtool)) {
    $signtool = "C:\Program Files (x86)\Windows Kits\10\bin\x64\signtool.exe"
}

$pfxPath = "$PSScriptRoot\Elite-EasySigner\EliteSoftware_Special.pfx"
$password = "Minecraft145!!"

if (-not (Test-Path $pfxPath)) {
    Write-Warning "Could not find certificate at $pfxPath. Skipping signature."
    exit 0
}

Write-Host "Signing executables using signtool..." -ForegroundColor Cyan

$exeFiles = @(
    (Join-Path $PSScriptRoot "EliteTaskbar.exe"),
    (Join-Path $PSScriptRoot "EliteSettings.exe"),
    (Join-Path $PSScriptRoot "EliteEverything.exe"),
    (Join-Path $PSScriptRoot "EliteDLLScanner.exe"),
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

Write-Host "Signing stage completed!" -ForegroundColor Green
