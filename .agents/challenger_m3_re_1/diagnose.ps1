# diagnose.ps1
$ScriptDir = "C:\Users\Administrator\Desktop\Elite-TaskBar"

# Kill existing settings instances
Get-Process -Name EliteSettings -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 1

# Launch CPL
$cplPath = Join-Path $ScriptDir "EliteSettings.cpl"
$procControl = Start-Process -FilePath "control.exe" -ArgumentList "`"$cplPath`"" -PassThru
Start-Sleep -Seconds 3

# Find settings process
$w = Get-Process | Where-Object { $_.MainWindowTitle -like "*Properties*" } | Where-Object { $_.Name -like "EST*" }
if ($w) {
    Write-Host "Found Settings Process PID: $($w.Id)" -ForegroundColor Green
    $cmdline = (Get-CimInstance Win32_Process -Filter "ProcessId = $($w.Id)").CommandLine
    Write-Host "Command Line: $cmdline" -ForegroundColor Yellow
    
    Write-Host "Loaded Modules:" -ForegroundColor Cyan
    $w.Modules | Select-Object FileName, ModuleName | Format-Table -AutoSize
    
    Stop-Process -Id $w.Id -Force
} else {
    Write-Host "Settings process not found." -ForegroundColor Red
}
Stop-Process -Id $procControl.Id -Force -ErrorAction SilentlyContinue
