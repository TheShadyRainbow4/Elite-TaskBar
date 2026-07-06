Get-Process -Name EliteTaskbar, EliteSettings, Win32Explorer -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Process -FilePath ".\EliteSettings.exe"
Start-Sleep -Seconds 3
Get-Process | Where-Object { $_.Name -like "Elite*" -or $_.Name -like "Win32*" } | Format-Table Id, Name, MainWindowTitle
