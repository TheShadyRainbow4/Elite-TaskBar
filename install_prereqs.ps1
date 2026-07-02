$ErrorActionPreference = 'Stop'

Write-Host "Bypassing GPO restrictions for WebView2..." -ForegroundColor Cyan
$edgeUpdatePath = "HKLM:\SOFTWARE\Policies\Microsoft\EdgeUpdate"
if (Test-Path $edgeUpdatePath) {
    Remove-ItemProperty -Path $edgeUpdatePath -Name "Install{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}" -Force -ErrorAction SilentlyContinue
    Remove-ItemProperty -Path $edgeUpdatePath -Name "InstallDefault" -Force -ErrorAction SilentlyContinue
    Set-ItemProperty -Path $edgeUpdatePath -Name "Install{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}" -Value 1 -Type DWord -Force -ErrorAction SilentlyContinue
    Set-ItemProperty -Path $edgeUpdatePath -Name "InstallDefault" -Value 1 -Type DWord -Force -ErrorAction SilentlyContinue
}

Write-Host "Downloading Visual Studio Build Tools..." -ForegroundColor Cyan
$installerPath = "$env:TEMP\vs_buildtools.exe"
Invoke-WebRequest -Uri "https://aka.ms/vs/17/release/vs_buildtools.exe" -OutFile $installerPath

Write-Host "Installing Build Tools (this may take a while)..." -ForegroundColor Cyan
$arguments = "--quiet --wait --norestart --nocache `
    --add Microsoft.VisualStudio.Workload.VCTools `
    --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    --add Microsoft.VisualStudio.Workload.ManagedDesktopBuildTools `
    --add Microsoft.Net.Component.4.6.TargetingPack `
    --add Microsoft.VisualStudio.Component.Windows11SDK.22621"

$process = Start-Process -FilePath $installerPath -ArgumentList $arguments -Wait -PassThru

if ($process.ExitCode -eq 0 -or $process.ExitCode -eq 3010) {
    Write-Host "Build tools installed successfully!" -ForegroundColor Green
    if ($process.ExitCode -eq 3010) {
        Write-Warning "A reboot is required to complete the installation."
    }
} else {
    Write-Error "Build tools installation failed with exit code $($process.ExitCode)."
}
