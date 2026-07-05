param(
    [string]$Version = "",
    [string]$Description = ""
)

$ErrorActionPreference = 'Stop'
$ScriptDir = $PSScriptRoot

Write-Host "=========================================" -ForegroundColor Magenta
Write-Host " EliteSoftware - Release Packaging Tool" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Magenta

if ([string]::IsNullOrWhiteSpace($Version)) {
    $Version = Read-Host "Enter Release Version (e.g. v1.5.0)"
}
if ([string]::IsNullOrWhiteSpace($Description)) {
    $Description = Read-Host "Enter Release Description"
}

if (-not $Version.StartsWith("v")) {
    $Version = "v$Version"
}

$ReleaseDir = Join-Path $ScriptDir "Releases"
if (-not (Test-Path $ReleaseDir)) {
    New-Item -ItemType Directory -Path $ReleaseDir | Out-Null
}

$Buildx64 = Join-Path $ScriptDir "BuildOutput"
$Buildx86 = Join-Path $ScriptDir "BuildOutputx86"

if (-not (Test-Path $Buildx64) -or -not (Test-Path $Buildx86)) {
    Write-Error "BuildOutput or BuildOutputx86 missing. Please run build.ps1 first."
    exit 1
}

$x64Zip = Join-Path $ReleaseDir "Elite-TaskBar_x64_$Version.zip"
$x86Zip = Join-Path $ReleaseDir "Elite-TaskBar_x86_$Version.zip"

Write-Host "Compressing x64 binaries..." -ForegroundColor Yellow
if (Test-Path $x64Zip) { Remove-Item $x64Zip -Force }
Compress-Archive -Path "$Buildx64\*" -DestinationPath $x64Zip -Force

Write-Host "Compressing x86 binaries..." -ForegroundColor Yellow
if (Test-Path $x86Zip) { Remove-Item $x86Zip -Force }
Compress-Archive -Path "$Buildx86\*" -DestinationPath $x86Zip -Force

Write-Host "Binaries packaged successfully to $ReleaseDir" -ForegroundColor Green

# Attempt GitHub Release if gh CLI is installed
try {
    $ghPath = Get-Command gh -ErrorAction Stop
    Write-Host "GitHub CLI found. Creating release and uploading assets..." -ForegroundColor Cyan
    
    # Create the release
    & gh release create $Version $x64Zip $x86Zip --title "Elite-TaskBar $Version" --notes $Description
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "GitHub Release $Version created successfully!" -ForegroundColor Green
    } else {
        Write-Warning "Failed to create GitHub release. You can upload the zip files manually."
    }
} catch {
    Write-Warning "GitHub CLI (gh) not found on PATH. Skipping automatic GitHub upload."
    Write-Host "Your release zips are ready in the Releases folder." -ForegroundColor Yellow
}
