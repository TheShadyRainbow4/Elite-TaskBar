$proc = Get-Process -Name EliteSettings -ErrorAction SilentlyContinue
if ($null -ne $proc) {
    Write-Host "EliteSettings PID: $($proc.Id)"
    Get-Content windows_output.txt | Where-Object { $_ -match [string]($proc.Id) }
} else {
    Write-Host "EliteSettings is not running."
    Get-Content windows_output.txt | Select-String -Pattern "Properties"
}
