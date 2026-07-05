# find_cpl_host.ps1
$ScriptDir = "C:\Users\Administrator\Desktop\Elite-TaskBar"

Get-Process -Name control, EliteSettings -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 1

$cplPath = Join-Path $ScriptDir "EliteSettings.cpl"
$procControl = Start-Process -FilePath "control.exe" -ArgumentList "`"$cplPath`"" -PassThru
Start-Sleep -Seconds 4

# Check all processes for EliteSettings module
$found = $false
Get-Process | ForEach-Object {
    try {
        $p = $_
        $cplModule = $p.Modules | Where-Object { $_.ModuleName -like "*EliteSettings*" }
        if ($cplModule) {
            Write-Host "Process '$($p.Name)' (PID $($p.Id)) has loaded CPL from: $($cplModule.FileName)" -ForegroundColor Green
            $found = $true
        }
    } catch {}
}

if (-not $found) {
    Write-Host "No process found with EliteSettings.cpl loaded." -ForegroundColor Red
}

Get-Process -Name control, EliteSettings -ErrorAction SilentlyContinue | Stop-Process -Force
