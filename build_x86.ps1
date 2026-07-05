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

$compileCmd86 = "cl.exe /FS /EHsc /MTd /D_DEBUG /Fe`"$BuildDir\EliteTaskbar_x86.exe`" /Fo`"$BuildDir\ObjectFiles/`" `"$SourceDir\main.cpp`" `"$SourceDir\Logger.cpp`" `"$SourceDir\TaskbarWindow.cpp`" `"$SourceDir\StartButton.cpp`" `"$SourceDir\ClockWidget.cpp`" `"$SourceDir\TrayIconScraper.cpp`" `"$SourceDir\TaskbarProperties.cpp`" `"$SourceDir\DesktopWindow.cpp`" `"$BuildDir\ResourceFiles\resources.res`" user32.lib advapi32.lib shell32.lib gdi32.lib dwmapi.lib comctl32.lib gdiplus.lib ole32.lib oleaut32.lib uxtheme.lib comdlg32.lib /link /MANIFEST:EMBED /MANIFESTINPUT:`"$SourceDir\app.manifest`" /MANIFESTUAC:NO"

$ErrorActionPreference = 'Continue'
Write-Host "Compiling x86 Resources and C++..."
cmd.exe /c "cd /d `"$BuildDir`" && call `"$VsDevCmd`" -arch=x86 && rc.exe /fo `"$BuildDir\ResourceFiles\resources.res`" `"$SourceDir\resources.rc`" && $compileCmd86"
$ErrorActionPreference = 'Stop'

if ($LASTEXITCODE -ne 0) { throw "x86 Build failed with exit code $LASTEXITCODE" }
Write-Host "x86 Build finished successfully."

