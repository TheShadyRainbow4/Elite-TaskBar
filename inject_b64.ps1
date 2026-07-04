$content = Get-Content SourceFiles\EliteSettings.ps1 -Raw
$b64 = Get-Content orb_base64.ps1 -Raw
$content = $content -replace "(?m)^#region Initialization & Boilerplate", ("$b64`n#region Initialization & Boilerplate")
Set-Content SourceFiles\EliteSettings.ps1 -Value $content
