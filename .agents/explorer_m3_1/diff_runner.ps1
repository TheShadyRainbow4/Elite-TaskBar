$files = @('TaskbarWindow.cpp', 'TrayIconScraper.cpp', 'TaskbarProperties.cpp', 'resources.rc', 'resource.h')
foreach ($f in $files) {
    $src = "SourceFiles\$f"
    $win32 = "Win32Explorer_26.0.3.0\App_Source\EliteTaskbar\$f"
    $out = ".agents\explorer_m3_1\diff_$f.patch"
    # git diff returns exit code 1 if differences are found, so we ignore it
    & git diff --no-index -- $src $win32 > $out
    Write-Output "Diffed $f to $out"
}
