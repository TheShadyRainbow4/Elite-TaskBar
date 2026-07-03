param(
    [string]$SourceDir,
    [string]$BuildDir,
    [string]$BuildDirx86,
    [string]$VsDevCmd
)
$ErrorActionPreference = 'Stop'
$stubCompileCmd64 = "cl.exe /EHsc /Zi /MTd /D_DEBUG /Fd`"$BuildDir\settings64.pdb`" /Fe`"$BuildDir\EliteSettings.exe`" `"$SourceDir\EliteSettingsStub.cpp`" `"$BuildDir\stub_resources.res`" user32.lib shell32.lib shlwapi.lib /link"
$stubCompileCmd86 = "cl.exe /EHsc /Zi /MTd /D_DEBUG /Fd`"$BuildDirx86\settings86.pdb`" /Fe`"$BuildDirx86\EliteSettings_x86.exe`" `"$SourceDir\EliteSettingsStub.cpp`" `"$BuildDirx86\stub_resources.res`" user32.lib shell32.lib shlwapi.lib /link"

$everyCompileCmd64 = "cl.exe /EHsc /Zi /MTd /D_DEBUG /Fd`"$BuildDir\everything64.pdb`" /Fe`"$BuildDir\EliteEverything.exe`" `"$SourceDir\EliteEverythingStub.cpp`" `"$BuildDir\everything_resources.res`" user32.lib shell32.lib shlwapi.lib /link"
$everyCompileCmd86 = "cl.exe /EHsc /Zi /MTd /D_DEBUG /Fd`"$BuildDirx86\everything86.pdb`" /Fe`"$BuildDirx86\EliteEverything_x86.exe`" `"$SourceDir\EliteEverythingStub.cpp`" `"$BuildDirx86\everything_resources.res`" user32.lib shell32.lib shlwapi.lib /link"

$dllCompileCmd64 = "cl.exe /EHsc /Zi /MTd /D_DEBUG /Fd`"$BuildDir\dllscanner64.pdb`" /Fe`"$BuildDir\EliteDLLScanner.exe`" `"$SourceDir\EliteDLLScannerStub.cpp`" `"$BuildDir\dll_resources.res`" user32.lib shell32.lib shlwapi.lib /link"
$dllCompileCmd86 = "cl.exe /EHsc /Zi /MTd /D_DEBUG /Fd`"$BuildDirx86\dllscanner86.pdb`" /Fe`"$BuildDirx86\EliteDLLScanner_x86.exe`" `"$SourceDir\EliteDLLScannerStub.cpp`" `"$BuildDirx86\dll_resources.res`" user32.lib shell32.lib shlwapi.lib /link"

$ErrorActionPreference = 'Continue'
Write-Host "Compiling Settings Stubs..."
cmd.exe /c "cd /d `"$BuildDir`" && call `"$VsDevCmd`" -arch=x64 && rc.exe /fo `"$BuildDir\stub_resources.res`" `"$SourceDir\EliteSettingsStub.rc`" && $stubCompileCmd64 && rc.exe /fo `"$BuildDir\everything_resources.res`" `"$SourceDir\EliteEverythingStub.rc`" && $everyCompileCmd64 && rc.exe /fo `"$BuildDir\dll_resources.res`" `"$SourceDir\EliteDLLScannerStub.rc`" && $dllCompileCmd64" 2>&1
if ($LASTEXITCODE -ne 0) { throw "Stubs x64 Build failed" }

cmd.exe /c "cd /d `"$BuildDirx86`" && call `"$VsDevCmd`" -arch=x86 && rc.exe /fo `"$BuildDirx86\stub_resources.res`" `"$SourceDir\EliteSettingsStub.rc`" && $stubCompileCmd86 && rc.exe /fo `"$BuildDirx86\everything_resources.res`" `"$SourceDir\EliteEverythingStub.rc`" && $everyCompileCmd86 && rc.exe /fo `"$BuildDirx86\dll_resources.res`" `"$SourceDir\EliteDLLScannerStub.rc`" && $dllCompileCmd86" 2>&1
if ($LASTEXITCODE -ne 0) { throw "Stubs x86 Build failed" }
$ErrorActionPreference = 'Stop'

Write-Host "Settings Stub Build finished successfully."
