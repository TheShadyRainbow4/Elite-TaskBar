param(
    [string]$BuildDir = "$PSScriptRoot\BuildOutput",
    [string]$BuildDirx86 = "$PSScriptRoot\BuildOutputx86"
)

$sys32 = [Environment]::GetFolderPath("System")
$sysWow = [Environment]::GetFolderPath("SystemX86")

# Delete legacy EliteSettings.exe and EliteSettings_x86.exe from system destinations - Draftsman-Dan
$subDir1 = "$PSScriptRoot\Win32Explorer_26.0.3.0"
$subDir2 = "$PSScriptRoot\Remaining_Shell\Win32Explorer_26.0.3.0"
$exesToDelete = @(
    (Join-Path $sys32 "EliteSettings.exe"),
    (Join-Path $sysWow "EliteSettings_x86.exe"),
    (Join-Path $subDir1 "EliteSettings.exe"),
    (Join-Path $subDir2 "EliteSettings.exe")
)
foreach ($file in $exesToDelete) {
    if (Test-Path $file) {
        Remove-Item -Path $file -Force -ErrorAction SilentlyContinue
        Write-Host "Actively deleted legacy Settings executable: $file" -ForegroundColor Yellow
    }
}

function Create-HardLink {
    param (
        [string]$SourcePath,
        [string]$DestDir
    )
    $file = Get-Item $SourcePath
    $targetPath = Join-Path $DestDir $file.Name

    # Force quit running instances of this executable
    if ($file.Extension -eq '.exe') {
        Get-Process -Name $file.BaseName -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
        # Add a tiny delay to let the OS release the file lock
        Start-Sleep -Milliseconds 200
    }

    if (Test-Path $targetPath) {
        Remove-Item -Path $targetPath -Force -ErrorAction SilentlyContinue
        if (Test-Path $targetPath) {
            $oldName = $file.BaseName + "_old_" + (Get-Date -Format "yyyyMMddHHmmss") + ".bak"
            Rename-Item -Path $targetPath -NewName $oldName -Force -ErrorAction SilentlyContinue
        }
    }
    
    # Create the hard link
    try {
        New-Item -ItemType HardLink -Path $targetPath -Target $SourcePath -Force -ErrorAction Stop | Out-Null
        Write-Host "Linked $($file.Name) -> $targetPath" -ForegroundColor Green
    } catch {
        Write-Host "Failed to link $($file.Name) to $targetPath : $($_.Exception.Message)" -ForegroundColor Red
    }
}

Write-Host "Deploying hardlinks to System32 and SysWOW64..." -ForegroundColor Cyan

# Mirror x64 builds to System32
if (Test-Path $BuildDir) {
    Get-ChildItem -Path $BuildDir -Filter "*.exe" -ErrorAction SilentlyContinue | Where-Object { $_.Name -ne "EliteSettings.exe" } | ForEach-Object { Create-HardLink -SourcePath $_.FullName -DestDir $sys32 }
    Get-ChildItem -Path $BuildDir -Filter "*.cpl" -ErrorAction SilentlyContinue | ForEach-Object { Create-HardLink -SourcePath $_.FullName -DestDir $sys32 }
    
    # Mirror into Win32Explorer folders
    $subDir1 = "$PSScriptRoot\Win32Explorer_26.0.3.0"
    $subDir2 = "$PSScriptRoot\Remaining_Shell\Win32Explorer_26.0.3.0"
    if (Test-Path $subDir1) {
        Get-ChildItem -Path $BuildDir -Filter "*.exe" -ErrorAction SilentlyContinue | Where-Object { $_.Name -ne "EliteSettings.exe" } | ForEach-Object { Create-HardLink -SourcePath $_.FullName -DestDir $subDir1 }
        Get-ChildItem -Path $BuildDir -Filter "*.cpl" -ErrorAction SilentlyContinue | ForEach-Object { Create-HardLink -SourcePath $_.FullName -DestDir $subDir1 }
    }
    if (Test-Path $subDir2) {
        Get-ChildItem -Path $BuildDir -Filter "*.exe" -ErrorAction SilentlyContinue | Where-Object { $_.Name -ne "EliteSettings.exe" } | ForEach-Object { Create-HardLink -SourcePath $_.FullName -DestDir $subDir2 }
        Get-ChildItem -Path $BuildDir -Filter "*.cpl" -ErrorAction SilentlyContinue | ForEach-Object { Create-HardLink -SourcePath $_.FullName -DestDir $subDir2 }
    }
}

# Mirror x86 builds to SysWOW64
if (Test-Path $BuildDirx86) {
    Get-ChildItem -Path $BuildDirx86 -Filter "*.exe" -ErrorAction SilentlyContinue | Where-Object { $_.Name -ne "EliteSettings_x86.exe" } | ForEach-Object { Create-HardLink -SourcePath $_.FullName -DestDir $sysWow }
    Get-ChildItem -Path $BuildDirx86 -Filter "*.cpl" -ErrorAction SilentlyContinue | ForEach-Object { Create-HardLink -SourcePath $_.FullName -DestDir $sysWow }
}
