$themeDir = 'C:\Users\Administrator\Desktop\Elite-TaskBar\Subagent_Tests\test_theme'
New-Item -ItemType Directory -Path $themeDir -Force | Out-Null
Add-Type -AssemblyName System.Drawing
$bmp = New-Object System.Drawing.Bitmap(1, 1)
$bmp.Save((Join-Path $themeDir 'img1.jpg'), [System.Drawing.Imaging.ImageFormat]::Jpeg)
$bmp.Save((Join-Path $themeDir 'img2.jpg'), [System.Drawing.Imaging.ImageFormat]::Jpeg)
$bmp.Dispose()
Set-ItemProperty -Path 'HKCU:\Software\EliteSoftware\Win32Explorer\Advanced' -Name 'DesktopThemePath' -Value $themeDir -Type String
.\Subagent_Tests\test_slideshow_diag.exe
