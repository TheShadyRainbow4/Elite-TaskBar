$ErrorActionPreference = 'Stop'
$ScriptDir = "C:\Users\Administrator\Desktop\Elite-TaskBar"

function Stop-EliteProcesses {
    Get-Process -Name EliteTaskbar, EST* -ErrorAction SilentlyContinue | Stop-Process -Force
    Start-Sleep -Seconds 1
}

Stop-EliteProcesses

# Setup registry
$regPath = "HKCU:\Software\EliteSoftware\Win32Explorer\Advanced"
Set-ItemProperty -Path $regPath -Name "DesktopWallpaperEnabled" -Value 1 -Type DWord
Set-ItemProperty -Path $regPath -Name "DesktopSlideshowEnabled" -Value 1 -Type DWord
Set-ItemProperty -Path $regPath -Name "DesktopSlideshowInterval" -Value 3 -Type DWord
Set-ItemProperty -Path $regPath -Name "DesktopThemePath" -Value "C:\Users\Administrator\Desktop\Elite-TaskBar\Subagent_Tests\test_theme" -Type String
Set-ItemProperty -Path $regPath -Name "DesktopReplacementEnabled" -Value 1 -Type DWord
Set-ItemProperty -Path $regPath -Name "ForceProgmanAllDisplays" -Value 1 -Type DWord

Write-Host "Launching EliteTaskbar..."
$proc = Start-Process -FilePath (Join-Path $ScriptDir "EliteTaskbar.exe") -ArgumentList "-allowMultiple" -PassThru
Start-Sleep -Seconds 4

Write-Host "Checking lock owners..."
.\Subagent_Tests\get_file_lock_owner.exe

Stop-EliteProcesses
