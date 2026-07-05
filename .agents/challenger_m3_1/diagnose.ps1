# diagnose.ps1
$content = Get-Content .\verify_m3_runtime.ps1 -Raw
$matches = [regex]::Match($content, '(?s)\$code\s*=\s*@"\s*(.*?)\s*"@')
if ($matches.Success) {
    $csharpCode = $matches.Groups[1].Value
    Write-Host "Compiling C# code..."
    try {
        Add-Type -TypeDefinition $csharpCode
        Write-Host "Compiled successfully!"
    } catch {
        Write-Error $_
    }
} else {
    Write-Host "No match found"
}
