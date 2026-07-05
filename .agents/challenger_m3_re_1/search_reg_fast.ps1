# search_reg_fast.ps1
$searchPaths = @(
    "HKCU:\Software\EliteSoftware",
    "HKLM:\Software\EliteSoftware",
    "HKCU:\Software\Win32Explorer",
    "HKLM:\Software\Win32Explorer",
    "HKCU:\Software\Microsoft\Windows\CurrentVersion\Control Panel\Cpls",
    "HKLM:\Software\Microsoft\Windows\CurrentVersion\Control Panel\Cpls",
    "HKCU:\Software\Classes\CLSID",
    "HKCU:\Software\Classes\Wow6432Node\CLSID",
    "HKLM:\Software\Classes\CLSID",
    "HKLM:\Software\Classes\Wow6432Node\CLSID"
)

foreach ($root in $searchPaths) {
    if (Test-Path $root) {
        Write-Host "Scanning $root..." -ForegroundColor Cyan
        Get-ChildItem -Path $root -Recurse -ErrorAction SilentlyContinue | ForEach-Object {
            try {
                $item = $_
                $props = Get-ItemProperty -Path $item.PSPath -ErrorAction SilentlyContinue
                if ($props) {
                    foreach ($p in $props.PSObject.Properties) {
                        if ($p.Value -and ($p.Value.ToString() -like "*EliteSettings*" -or $p.Value.ToString() -like "*Win32Explorer_26.0.3.0*")) {
                            Write-Host "FOUND: $($item.PSPath) -> $($p.Name) = $($p.Value)" -ForegroundColor Green
                        }
                    }
                }
            } catch {}
        }
    }
}
Write-Host "Scan finished." -ForegroundColor Cyan
