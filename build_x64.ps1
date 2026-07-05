param(
    [string]$SourceDir,
    [string]$BuildDir,
    [string]$VsDevCmd
)
$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Path "$BuildDir\ObjectFiles" -Force | Out-Null
New-Item -ItemType Directory -Path "$BuildDir\DebugFiles" -Force | Out-Null
New-Item -ItemType Directory -Path "$BuildDir\ObjectLibraryFiles" -Force | Out-Null
New-Item -ItemType Directory -Path "$BuildDir\ResourceFiles" -Force | Out-Null

$compileCmd64 = "cl.exe /FS /EHsc /Zi /MTd /D_DEBUG /Fd`"$BuildDir\DebugFiles\taskbar64.pdb`" /Fe`"$BuildDir\EliteTaskbar.exe`" /Fo`"$BuildDir\ObjectFiles\\`" `"$SourceDir\main.cpp`" `"$SourceDir\Logger.cpp`" `"$SourceDir\TaskbarWindow.cpp`" `"$SourceDir\StartButton.cpp`" `"$SourceDir\ClockWidget.cpp`" `"$SourceDir\TrayIconScraper.cpp`" `"$SourceDir\TaskbarProperties.cpp`" `"$BuildDir\ResourceFiles\resources.res`" user32.lib advapi32.lib shell32.lib gdi32.lib dwmapi.lib comctl32.lib gdiplus.lib ole32.lib oleaut32.lib uxtheme.lib comdlg32.lib /link /PDB:`"$BuildDir\DebugFiles\EliteTaskbar.pdb`" /ILK:`"$BuildDir\DebugFiles\EliteTaskbar.ilk`" /MANIFEST:EMBED /MANIFESTINPUT:`"$SourceDir\app.manifest`" /MANIFESTUAC:NO"

$ErrorActionPreference = 'Continue'
Write-Host "Compiling x64 Resources and C++..."
cmd.exe /c "cd /d `"$BuildDir`" && call `"$VsDevCmd`" -arch=x64 && rc.exe /fo `"$BuildDir\ResourceFiles\resources.res`" `"$SourceDir\resources.rc`" && $compileCmd64" 2>&1
$ErrorActionPreference = 'Stop'

if ($LASTEXITCODE -ne 0) { throw "x64 Build failed with exit code $LASTEXITCODE" }
Write-Host "x64 Build finished successfully."
