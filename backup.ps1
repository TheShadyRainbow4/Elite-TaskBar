$ErrorActionPreference = 'Stop'
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$BackupDir = Join-Path $ScriptDir "Backups"

if (-not (Test-Path $BackupDir)) {
    New-Item -ItemType Directory -Path $BackupDir | Out-Null
}

# Timestamp format: MMM-DD-YYYY-HH-MM
$timestamp = Get-Date -Format "MMM-dd-yyyy-HH-mm"
$cabFileName = "EliteTaskBar_Backup-$timestamp.cab"
$cabFilePath = Join-Path $BackupDir $cabFileName

Write-Host "Creating CAB backup snapshot: $cabFilePath" -ForegroundColor Cyan

# Prepare DDF (Diamond Directive File) for MakeCab
$ddfPath = Join-Path $env:TEMP "EliteTaskBar_Backup_$timestamp.ddf"

$ddfContent = @(
    ".OPTION EXPLICIT",
    ".Set CabinetNameTemplate=$cabFileName",
    ".Set DiskDirectoryTemplate=$BackupDir",
    ".Set MaxDiskSize=CDROM",
    ".Set Cabinet=on",
    ".Set Compress=on"
)

$filesToBackup = (Get-ChildItem -Path (Join-Path $ScriptDir "SourceFiles") -Recurse -File) + (Get-ChildItem -Path $ScriptDir -File) | Where-Object {
    $_.Extension -notmatch "^\.(obj|pdb|ilk|tlog|idb|iobj|ipch|sdf|res|pch|lib|zip|pdf|exe|cpl|txt|log|cab)$" -and $_.Name -notmatch "_old"
}

foreach ($file in $filesToBackup) {
    # Calculate relative path
    $relativePath = $file.FullName.Substring($ScriptDir.Length + 1)
    $destDir = Split-Path $relativePath -Parent
    
    if ($destDir) {
        $ddfContent += ".Set DestinationDir=$destDir"
    } else {
        $ddfContent += ".Set DestinationDir="
    }
    
    $ddfContent += "`"$($file.FullName)`""
}

Set-Content -Path $ddfPath -Value $ddfContent -Encoding Ascii

# Run MakeCab
Write-Host "Compressing files into CAB format..." -ForegroundColor Cyan
$makeCabOutput = & makecab.exe /F $ddfPath
$makeCabExitCode = $LASTEXITCODE

# Cleanup Temp DDF and default MakeCab report files
Remove-Item -Path $ddfPath -Force -ErrorAction SilentlyContinue
Remove-Item -Path (Join-Path $ScriptDir "setup.rpt") -Force -ErrorAction SilentlyContinue
Remove-Item -Path (Join-Path $ScriptDir "setup.inf") -Force -ErrorAction SilentlyContinue

if ($makeCabExitCode -eq 0 -and (Test-Path $cabFilePath)) {
    Write-Host "Backup completed successfully!" -ForegroundColor Green
} else {
    Write-Warning "Backup failed or CAB file was not created. MakeCab output:`n$makeCabOutput"
}
