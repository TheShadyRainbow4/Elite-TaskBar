
param(
    [string]$Configuration = "Release",
    [string]$Platform = "x64"
)

$logFile = "build_log.txt"
function Log-Message($msg) {
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $fullMsg = "[$timestamp] $msg"
    Write-Host $fullMsg
    $fullMsg | Out-File -FilePath $logFile -Append
}

"--- Build Log Start ---" | Out-File -FilePath $logFile

# 1. Locate MSBuild
Log-Message "Locating MSBuild..."
$msbuildPath = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
if (-not (Test-Path $msbuildPath)) {
    Log-Message "ERROR: MSBuild.exe not found at $msbuildPath"
    exit 1
}
Log-Message "Found MSBuild: $msbuildPath"

# 2. Bootstrap vcpkg if needed
$vcpkgRoot = Join-Path (Get-Location) "External_Dependencies\vcpkg"
$vcpkgExe = Join-Path $vcpkgRoot "vcpkg.exe"

if (-not (Test-Path $vcpkgExe)) {
    Log-Message "vcpkg.exe not found. Bootstrapping..."
    $bootstrapScript = Join-Path $vcpkgRoot "bootstrap-vcpkg.bat"
    if (Test-Path $bootstrapScript) {
        Start-Process -FilePath $bootstrapScript -Wait -NoNewWindow
    } else {
        Log-Message "ERROR: vcpkg directory or bootstrap script missing at $vcpkgRoot"
        exit 1
    }
}

# 3. Build Solution
$solutionFile = "Project_Core\Win32Explorer.sln"
Log-Message "Building $solutionFile ($Configuration|$Platform)..."

$msbuildArgs = @(
    $solutionFile,
    "/p:Configuration=$Configuration",
    "/p:Platform=$Platform", "/t:Win32Explorer",
    "/m", # Parallel build
    "/v:minimal"
)

& $msbuildPath $msbuildArgs | Tee-Object -FilePath $logFile -Append

if ($LASTEXITCODE -ne 0) {
    Log-Message "Build FAILED with exit code $LASTEXITCODE."
    exit $LASTEXITCODE
}

# 4. Relocate output to root
Log-Message "Relocating build artifacts to project root..."

# Identify artifact locations based on Platform/Configuration
# MSBuild output paths relative to SolutionDir usually
if ($Platform -eq "x64") {
    $exeDir = Join-Path (Get-Location) "App_Source\x64\$Configuration"
} elseif ($Platform -eq "Win32") {
    $exeDir = Join-Path (Get-Location) "App_Source\$Configuration"
} else {
    $exeDir = Join-Path (Get-Location) "App_Source\$Platform\$Configuration"
}

$root = "C:\Users\Administrator\Desktop\Elite-TaskBar"

# Copy EXE
$exePath = Join-Path $exeDir "Win32Explorer.exe"
if (Test-Path $exePath) {
    # Sign the executable before copying using Elite-EasySigner
    $signerTool = "C:\Users\Administrator\Desktop\Elite-TaskBar\Elite-EasySigner\Elite-EasySigner.exe"
    
    if (Test-Path $signerTool) {
        Log-Message "Signing $exePath using Elite-EasySigner..."
        & $signerTool $exePath | Out-Null
    } else {
        Log-Message "WARNING: Elite-EasySigner not found at $signerTool, skipping sign."
    }

    $targetExe = Join-Path $root "Win32Explorer.exe"
    $targetOld = Join-Path $root "Win32Explorer.old.bak"
    
    if (Test-Path $targetExe) {
        if (Test-Path $targetOld) {
            Remove-Item -Path $targetOld -Force -ErrorAction SilentlyContinue
        }
        Rename-Item -Path $targetExe -NewName "Win32Explorer.old.bak" -Force -ErrorAction SilentlyContinue
    }

    Log-Message "Copying $exePath to $root\Win32Explorer.exe"
    Copy-Item -Path $exePath -Destination $targetExe -Force
} else {
    Log-Message "WARNING: EXE not found at $exePath"
}

Log-Message "Build artifacts successfully relocated to root."
Log-Message "Build SUCCESSFUL."


