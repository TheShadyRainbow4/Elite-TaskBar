Stop-Process -Name Win32Explorer -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 3

Write-Host "Starting Win32Explorer..."
$proc = Start-Process -FilePath "C:\Users\Administrator\Desktop\Elite-TaskBar\Win32Explorer_26.0.3.0\Win32Explorer.exe" -ArgumentList "C:\Windows" -PassThru
Start-Sleep -Seconds 5

Write-Host "Process Id: $($proc.Id)"
Write-Host "Process HasExited: $($proc.HasExited)"
if ($proc.HasExited) {
    Write-Host "Process ExitCode: $($proc.ExitCode)"
}

$allProcs = Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue
Write-Host "All Win32Explorer processes running: $($allProcs.Count)"
foreach ($p in $allProcs) {
    Write-Host "  PID: $($p.Id), Title: $($p.MainWindowTitle)"
}
