param(
    [string]$BuildDir,
    [string]$BuildDirx86
)

$ErrorActionPreference = 'Stop'

Write-Host "Signing executables using signtool.exe..." -ForegroundColor Cyan

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

$signtool = (Get-ChildItem -Path "C:\Program Files (x86)\Windows Kits\10\bin\*" -Filter signtool.exe -Recurse | Where-Object { $_.FullName -like "*x64*" } | Select-Object -First 1).FullName
if (-not $signtool) {
    $signtool = (Get-ChildItem -Path "C:\Program Files (x86)\Windows Kits\10\bin\*" -Filter signtool.exe -Recurse | Select-Object -First 1).FullName
}

$pfx = Join-Path $PSScriptRoot "Elite-EasySigner\EliteSoftware_Special.pfx"
$pass = "Minecraft145!!"

if ($signtool -and (Test-Path $pfx)) {
    Write-Host "Found signtool: $signtool"
    foreach ($file in $exeFiles) {
        if (Test-Path $file) {
            Write-Host "Signing $file..."
            & $signtool sign /f $pfx /p $pass /fd SHA256 /v $file
        }
    }
} else {
    Write-Warning "signtool.exe or PFX certificate not found! Skipping signing."
}

Write-Host "Signing stage completed!" -ForegroundColor Green
