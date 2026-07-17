param(
    [string]$Configuration = "Release",
    [string]$Target = "All"
)

if ($env:ELITE_AUDITOR_RUN -ne "1") {
    Write-Host "Build script locked by Auditor."
    exit 0
}


# Elite-TaskBar Build Script
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition

$LockFile = Join-Path $env:TEMP "elite_taskbar_build_v2.lock"
$lockStream = $null
while ($null -eq $lockStream) {
    try {
        $lockStream = [System.IO.File]::Open($LockFile, [System.IO.FileMode]::OpenOrCreate, [System.IO.FileAccess]::ReadWrite, [System.IO.FileShare]::None)
    } catch {
        Write-Host "Waiting for another agent to finish building..." -ForegroundColor Yellow
        Start-Sleep -Seconds 2
    }
}
try {
$BackupScript = Join-Path $ScriptDir "backup.ps1"
$ErrorActionPreference = 'Stop'

Write-Host "Checking for running Elite processes..." -ForegroundColor Cyan
Get-Process -Name EliteTaskbar, EliteSettings, EliteEverything, EliteDLLScanner, Win32Explorer -ErrorAction SilentlyContinue | Stop-Process -Force
Get-CimInstance Win32_Process -Filter "Name = 'rundll32.exe'" | Where-Object CommandLine -match "EliteSettings\.cpl" | Invoke-CimMethod -MethodName Terminate | Out-Null
Start-Sleep -Seconds 1

Write-Host "Triggering pre-build backup..." -ForegroundColor Cyan
$BackupScript = Join-Path $ScriptDir "backup.ps1"
if (Test-Path $BackupScript) {
    try {
        & $BackupScript -BuildOutputOnly
    } catch {
        Write-Warning "Backup failed, but continuing build."
    }
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

# Pre-build Synchronisation: Copy files to Win32Explorer submodule
$submodDir = Join-Path $ScriptDir "Win32Explorer_26.0.3.0\App_Source\EliteTaskbar"
$submodDirRemaining = Join-Path $ScriptDir "Remaining_Shell\Win32Explorer_26.0.3.0\App_Source\EliteTaskbar"

Write-Host "Pre-build Synchronisation: Copying source files to Win32Explorer..." -ForegroundColor Yellow
if (-not (Test-Path $submodDir)) {
    New-Item -ItemType Directory -Path $submodDir -Force | Out-Null
}
if (-not (Test-Path $submodDirRemaining)) {
    New-Item -ItemType Directory -Path $submodDirRemaining -Force | Out-Null
}

# Dynamically copy all source files to submodules - Draftsman-Dan-Gen2
Get-ChildItem -Path $SourceDir -File | ForEach-Object {
    $fileName = $_.Name
    Copy-Item $_.FullName -Destination (Join-Path $submodDir $fileName) -Force
    Copy-Item $_.FullName -Destination (Join-Path $submodDirRemaining $fileName) -Force
}

# Adjust resource paths in the copied resources.rc for submodule compilation contexts
$targetRc = Join-Path $submodDir "resources.rc"
if (Test-Path $targetRc) {
    $content = Get-Content $targetRc -Raw
    $newContent = $content.Replace('..\\Resources\\', 'EliteTaskbar\\Resources\\')
    Set-Content -Path $targetRc -Value $newContent -NoNewline
}
$targetRcRemaining = Join-Path $submodDirRemaining "resources.rc"
if (Test-Path $targetRcRemaining) {
    $content = Get-Content $targetRcRemaining -Raw
    $newContent = $content.Replace('..\\Resources\\', 'EliteTaskbar\\Resources\\')
    Set-Content -Path $targetRcRemaining -Value $newContent -NoNewline
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
if ($Target -eq "All" -or $Target -eq "Taskbar") {
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
}

if (-not $failed -and ($Target -eq "All" -or $Target -eq "Settings")) {
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

if (-not $failed -and ($Target -eq "All" -or $Target -eq "Win32Explorer")) {
    # 1. Compile Win32Explorer
    Write-Host "Building Win32Explorer..." -ForegroundColor Cyan
    $origDir = Get-Location
    Set-Location "$ScriptDir\Win32Explorer_26.0.3.0"
    & ".\build_Win32Explorer.ps1" -Platform "x64"
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Win32Explorer x64 build failed!"
        $failed = $true
    } else {
        Start-Sleep -Seconds 2
        & ".\build_Win32Explorer.ps1" -Platform "Win32"
        if ($LASTEXITCODE -ne 0) {
            Write-Error "Win32Explorer Win32 build failed!"
            $failed = $true
        }
    }
    Set-Location $origDir
}

if (-not $failed -and ($Target -eq "All" -or $Target -eq "StartMenu") -and (Test-Path "$ScriptDir\SourceFiles\EliteStartMenu.cpp")) {
    # 2. Compile EliteStartMenu (Native C++ - PS2EXE is forbidden)
    try {
        Write-Host 'Compiling EliteStartMenu (Native C++)...' -ForegroundColor Cyan
        $startMenuSrc = "`"$ScriptDir\SourceFiles\EliteStartMenu.cpp`""
        $startMenuRc  = "`"$ScriptDir\SourceFiles\EliteStartMenu.rc`""
        $startMenuLibs = "user32.lib shell32.lib shlwapi.lib comctl32.lib advapi32.lib uxtheme.lib gdi32.lib ole32.lib"
        $startMenuManifest = "`"$ScriptDir\SourceFiles\cpl.manifest`""

        # x64 build
        cmd.exe /c "cd /d `"$BuildDir`" && call `"$vsDevCmd`" -arch=x64 && rc.exe /fo `"$BuildDir\ResourceFiles\startmenu_resources.res`" $startMenuRc"
        if ($LASTEXITCODE -ne 0) { throw "EliteStartMenu x64 RC failed" }
        cmd.exe /c "cd /d `"$BuildDir`" && call `"$vsDevCmd`" -arch=x64 && cl.exe /FS /EHsc /MT /DNDEBUG /Fe`"$BuildDir\EliteStartMenu.exe`" /Fo`"$BuildDir\ObjectFiles/`" $startMenuSrc `"$BuildDir\ResourceFiles\startmenu_resources.res`" $startMenuLibs /link /MANIFEST:EMBED /MANIFESTINPUT:$startMenuManifest /MANIFESTUAC:NO"
        if ($LASTEXITCODE -ne 0) { throw "EliteStartMenu x64 cl failed" }

        # x86 build
        cmd.exe /c "cd /d `"$BuildDirx86`" && call `"$vsDevCmd`" -arch=x86 && rc.exe /fo `"$BuildDirx86\ResourceFiles\startmenu_resources.res`" $startMenuRc"
        if ($LASTEXITCODE -ne 0) { throw "EliteStartMenu x86 RC failed" }
        cmd.exe /c "cd /d `"$BuildDirx86`" && call `"$vsDevCmd`" -arch=x86 && cl.exe /FS /EHsc /MT /DNDEBUG /Fe`"$BuildDirx86\EliteStartMenu.exe`" /Fo`"$BuildDirx86\ObjectFiles/`" $startMenuSrc `"$BuildDirx86\ResourceFiles\startmenu_resources.res`" $startMenuLibs /link /MANIFEST:EMBED /MANIFESTINPUT:$startMenuManifest /MANIFESTUAC:NO"
        if ($LASTEXITCODE -ne 0) { throw "EliteStartMenu x86 cl failed" }
    } catch {
        Write-Error "EliteStartMenu compilation failed: $_"
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

if (-not $failed -and ($Target -eq "All" -or $Target -eq "PostBuild")) {
    $processesToClean = @("EliteTaskbar.exe", "EliteSettings.exe", "EliteSettings.cpl", "EliteEverything.exe", "EliteDLLScanner.exe", "Win32Explorer.exe", "EliteStartMenu.exe")
    foreach ($proc in $processesToClean) {
        if (Test-Path "$ScriptDir\$proc") { Remove-Item "$ScriptDir\$proc" -Force -ErrorAction SilentlyContinue }
    }

    if (Test-Path "$BuildDir\EliteTaskbar.exe") { Copy-Item "$BuildDir\EliteTaskbar.exe" "$ScriptDir\EliteTaskbar.exe" -Force }
    if (Test-Path "$BuildDir\EliteSettings.cpl") { Copy-Item "$BuildDir\EliteSettings.cpl" "$ScriptDir\EliteSettings.cpl" -Force }
    if (Test-Path "$BuildDir\EliteEverything.exe") { Copy-Item "$BuildDir\EliteEverything.exe" "$ScriptDir\EliteEverything.exe" -Force }
    if (Test-Path "$BuildDir\EliteDLLScanner.exe") { Copy-Item "$BuildDir\EliteDLLScanner.exe" "$ScriptDir\EliteDLLScanner.exe" -Force }
    if (Test-Path "$BuildDir\Win32Explorer.exe") { Copy-Item "$BuildDir\Win32Explorer.exe" "$ScriptDir\Win32Explorer.exe" -Force }
    if (Test-Path "$BuildDir\EliteStartMenu.exe") {
        Copy-Item "$BuildDir\EliteStartMenu.exe" "$ScriptDir\EliteStartMenu.exe" -Force
    }
    
    # Run the separate signing stage (after all compilations and copy actions are completed)
    & "$ScriptDir\build_sign.ps1" -BuildDir $BuildDir -BuildDirx86 $BuildDirx86
    
    Write-Host "Cleaning up old executables and control panel files..." -ForegroundColor Cyan
    $cleanupPaths = @($PSScriptRoot, "$PSScriptRoot\BuildOutput", "$PSScriptRoot\BuildOutputx86")
    foreach ($path in $cleanupPaths) {
        if (Test-Path $path) {
            Get-ChildItem -Path $path -File -ErrorAction SilentlyContinue | Where-Object {
                $_.Name -like "*old*.exe" -or $_.Name -like "*Old*.exe" -or $_.Name -like "*old*.cpl" -or $_.Name -like "*Old*.cpl"
            } | Remove-Item -Force -ErrorAction SilentlyContinue
        }
    }
    
    Write-Host "Executing deployment script to mirror files via hardlinks..." -ForegroundColor Cyan
    & "$PSScriptRoot\deploy_hardlinks.ps1" -BuildDir $BuildDir -BuildDirx86 $BuildDirx86

    
    Write-Host "Auto-committing submodules and main repository..." -ForegroundColor Cyan
    $ErrorActionPreference = 'Continue'
    
    Get-ChildItem -Path $PSScriptRoot -Filter ".git" -Recurse -Directory -Force -ErrorAction SilentlyContinue | ForEach-Object {
        $repoPath = $_.Parent.FullName
        if ($repoPath -ne $PSScriptRoot) {
            git -C $repoPath add .
            git -C $repoPath commit -m "Auto-commit after successful build (build.ps1)"
        }
    }
    
    git add .
    git commit -m "Auto-commit after successful build (build.ps1)"
    # git push origin HEAD
    $ErrorActionPreference = 'Stop'
    
    Write-Host "Done!" -ForegroundColor Green
}

} finally {
    if ($lockStream) {
        $lockStream.Close()
    }
    Remove-Item $LockFile -Force -ErrorAction SilentlyContinue
}
