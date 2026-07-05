$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$ReleaseDir = Join-Path $ScriptDir "Release"
$Output64 = Join-Path $ScriptDir "BuildOutput"
$Output86 = Join-Path $ScriptDir "BuildOutputx86"

if (-not (Test-Path $ReleaseDir)) {
    New-Item -ItemType Directory -Path $ReleaseDir | Out-Null
}

$InstallInstructions = @"
Elite-TaskBar Installation Instructions

1. Extract all files from this ZIP archive.
2. Copy ALL of the extracted .exe and .cpl files into your C:\Windows\System32 directory.
3. If prompted by User Account Control (UAC), click Yes to approve the administrator operation.
4. Open the EliteSettings.cpl from your Control Panel or run EliteTaskbar.exe directly!

Note: Do not run the executables from inside the ZIP file or from your Desktop without moving them to System32 first, as they may rely on native DLL paths.
"@
$InstallInstructionsPath = Join-Path $ReleaseDir "README-INSTALL.txt"
Set-Content -Path $InstallInstructionsPath -Value $InstallInstructions

# Create x64 zip
$x64Dir = Join-Path $ReleaseDir "x64_temp"
if (Test-Path $x64Dir) { Remove-Item $x64Dir -Recurse -Force }
New-Item -ItemType Directory -Path $x64Dir | Out-Null

Get-ChildItem -Path $Output64 -File -Filter "*.exe" | Copy-Item -Destination $x64Dir
Get-ChildItem -Path $Output64 -File -Filter "*.cpl" | Copy-Item -Destination $x64Dir
Copy-Item $InstallInstructionsPath -Destination $x64Dir

$x64Zip = Join-Path $ReleaseDir "Elite-TaskBar_x64.zip"
if (Test-Path $x64Zip) { Remove-Item $x64Zip -Force }
Compress-Archive -Path "$x64Dir\*" -DestinationPath $x64Zip

# Create x86 zip
$x86Dir = Join-Path $ReleaseDir "x86_temp"
if (Test-Path $x86Dir) { Remove-Item $x86Dir -Recurse -Force }
New-Item -ItemType Directory -Path $x86Dir | Out-Null

Get-ChildItem -Path $Output86 -File -Filter "*_x86.exe" | ForEach-Object {
    $newName = $_.Name -replace "_x86.exe$", ".exe"
    Copy-Item $_.FullName -Destination (Join-Path $x86Dir $newName)
}
Get-ChildItem -Path $Output86 -File -Filter "*_x86.cpl" | ForEach-Object {
    $newName = $_.Name -replace "_x86.cpl$", ".cpl"
    Copy-Item $_.FullName -Destination (Join-Path $x86Dir $newName)
}
Copy-Item $InstallInstructionsPath -Destination $x86Dir

$x86Zip = Join-Path $ReleaseDir "Elite-TaskBar_x86.zip"
if (Test-Path $x86Zip) { Remove-Item $x86Zip -Force }
Compress-Archive -Path "$x86Dir\*" -DestinationPath $x86Zip

# Cleanup
Remove-Item $x64Dir -Recurse -Force
Remove-Item $x86Dir -Recurse -Force
Remove-Item $InstallInstructionsPath -Force

Write-Host "Releases packaged successfully into $ReleaseDir" -ForegroundColor Green
