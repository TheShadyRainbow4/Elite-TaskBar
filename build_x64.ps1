param(
    [string]$SourceDir,
    [string]$BuildDir,
    [string]$VsDevCmd
)
$ErrorActionPreference = 'Stop'
$compileCmd64 = "cl.exe /EHsc /Zi /MTd /D_DEBUG /Fd`"$BuildDir\taskbar64.pdb`" /Fe`"$BuildDir\EliteTaskbar.exe`" `"$SourceDir\main.cpp`" `"$SourceDir\Logger.cpp`" `"$SourceDir\TaskbarWindow.cpp`" `"$SourceDir\StartButton.cpp`" `"$SourceDir\ClockWidget.cpp`" `"$SourceDir\TaskbarProperties.cpp`" `"$BuildDir\resources.res`" user32.lib advapi32.lib shell32.lib gdi32.lib dwmapi.lib comctl32.lib gdiplus.lib ole32.lib uxtheme.lib comdlg32.lib /link /MANIFEST:EMBED /MANIFESTINPUT:`"$SourceDir\app.manifest`""

$ErrorActionPreference = 'Continue'
Write-Host "Compiling x64 Resources and C++..."
cmd.exe /c "cd /d `"$BuildDir`" && call `"$VsDevCmd`" -arch=x64 && rc.exe /fo `"$BuildDir\resources.res`" `"$SourceDir\resources.rc`" && $compileCmd64" 2>&1
$ErrorActionPreference = 'Stop'

if ($LASTEXITCODE -ne 0) { throw "x64 Build failed with exit code $LASTEXITCODE" }
Write-Host "x64 Build finished successfully."
