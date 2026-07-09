param(
    [string]$BuildDir = "$PSScriptRoot\BuildOutput",
    [string]$BuildDirx86 = "$PSScriptRoot\BuildOutputx86"
)

$sys32 = [Environment]::GetFolderPath("System")
$sysWow = [Environment]::GetFolderPath("SystemX86")

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
    Get-ChildItem -Path $BuildDir -Filter "*.exe" -ErrorAction SilentlyContinue | ForEach-Object { Create-HardLink -SourcePath $_.FullName -DestDir $sys32 }
    Get-ChildItem -Path $BuildDir -Filter "*.cpl" -ErrorAction SilentlyContinue | ForEach-Object { Create-HardLink -SourcePath $_.FullName -DestDir $sys32 }
}

# Mirror x86 builds to SysWOW64
if (Test-Path $BuildDirx86) {
    Get-ChildItem -Path $BuildDirx86 -Filter "*.exe" -ErrorAction SilentlyContinue | ForEach-Object { Create-HardLink -SourcePath $_.FullName -DestDir $sysWow }
    Get-ChildItem -Path $BuildDirx86 -Filter "*.cpl" -ErrorAction SilentlyContinue | ForEach-Object { Create-HardLink -SourcePath $_.FullName -DestDir $sysWow }
}
