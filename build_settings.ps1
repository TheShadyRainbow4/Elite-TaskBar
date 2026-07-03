param(
    [string]$SourceDir,
    [string]$BuildDir,
    [string]$BuildDirx86,
    [string]$VsDevCmd
)
$ErrorActionPreference = 'Stop'
$stubCompileCmd64 = "cl.exe /EHsc /Zi /MTd /D_DEBUG /Fd`"$BuildDir\settings64.pdb`" /Fe`"$BuildDir\EliteSettings.exe`" `"$SourceDir\EliteSettingsStub.cpp`" `"$BuildDir\stub_resources.res`" user32.lib shell32.lib shlwapi.lib /link"
$stubCompileCmd86 = "cl.exe /EHsc /Zi /MTd /D_DEBUG /Fd`"$BuildDirx86\settings86.pdb`" /Fe`"$BuildDirx86\EliteSettings_x86.exe`" `"$SourceDir\EliteSettingsStub.cpp`" `"$BuildDirx86\stub_resources.res`" user32.lib shell32.lib shlwapi.lib /link"

Write-Host "Compiling Settings Stubs..."
cmd.exe /c "cd /d `"$BuildDir`" && call `"$VsDevCmd`" -arch=x64 && rc.exe /fo `"$BuildDir\stub_resources.res`" `"$SourceDir\EliteSettingsStub.rc`" && $stubCompileCmd64"
if ($LASTEXITCODE -ne 0) { throw "Settings Stub x64 Build failed" }

cmd.exe /c "cd /d `"$BuildDirx86`" && call `"$VsDevCmd`" -arch=x86 && rc.exe /fo `"$BuildDirx86\stub_resources.res`" `"$SourceDir\EliteSettingsStub.rc`" && $stubCompileCmd86"
if ($LASTEXITCODE -ne 0) { throw "Settings Stub x86 Build failed" }

Write-Host "Settings Stub Build finished successfully."
