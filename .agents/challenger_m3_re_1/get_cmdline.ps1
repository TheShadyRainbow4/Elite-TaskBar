# get_cmdline.ps1
$ScriptDir = "C:\Users\Administrator\Desktop\Elite-TaskBar"
Get-Process -Name EliteSettings, control -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 1
$cplPath = Join-Path $ScriptDir "EliteSettings.cpl"
$procControl = Start-Process -FilePath "control.exe" -ArgumentList "`"$cplPath`"" -PassThru
Start-Sleep -Seconds 3
$w = Get-Process | Where-Object { $_.MainWindowTitle -like "*Properties*" } | Where-Object { $_.Name -like "EST*" }
if ($w) {
    $cmdline = (Get-CimInstance Win32_Process -Filter "ProcessId = $($w.Id)").CommandLine
    Write-Host "Command Line of EST process: $cmdline" -ForegroundColor Green
    Stop-Process -Id $w.Id -Force
} else {
    Write-Host "EST process not found!" -ForegroundColor Red
}
Stop-Process -Id $procControl.Id -Force -ErrorAction SilentlyContinue
