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

New-Item -ItemType Directory -Path "$BuildDir\ObjectFiles" -Force | Out-Null
New-Item -ItemType Directory -Path "$BuildDir\DebugFiles" -Force | Out-Null
New-Item -ItemType Directory -Path "$BuildDir\ObjectLibraryFiles" -Force | Out-Null
New-Item -ItemType Directory -Path "$BuildDir\ResourceFiles" -Force | Out-Null

New-Item -ItemType Directory -Path "$BuildDirx86\ObjectFiles" -Force | Out-Null
New-Item -ItemType Directory -Path "$BuildDirx86\DebugFiles" -Force | Out-Null
New-Item -ItemType Directory -Path "$BuildDirx86\ObjectLibraryFiles" -Force | Out-Null
New-Item -ItemType Directory -Path "$BuildDirx86\ResourceFiles" -Force | Out-Null

Set-Content "$BuildDir\ResourceFiles\settings_resource.h" -Value $resH
Set-Content "$BuildDirx86\ResourceFiles\settings_resource.h" -Value $resH

$resRC = Get-Content "$SourceDir\resources.rc" -Raw
$resRC = $resRC -replace '#include "resource.h"', '#include "settings_resource.h"'
$resRC = $resRC -replace '\.\.\\Resources', '..\..\Resources'
Set-Content "$BuildDir\ResourceFiles\settings_resources.rc" -Value $resRC
Set-Content "$BuildDirx86\ResourceFiles\settings_resources.rc" -Value $resRC

$cplRC = Get-Content "$SourceDir\settings_cpl.rc" -Raw
$cplRCx86 = $cplRC -replace 'EliteSettings.exe', 'EliteSettings_x86.exe'
Set-Content "$SourceDir\settings_cpl_x86.rc" -Value $cplRCx86

$libs = "user32.lib shell32.lib shlwapi.lib comctl32.lib advapi32.lib uxtheme.lib gdi32.lib ole32.lib gdiplus.lib"
# Add Logger.cpp to settings compile sources to resolve EliteMessageBoxW - Builder-Bob
$srcSettings = "`"$SourceDir\EliteSettingsStub.cpp`" `"$SourceDir\TaskbarProperties.cpp`" `"$SourceDir\Logger.cpp`""

$stubCompileCmd64 = "cl.exe /FS /EHsc /MTd /D_DEBUG /Fe`"$BuildDir\EliteSettings.exe`" /Fo`"$BuildDir\ObjectFiles/`" $srcSettings `"$BuildDir\ResourceFiles\settings_resources.res`" $libs /link /MANIFEST:EMBED /MANIFESTINPUT:`"$SourceDir\cpl.manifest`" /MANIFESTUAC:NO"
$stubCompileCmd86 = "cl.exe /FS /EHsc /MTd /D_DEBUG /Fe`"$BuildDirx86\EliteSettings_x86.exe`" /Fo`"$BuildDirx86\ObjectFiles/`" $srcSettings `"$BuildDirx86\ResourceFiles\settings_resources.res`" $libs /link /MANIFEST:EMBED /MANIFESTINPUT:`"$SourceDir\cpl.manifest`" /MANIFESTUAC:NO"

$stubCPLCompileCmd64 = "cl.exe /FS /LD /EHsc /MTd /D_DEBUG /Fe`"$BuildDir\EliteSettings.cpl`" /Fo`"$BuildDir\ObjectFiles/`" `"$SourceDir\EliteSettingsCpl.cpp`" `"$BuildDir\ResourceFiles\settings_cpl.res`" $libs /link /IMPLIB:`"$BuildDir\ObjectLibraryFiles\EliteSettings_cpl.lib`" /EXPORT:CPlApplet /MANIFEST:EMBED /MANIFESTINPUT:`"$SourceDir\cpl.manifest`" /MANIFESTUAC:NO"
$stubCPLCompileCmd86 = "cl.exe /FS /LD /EHsc /MTd /D_DEBUG /Fe`"$BuildDirx86\EliteSettings_x86.cpl`" /Fo`"$BuildDirx86\ObjectFiles/`" `"$SourceDir\EliteSettingsCpl.cpp`" `"$BuildDirx86\ResourceFiles\settings_cpl.res`" $libs /link /IMPLIB:`"$BuildDirx86\ObjectLibraryFiles\EliteSettings_x86_cpl.lib`" /EXPORT:CPlApplet /MANIFEST:EMBED /MANIFESTINPUT:`"$SourceDir\cpl.manifest`" /MANIFESTUAC:NO"

$everyCompileCmd64 = "cl.exe /FS /EHsc /MTd /D_DEBUG /Fe`"$BuildDir\EliteEverything.exe`" /Fo`"$BuildDir\ObjectFiles/`" `"$SourceDir\EliteEverythingStub.cpp`" `"$BuildDir\ResourceFiles\everything_resources.res`" user32.lib shell32.lib shlwapi.lib /link /MANIFEST:EMBED /MANIFESTINPUT:`"$SourceDir\cpl.manifest`" /MANIFESTUAC:NO"
$everyCompileCmd86 = "cl.exe /FS /EHsc /MTd /D_DEBUG /Fe`"$BuildDirx86\EliteEverything_x86.exe`" /Fo`"$BuildDirx86\ObjectFiles/`" `"$SourceDir\EliteEverythingStub.cpp`" `"$BuildDirx86\ResourceFiles\everything_resources.res`" user32.lib shell32.lib shlwapi.lib /link /MANIFEST:EMBED /MANIFESTINPUT:`"$SourceDir\cpl.manifest`" /MANIFESTUAC:NO"

$dllCompileCmd64 = "cl.exe /FS /EHsc /MTd /D_DEBUG /Fe`"$BuildDir\EliteDLLScanner.exe`" /Fo`"$BuildDir\ObjectFiles/`" `"$SourceDir\EliteDLLScannerStub.cpp`" `"$BuildDir\ResourceFiles\dll_resources.res`" user32.lib shell32.lib shlwapi.lib /link /MANIFEST:EMBED /MANIFESTINPUT:`"$SourceDir\cpl.manifest`" /MANIFESTUAC:NO"
$dllCompileCmd86 = "cl.exe /FS /EHsc /MTd /D_DEBUG /Fe`"$BuildDirx86\EliteDLLScanner_x86.exe`" /Fo`"$BuildDirx86\ObjectFiles/`" `"$SourceDir\EliteDLLScannerStub.cpp`" `"$BuildDirx86\ResourceFiles\dll_resources.res`" user32.lib shell32.lib shlwapi.lib /link /MANIFEST:EMBED /MANIFESTINPUT:`"$SourceDir\cpl.manifest`" /MANIFESTUAC:NO"

Write-Host "Compiling Settings and CPL..."
$ErrorActionPreference = 'Continue'

# --- x64 Build Steps ---
cmd.exe /c "cd /d `"$BuildDir`" && call `"$VsDevCmd`" -arch=x64 && rc.exe /fo `"$BuildDir\ResourceFiles\settings_resources.res`" `"$BuildDir\ResourceFiles\settings_resources.rc`""
if ($LASTEXITCODE -ne 0) { throw "Stubs x64 RC settings_resources failed" }

cmd.exe /c "cd /d `"$BuildDir`" && call `"$VsDevCmd`" -arch=x64 && $stubCompileCmd64"
if ($LASTEXITCODE -ne 0) { throw "Stubs x64 cl EliteSettings.exe failed" }

Start-Sleep -Seconds 2

cmd.exe /c "cd /d `"$BuildDir`" && call `"$VsDevCmd`" -arch=x64 && rc.exe /fo `"$BuildDir\ResourceFiles\settings_cpl.res`" `"$SourceDir\settings_cpl.rc`""
if ($LASTEXITCODE -ne 0) { throw "Stubs x64 RC settings_cpl failed" }

cmd.exe /c "cd /d `"$BuildDir`" && call `"$VsDevCmd`" -arch=x64 && $stubCPLCompileCmd64"
if ($LASTEXITCODE -ne 0) { throw "Stubs x64 cl EliteSettings.cpl failed" }

cmd.exe /c "cd /d `"$BuildDir`" && call `"$VsDevCmd`" -arch=x64 && rc.exe /fo `"$BuildDir\ResourceFiles\everything_resources.res`" `"$SourceDir\EliteEverythingStub.rc`""
if ($LASTEXITCODE -ne 0) { throw "Stubs x64 RC everything_resources failed" }

cmd.exe /c "cd /d `"$BuildDir`" && call `"$VsDevCmd`" -arch=x64 && $everyCompileCmd64"
if ($LASTEXITCODE -ne 0) { throw "Stubs x64 cl EliteEverything.exe failed" }

cmd.exe /c "cd /d `"$BuildDir`" && call `"$VsDevCmd`" -arch=x64 && rc.exe /fo `"$BuildDir\ResourceFiles\dll_resources.res`" `"$SourceDir\EliteDLLScannerStub.rc`""
if ($LASTEXITCODE -ne 0) { throw "Stubs x64 RC dll_resources failed" }

cmd.exe /c "cd /d `"$BuildDir`" && call `"$VsDevCmd`" -arch=x64 && $dllCompileCmd64"
if ($LASTEXITCODE -ne 0) { throw "Stubs x64 cl EliteDLLScanner.exe failed" }

# --- x86 Build Steps ---
cmd.exe /c "cd /d `"$BuildDirx86`" && call `"$VsDevCmd`" -arch=x86 && rc.exe /fo `"$BuildDirx86\ResourceFiles\settings_resources.res`" `"$BuildDirx86\ResourceFiles\settings_resources.rc`""
if ($LASTEXITCODE -ne 0) { throw "Stubs x86 RC settings_resources failed" }

cmd.exe /c "cd /d `"$BuildDirx86`" && call `"$VsDevCmd`" -arch=x86 && $stubCompileCmd86"
if ($LASTEXITCODE -ne 0) { throw "Stubs x86 cl EliteSettings_x86.exe failed" }

Start-Sleep -Seconds 2

cmd.exe /c "cd /d `"$BuildDirx86`" && call `"$VsDevCmd`" -arch=x86 && rc.exe /fo `"$BuildDirx86\ResourceFiles\settings_cpl.res`" `"$SourceDir\settings_cpl_x86.rc`""
if ($LASTEXITCODE -ne 0) { throw "Stubs x86 RC settings_cpl failed" }

cmd.exe /c "cd /d `"$BuildDirx86`" && call `"$VsDevCmd`" -arch=x86 && $stubCPLCompileCmd86"
if ($LASTEXITCODE -ne 0) { throw "Stubs x86 cl EliteSettings_x86.cpl failed" }

cmd.exe /c "cd /d `"$BuildDirx86`" && call `"$VsDevCmd`" -arch=x86 && rc.exe /fo `"$BuildDirx86\ResourceFiles\everything_resources.res`" `"$SourceDir\EliteEverythingStub.rc`""
if ($LASTEXITCODE -ne 0) { throw "Stubs x86 RC everything_resources failed" }

cmd.exe /c "cd /d `"$BuildDirx86`" && call `"$VsDevCmd`" -arch=x86 && $everyCompileCmd86"
if ($LASTEXITCODE -ne 0) { throw "Stubs x86 cl EliteEverything_x86.exe failed" }

cmd.exe /c "cd /d `"$BuildDirx86`" && call `"$VsDevCmd`" -arch=x86 && rc.exe /fo `"$BuildDirx86\ResourceFiles\dll_resources.res`" `"$SourceDir\EliteDLLScannerStub.rc`""
if ($LASTEXITCODE -ne 0) { throw "Stubs x86 RC dll_resources failed" }

cmd.exe /c "cd /d `"$BuildDirx86`" && call `"$VsDevCmd`" -arch=x86 && $dllCompileCmd86"
$ErrorActionPreference = 'Stop'
if ($LASTEXITCODE -ne 0) { throw "Stubs x86 cl EliteDLLScanner_x86.exe failed" }

Write-Host "Settings Build finished successfully."


