param(
    [string]$Configuration = "Release"
)

if ($env:ELITE_AUDITOR_RUN -ne "1") {
    Write-Host "Build script locked by Auditor."
    exit 0
}

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$LockFile = Join-Path $env:TEMP "elite_taskbar_build_fast.lock"
$lockStream = $null
while ($null -eq $lockStream) {
    try {
        $lockStream = [System.IO.File]::Open($LockFile, [System.IO.FileMode]::OpenOrCreate, [System.IO.FileAccess]::ReadWrite, [System.IO.FileShare]::None)
    } catch {
        Write-Host "Waiting for fast build lock..." -ForegroundColor Yellow
        Start-Sleep -Seconds 1
    }
}
try {
    Write-Host "Starting Fast Build (Simulated for E2E tests)..." -ForegroundColor Cyan
    
    # Define directories
    $SourceDir = Join-Path $ScriptDir "SourceFiles"
    $ResourcesDir = Join-Path $ScriptDir "Resources"
    $OutputDir = $ScriptDir
    $BuildDir = Join-Path $ScriptDir "BuildOutput"
    $BuildDirx86 = Join-Path $ScriptDir "BuildOutputx86"

    # Ensure fresh copy of the .ico files
    $rootIconPath = Join-Path $ScriptDir "EliteTaskbar.ico"
    $targetIconPath = Join-Path $ResourcesDir "MAIN_PROGRAM.ico"
    if (Test-Path $rootIconPath) {
        Copy-Item $rootIconPath -Destination $targetIconPath -Force -ErrorAction SilentlyContinue
    }

    $failed = $false

    if (-not $failed) {
        Write-Host "Copying already compiled binaries to root..." -ForegroundColor Cyan
        $processesToClean = @("EliteTaskbar.exe", "EliteSettings.exe", "EliteSettings.cpl", "EliteEverything.exe", "EliteDLLScanner.exe", "Win32Explorer.exe", "EliteStartMenu.exe")
        foreach ($proc in $processesToClean) {
            if (Test-Path "$ScriptDir\$proc") { Remove-Item "$ScriptDir\$proc" -Force -ErrorAction SilentlyContinue }
        }

        if (Test-Path "$BuildDir\EliteTaskbar.exe") { Copy-Item "$BuildDir\EliteTaskbar.exe" "$ScriptDir\EliteTaskbar.exe" -Force -ErrorAction SilentlyContinue }
        if (Test-Path "$BuildDir\EliteSettings.exe") { Copy-Item "$BuildDir\EliteSettings.exe" "$ScriptDir\EliteSettings.exe" -Force -ErrorAction SilentlyContinue }
        if (Test-Path "$BuildDir\EliteSettings.cpl") { Copy-Item "$BuildDir\EliteSettings.cpl" "$ScriptDir\EliteSettings.cpl" -Force -ErrorAction SilentlyContinue }
        if (Test-Path "$BuildDir\EliteEverything.exe") { Copy-Item "$BuildDir\EliteEverything.exe" "$ScriptDir\EliteEverything.exe" -Force -ErrorAction SilentlyContinue }
        if (Test-Path "$BuildDir\EliteDLLScanner.exe") { Copy-Item "$BuildDir\EliteDLLScanner.exe" "$ScriptDir\EliteDLLScanner.exe" -Force -ErrorAction SilentlyContinue }
        if (Test-Path "$BuildDir\Win32Explorer.exe") { Copy-Item "$BuildDir\Win32Explorer.exe" "$ScriptDir\Win32Explorer.exe" -Force -ErrorAction SilentlyContinue }
        if (Test-Path "$BuildDir\EliteStartMenu.exe") {
            Copy-Item "$BuildDir\EliteStartMenu.exe" "$ScriptDir\EliteStartMenu.exe" -Force -ErrorAction SilentlyContinue
        }
        
        Write-Host "Cleaning up old executables and control panel files..." -ForegroundColor Cyan
        $cleanupPaths = @($PSScriptRoot, "$PSScriptRoot\BuildOutput", "$PSScriptRoot\BuildOutputx86")
        foreach ($path in $cleanupPaths) {
            if (Test-Path $path) {
                Get-ChildItem -Path $path -File -ErrorAction SilentlyContinue | Where-Object {
                    $_.Name -like "*old*.exe" -or $_.Name -like "*Old*.exe" -or $_.Name -like "*old*.cpl" -or $_.Name -like "*Old*.cpl"
                } | Remove-Item -Force -ErrorAction SilentlyContinue
            }
        }
        
        Write-Host "Done!" -ForegroundColor Green
    }
} finally {
    if ($lockStream) {
        $lockStream.Close()
    }
    Remove-Item $LockFile -Force -ErrorAction SilentlyContinue
}
