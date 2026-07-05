# run_test_viewmode.ps1
$ErrorActionPreference = 'Stop'

Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 1

$regSettingsPath = "HKCU:\Software\Win32Explorer\Settings"
if (Test-Path HKCU:\Software\Win32Explorer) {
    Remove-Item -Path HKCU:\Software\Win32Explorer -Recurse -Force -ErrorAction SilentlyContinue | Out-Null
}

if (!(Test-Path $regSettingsPath)) { New-Item -Path $regSettingsPath -Force | Out-Null }
Set-ItemProperty -Path $regSettingsPath -Name "ViewModeGlobal" -Value 12 -Type DWord
Set-ItemProperty -Path $regSettingsPath -Name "ShowInGroupsGlobal" -Value 0 -Type DWord
Set-ItemProperty -Path $regSettingsPath -Name "ConfirmCloseTabs" -Value 0 -Type DWord
Set-ItemProperty -Path $regSettingsPath -Name "EnablePortableMirror" -Value 0 -Type DWord
Set-ItemProperty -Path $regSettingsPath -Name "EnableShellBagsSupport" -Value 0 -Type DWord

$explorerPath = "C:\Users\Administrator\Desktop\Elite-TaskBar\Win32Explorer.exe"
& psexec64 -i 1 -d $explorerPath "C:\Windows"
Start-Sleep -Seconds 3

$proc = Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue
if ($null -eq $proc) {
    Write-Host "Process exited immediately when registry is cleared."
} else {
    Write-Host "Process is still running! PID: $($proc.Id)"
    $proc | Stop-Process -Force
}
