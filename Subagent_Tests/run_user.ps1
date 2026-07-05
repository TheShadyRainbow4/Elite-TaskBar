# run_user.ps1
$ErrorActionPreference = 'Stop'

taskkill /F /IM Win32Explorer.exe 2>$null | Out-Null
Start-Sleep -Seconds 1

$explorerPath = "C:\Users\Administrator\Desktop\Elite-TaskBar\Win32Explorer.exe"
& psexec64 -i 1 -u User -p "" $explorerPath "C:\Windows"

# Poll for process
$proc = $null
for ($i = 0; $i -lt 20; $i++) {
    $proc = Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue
    if ($null -ne $proc) { break }
    Start-Sleep -Milliseconds 500
}
if ($null -eq $proc) { throw "Failed to start Win32Explorer." }
$targetPid = $proc.Id
Write-Host "Started PID: $($targetPid)"

Start-Sleep -Seconds 4

$pCheck = Get-Process -Id $targetPid -ErrorAction SilentlyContinue
if ($null -eq $pCheck) {
    Write-Host "Process is NOT running at query time!"
} else {
    Write-Host "Process IS running at query time!"
    $pCheck | Stop-Process -Force -ErrorAction SilentlyContinue
}
