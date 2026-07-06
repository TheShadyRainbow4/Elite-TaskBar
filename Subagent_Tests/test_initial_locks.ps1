$themeDir = 'C:\Users\Administrator\Desktop\Elite-TaskBar\Subagent_Tests\test_theme'
if (!(Test-Path $themeDir)) { New-Item -ItemType Directory -Path $themeDir | Out-Null }
$img1Path = Join-Path $themeDir 'img1.jpg'
$img2Path = Join-Path $themeDir 'img2.jpg'

Add-Type -AssemblyName System.Drawing
$bmp = New-Object System.Drawing.Bitmap(1, 1)
$bmp.Save($img1Path, [System.Drawing.Imaging.ImageFormat]::Jpeg)
$bmp.Save($img2Path, [System.Drawing.Imaging.ImageFormat]::Jpeg)
$bmp.Dispose()

function Is-FileLocked($path) {
    try {
        $file = [System.IO.File]::Open($path, 'Open', 'Write', 'None')
        $file.Close()
        return $false
    } catch {
        return $true
    }
}

Write-Host "img1 locked: $(Is-FileLocked $img1Path)"
Write-Host "img2 locked: $(Is-FileLocked $img2Path)"
