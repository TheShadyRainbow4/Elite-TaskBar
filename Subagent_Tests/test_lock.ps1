Add-Type -AssemblyName System.Drawing
$themeDir = 'C:\Users\Administrator\Desktop\Elite-TaskBar\Subagent_Tests\test_theme'
$imgPath = Join-Path $themeDir 'img1.jpg'

if (!(Test-Path $imgPath)) {
    New-Item -ItemType Directory -Path $themeDir -Force | Out-Null
    $bmpTmp = New-Object System.Drawing.Bitmap(1, 1)
    $bmpTmp.Save($imgPath, [System.Drawing.Imaging.ImageFormat]::Jpeg)
    $bmpTmp.Dispose()
}

$bmp = New-Object System.Drawing.Bitmap($imgPath)

function Is-FileLocked($path) {
    try {
        $file = [System.IO.File]::Open($path, 'Open', 'Write', 'None')
        $file.Close()
        return $false
    } catch {
        return $true
    }
}

$locked = Is-FileLocked $imgPath
Write-Host "Is img1.jpg locked when loaded by Gdiplus: $locked"
$bmp.Dispose()
