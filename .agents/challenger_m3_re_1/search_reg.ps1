# search_reg.ps1
$paths = @("HKCU:\Software", "HKLM:\Software")
foreach ($path in $paths) {
    if (Test-Path $path) {
        Get-ChildItem -Path $path -Recurse -ErrorAction SilentlyContinue | ForEach-Object {
            try {
                $item = $_
                $props = Get-ItemProperty -Path $item.PSPath -ErrorAction SilentlyContinue
                if ($props) {
                    foreach ($p in $props.PSObject.Properties) {
                        if ($p.Value -and ($p.Value.ToString() -like "*EliteSettings.cpl*" -or $p.Value.ToString() -like "*Win32Explorer_26.0.3.0*")) {
                            Write-Host "$($item.PSPath) -> $($p.Name) = $($p.Value)"
                        }
                    }
                }
            } catch {}
        }
    }
}
