# query_running_view.ps1
$ErrorActionPreference = 'Stop'

Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 1

$regSettingsPath = "HKCU:\Software\Win32Explorer\Settings"
if (Test-Path HKCU:\Software\Win32Explorer) {
    Remove-Item -Path HKCU:\Software\Win32Explorer -Recurse -Force -ErrorAction SilentlyContinue | Out-Null
}

if (!(Test-Path $regSettingsPath)) { New-Item -Path $regSettingsPath -Force | Out-Null }
Set-ItemProperty -Path $regSettingsPath -Name "ViewModeGlobal" -Value 5 -Type DWord # Tiles
Set-ItemProperty -Path $regSettingsPath -Name "ConfirmCloseTabs" -Value 0 -Type DWord

$explorerPath = "C:\Users\Administrator\Desktop\Elite-TaskBar\Win32Explorer.exe"
& psexec64 -i 1 -d $explorerPath "C:\Windows"

# Poll for process
$proc = $null
for ($i = 0; $i -lt 20; $i++) {
    $proc = Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue
    if ($null -ne $proc) { break }
    Start-Sleep -Milliseconds 500
}
if ($null -eq $proc) { throw "Failed to start Win32Explorer." }
$targetPid = $proc.Id
Write-Host "Process started with PID: $targetPid"

Start-Sleep -Seconds 4

$pCheck = Get-Process -Id $targetPid -ErrorAction SilentlyContinue
if ($null -eq $pCheck) {
    Write-Host "Process has exited on its own!"
} else {
    Write-Host "Process is still running!"
    $pCheck | Stop-Process -Force
}
