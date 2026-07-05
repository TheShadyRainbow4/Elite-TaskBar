# run_direct.ps1
$events = Get-WinEvent -FilterHashtable @{LogName='Application'} -MaxEvents 50
$filtered = $events | Where-Object { $_.Message -like "*Win32Explorer*" }
if ($filtered) {
    foreach ($e in $filtered) {
        Write-Host "Time: $($e.TimeCreated) | ID: $($e.Id)"
        Write-Host "Message: $($e.Message)"
        Write-Host "------------------------------------"
    }
} else {
    Write-Host "No event log entries found matching Win32Explorer."
}
