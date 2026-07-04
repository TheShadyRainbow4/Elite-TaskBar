$oldName = "Explorer++"
$newName = "Win32Explorer"
$logFile = "rebrand_log.txt"

function Log-Message($msg) {
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $fullMsg = "[$timestamp] $msg"
    Write-Host $fullMsg
    $fullMsg | Out-File -FilePath $logFile -Append
}

"--- Rebranding Log Start ---" | Out-File -FilePath $logFile

# 1. Text replacement in all files (excluding .git and vcpkg)
Log-Message "Starting text replacement in files..."
$filesToEdit = Get-ChildItem -Path . -Recurse -File | Where-Object { 
    $_.FullName -notmatch "\\.git\\" -and $_.FullName -notmatch "\\vcpkg\\" -and $_.Name -ne $logFile -and $_.Name -ne "rebrand.ps1"
}

foreach ($file in $filesToEdit) {
    try {
        $content = Get-Content -Path $file.FullName -Raw -ErrorAction SilentlyContinue
        if ($null -ne $content -and $content -match [regex]::Escape($oldName)) {
            Log-Message "Updating content in: $($file.FullName)"
            $newContent = $content -replace [regex]::Escape($oldName), $newName
            Set-Content -Path $file.FullName -Value $newContent -Encoding UTF8
        }
    } catch {
        Log-Message "WARNING: Could not process file content: $($file.FullName)"
    }
}

# 2. Rename files (bottom-up)
Log-Message "Starting file renaming..."
$filesToRename = Get-ChildItem -Path . -Recurse -File | Where-Object { 
    $_.Name -like "*$oldName*" -and $_.FullName -notmatch "\\.git\\" -and $_.FullName -notmatch "\\vcpkg\\"
} | Sort-Object FullName -Descending

foreach ($file in $filesToRename) {
    try {
        $newNameFile = $file.Name -replace [regex]::Escape($oldName), $newName
        Log-Message "Renaming file: $($file.FullName) -> $newNameFile"
        Rename-Item -Path $file.FullName -NewName $newNameFile
    } catch {
        Log-Message "ERROR: Failed to rename file: $($file.FullName)"
    }
}

# 3. Rename directories (bottom-up)
Log-Message "Starting directory renaming..."
$dirsToRename = Get-ChildItem -Path . -Recurse -Directory | Where-Object { 
    $_.Name -like "*$oldName*" -and $_.FullName -notmatch "\\.git\\" -and $_.FullName -notmatch "\\vcpkg\\"
} | Sort-Object FullName -Descending

foreach ($dir in $dirsToRename) {
    try {
        $newNameDir = $dir.Name -replace [regex]::Escape($oldName), $newName
        Log-Message "Renaming directory: $($dir.FullName) -> $newNameDir"
        Rename-Item -Path $dir.FullName -NewName $newNameDir
    } catch {
        Log-Message "ERROR: Failed to rename directory: $($dir.FullName)"
    }
}

Log-Message "Rebranding complete."

