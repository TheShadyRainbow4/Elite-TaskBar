$sb = New-Object System.Text.StringBuilder
[void]$sb.AppendLine('$global:embeddedOrbs = @{')
Get-ChildItem -Path C:\Users\Administrator\Desktop\Elite-TaskBar\Resources\StartOrb -Filter *.png | ForEach-Object {
    $b64 = [Convert]::ToBase64String([System.IO.File]::ReadAllBytes($_.FullName))
    $name = $_.BaseName
    [void]$sb.AppendLine("'$name' = '$b64';")
}
[void]$sb.AppendLine('}')
Set-Content -Path orb_base64.ps1 -Value $sb.ToString()
