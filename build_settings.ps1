param(
    [string]$SourceDir,
    [string]$BuildDir,
    [string]$BuildDirx86,
    [string]$VsDevCmd
)
$ErrorActionPreference = 'Stop'

# Create custom resource files that swap IDI_MAIN_PROGRAM and IDI_PREFERENCES so the Settings icon is the main one
$resH = Get-Content "$SourceDir\resource.h" -Raw
$resH = $resH -replace '#define IDI_MAIN_PROGRAM 101', '#define IDI_MAIN_PROGRAM 102'
$resH = $resH -replace '#define IDI_PREFERENCES  102', '#define IDI_PREFERENCES  101'
Set-Content "$BuildDir\settings_resource.h" -Value $resH
Set-Content "$BuildDirx86\settings_resource.h" -Value $resH

$resRC = Get-Content "$SourceDir\resources.rc" -Raw
$resRC = $resRC -replace '#include "resource.h"', '#include "settings_resource.h"'
Set-Content "$BuildDir\settings_resources.rc" -Value $resRC
Set-Content "$BuildDirx86\settings_resources.rc" -Value $resRC

$libs = "user32.lib shell32.lib shlwapi.lib comctl32.lib advapi32.lib uxtheme.lib gdi32.lib ole32.lib gdiplus.lib"
$srcSettings = "`"$SourceDir\EliteSettingsStub.cpp`" `"$SourceDir\TaskbarProperties.cpp`""

$stubCompileCmd64 = "cl.exe /EHsc /Zi /MTd /D_DEBUG /Fd`"$BuildDir\settings64.pdb`" /Fe`"$BuildDir\EliteSettings.exe`" /Fo`"$BuildDir\SettingsObj_exe_64_\\`" $srcSettings `"$BuildDir\settings_resources.res`" $libs /link /MANIFEST:EMBED /MANIFESTINPUT:`"$SourceDir\cpl.manifest`" /MANIFESTUAC:NO"
$stubCompileCmd86 = "cl.exe /EHsc /Zi /MTd /D_DEBUG /Fd`"$BuildDirx86\settings86.pdb`" /Fe`"$BuildDirx86\EliteSettings_x86.exe`" /Fo`"$BuildDirx86\SettingsObj_exe_86_\\`" $srcSettings `"$BuildDirx86\settings_resources.res`" $libs /link /MANIFEST:EMBED /MANIFESTINPUT:`"$SourceDir\cpl.manifest`" /MANIFESTUAC:NO"

$stubCPLCompileCmd64 = "cl.exe /LD /EHsc /Zi /MTd /D_DEBUG /Fd`"$BuildDir\settings_cpl64.pdb`" /Fe`"$BuildDir\EliteSettings.cpl`" /Fo`"$BuildDir\SettingsObj_cpl_64_\\`" $srcSettings `"$BuildDir\settings_resources.res`" $libs /link /MANIFEST:EMBED /MANIFESTINPUT:`"$SourceDir\cpl.manifest`" /MANIFESTUAC:NO"
$stubCPLCompileCmd86 = "cl.exe /LD /EHsc /Zi /MTd /D_DEBUG /Fd`"$BuildDirx86\settings_cpl86.pdb`" /Fe`"$BuildDirx86\EliteSettings_x86.cpl`" /Fo`"$BuildDirx86\SettingsObj_cpl_86_\\`" $srcSettings `"$BuildDirx86\settings_resources.res`" $libs /link /MANIFEST:EMBED /MANIFESTINPUT:`"$SourceDir\cpl.manifest`" /MANIFESTUAC:NO"

$everyCompileCmd64 = "cl.exe /EHsc /Zi /MTd /D_DEBUG /Fd`"$BuildDir\everything64.pdb`" /Fe`"$BuildDir\EliteEverything.exe`" `"$SourceDir\EliteEverythingStub.cpp`" `"$BuildDir\everything_resources.res`" user32.lib shell32.lib shlwapi.lib /link /MANIFEST:EMBED /MANIFESTINPUT:`"$SourceDir\cpl.manifest`" /MANIFESTUAC:NO"
$everyCompileCmd86 = "cl.exe /EHsc /Zi /MTd /D_DEBUG /Fd`"$BuildDirx86\everything86.pdb`" /Fe`"$BuildDirx86\EliteEverything_x86.exe`" `"$SourceDir\EliteEverythingStub.cpp`" `"$BuildDirx86\everything_resources.res`" user32.lib shell32.lib shlwapi.lib /link /MANIFEST:EMBED /MANIFESTINPUT:`"$SourceDir\cpl.manifest`" /MANIFESTUAC:NO"

$dllCompileCmd64 = "cl.exe /EHsc /Zi /MTd /D_DEBUG /Fd`"$BuildDir\dllscanner64.pdb`" /Fe`"$BuildDir\EliteDLLScanner.exe`" `"$SourceDir\EliteDLLScannerStub.cpp`" `"$BuildDir\dll_resources.res`" user32.lib shell32.lib shlwapi.lib /link /MANIFEST:EMBED /MANIFESTINPUT:`"$SourceDir\cpl.manifest`" /MANIFESTUAC:NO"
$dllCompileCmd86 = "cl.exe /EHsc /Zi /MTd /D_DEBUG /Fd`"$BuildDirx86\dllscanner86.pdb`" /Fe`"$BuildDirx86\EliteDLLScanner_x86.exe`" `"$SourceDir\EliteDLLScannerStub.cpp`" `"$BuildDirx86\dll_resources.res`" user32.lib shell32.lib shlwapi.lib /link /MANIFEST:EMBED /MANIFESTINPUT:`"$SourceDir\cpl.manifest`" /MANIFESTUAC:NO"

New-Item -ItemType Directory -Path "$BuildDir\SettingsObj_exe_64_" -Force | Out-Null
New-Item -ItemType Directory -Path "$BuildDirx86\SettingsObj_exe_86_" -Force | Out-Null
New-Item -ItemType Directory -Path "$BuildDir\SettingsObj_cpl_64_" -Force | Out-Null
New-Item -ItemType Directory -Path "$BuildDirx86\SettingsObj_cpl_86_" -Force | Out-Null

$ErrorActionPreference = 'Continue'
Write-Host "Compiling Settings Stubs..."
cmd.exe /c "cd /d `"$BuildDir`" && call `"$VsDevCmd`" -arch=x64 && rc.exe /fo `"$BuildDir\settings_resources.res`" `"$BuildDir\settings_resources.rc`" && $stubCompileCmd64 && $stubCPLCompileCmd64 && rc.exe /fo `"$BuildDir\everything_resources.res`" `"$SourceDir\EliteEverythingStub.rc`" && $everyCompileCmd64 && rc.exe /fo `"$BuildDir\dll_resources.res`" `"$SourceDir\EliteDLLScannerStub.rc`" && $dllCompileCmd64" 2>&1
if ($LASTEXITCODE -ne 0) { throw "Stubs x64 Build failed" }

cmd.exe /c "cd /d `"$BuildDirx86`" && call `"$VsDevCmd`" -arch=x86 && rc.exe /fo `"$BuildDirx86\settings_resources.res`" `"$BuildDirx86\settings_resources.rc`" && $stubCompileCmd86 && $stubCPLCompileCmd86 && rc.exe /fo `"$BuildDirx86\everything_resources.res`" `"$SourceDir\EliteEverythingStub.rc`" && $everyCompileCmd86 && rc.exe /fo `"$BuildDirx86\dll_resources.res`" `"$SourceDir\EliteDLLScannerStub.rc`" && $dllCompileCmd86" 2>&1
if ($LASTEXITCODE -ne 0) { throw "Stubs x86 Build failed" }
$ErrorActionPreference = 'Stop'

Write-Host "Settings Stub Build finished successfully."
