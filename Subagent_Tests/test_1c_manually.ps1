# test_1c_manually.ps1
$ErrorActionPreference = 'Stop'
$ScriptDir = "C:\Users\Administrator\Desktop\Elite-TaskBar"

function Stop-EliteProcesses {
    Write-Host "Stopping any running EliteTaskbar processes..." -ForegroundColor Gray
    Get-Process -Name EliteTaskbar, EliteSettings, EliteEverything, EliteDLLScanner, Win32Explorer, EST* -ErrorAction SilentlyContinue | Stop-Process -Force
    Start-Sleep -Seconds 2
}

$regPath = "HKCU:\Software\EliteSoftware\Win32Explorer\Advanced"
$themeDir = Join-Path $ScriptDir "Subagent_Tests\test_theme"
if (!(Test-Path $themeDir)) {
    New-Item -ItemType Directory -Path $themeDir | Out-Null
}

Add-Type -AssemblyName System.Drawing
$bmp = New-Object System.Drawing.Bitmap(1, 1)
$img1Path = Join-Path $themeDir "img1.jpg"
$img2Path = Join-Path $themeDir "img2.jpg"
$bmp.Save($img1Path, [System.Drawing.Imaging.ImageFormat]::Jpeg)
$bmp.Save($img2Path, [System.Drawing.Imaging.ImageFormat]::Jpeg)
$bmp.Dispose()

function Is-FileLocked($path) {
    try {
        $file = [System.IO.File]::Open($path, 'Open', 'Write', 'None')
        $file.Close()
        return $false
    } catch {
        return $true
    }
}

Stop-EliteProcesses

Write-Host "Setting registry keys for Test 1C..."
Set-ItemProperty -Path $regPath -Name "DesktopSlideshowInterval" -Value 4294968 -Type DWord
Set-ItemProperty -Path $regPath -Name "DesktopWallpaperEnabled" -Value 1 -Type DWord
Set-ItemProperty -Path $regPath -Name "DesktopSlideshowEnabled" -Value 1 -Type DWord
Set-ItemProperty -Path $regPath -Name "DesktopThemePath" -Value $themeDir -Type String
Set-ItemProperty -Path $regPath -Name "DesktopReplacementEnabled" -Value 1 -Type DWord
Set-ItemProperty -Path $regPath -Name "ForceProgmanAllDisplays" -Value 1 -Type DWord

Write-Host "Starting EliteTaskbar.exe..."
$proc = Start-Process -FilePath (Join-Path $ScriptDir "EliteTaskbar.exe") -ArgumentList "-allowMultiple" -PassThru
Start-Sleep -Seconds 4

Write-Host "Checking initial lock state..."
Write-Host "img1 locked: $(Is-FileLocked $img1Path)"
Write-Host "img2 locked: $(Is-FileLocked $img2Path)"

Write-Host "Monitoring locks for 5 seconds..."
for ($i = 0; $i -lt 10; $i++) {
    Write-Host "Iteration $i : img1=$(Is-FileLocked $img1Path), img2=$(Is-FileLocked $img2Path)"
    Start-Sleep -Milliseconds 500
}

Stop-EliteProcesses
Remove-Item -Path $themeDir -Recurse -Force -ErrorAction SilentlyContinue
