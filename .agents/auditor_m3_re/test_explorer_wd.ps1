Get-Process -Name Win32Explorer, EliteTaskbar -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 3
$explorerPath = "C:\Users\Administrator\Desktop\Elite-TaskBar\Win32Explorer.exe"
$p = Start-Process -FilePath $explorerPath -WorkingDirectory "C:\Users\Administrator\Desktop\Elite-TaskBar" -PassThru
Start-Sleep -Seconds 6
$proc = Get-Process -Id $p.Id -ErrorAction SilentlyContinue
if ($null -ne $proc) {
    Write-Host "SUCCESS: Win32Explorer is running with PID: $($p.Id)"
    Stop-Process -Id $p.Id -Force
} else {
    Write-Host "FAILURE: Win32Explorer did not stay running. Exit Code: $($p.ExitCode)"
}
