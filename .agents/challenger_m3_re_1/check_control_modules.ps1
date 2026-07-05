# check_control_modules.ps1
$ScriptDir = "C:\Users\Administrator\Desktop\Elite-TaskBar"

Get-Process -Name control, EliteSettings -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 1

$cplPath = Join-Path $ScriptDir "EliteSettings.cpl"
$procControl = Start-Process -FilePath "control.exe" -ArgumentList "`"$cplPath`"" -PassThru
Start-Sleep -Seconds 3

# List modules loaded by control.exe
$proc = Get-Process -Id $procControl.Id -ErrorAction SilentlyContinue
if ($proc) {
    Write-Host "Found control.exe process PID: $($proc.Id)" -ForegroundColor Green
    $cplModule = $proc.Modules | Where-Object { $_.ModuleName -like "*EliteSettings*" }
    if ($cplModule) {
        Write-Host "EliteSettings CPL Module loaded from: $($cplModule.FileName)" -ForegroundColor Green
    } else {
        Write-Host "EliteSettings.cpl not found in loaded modules of control.exe." -ForegroundColor Red
        Write-Host "All Modules:" -ForegroundColor Yellow
        $proc.Modules | Select-Object FileName, ModuleName | Format-Table -AutoSize
    }
} else {
    Write-Host "control.exe process not found." -ForegroundColor Red
}

Get-Process -Name control, EliteSettings -ErrorAction SilentlyContinue | Stop-Process -Force
