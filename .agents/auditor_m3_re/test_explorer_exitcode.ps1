Get-Process -Name Win32Explorer, EliteTaskbar -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 3
$explorerPath = "C:\Users\Administrator\Desktop\Elite-TaskBar\Win32Explorer.exe"
$p = Start-Process -FilePath $explorerPath -PassThru
$p.WaitForExit()
Write-Host "Exit Code: $($p.ExitCode)"
