param([switch]$Apply)
$ErrorActionPreference='Stop'
$projectRoot=(Resolve-Path -LiteralPath "$PSScriptRoot/..").Path
$candidates=[System.Collections.Generic.List[string]]::new()
# Explicitly named generated outputs only. Never enumerate arbitrary source
# folders for deletion, and never follow a junction out of this workspace.
foreach($relative in @('build','build-debug','dist/windows-x64','aqtinstall.log',
    'examples/__pycache__','scripts/__pycache__','tests/__pycache__','integration/linux/__pycache__')) {
    $candidate=[IO.Path]::GetFullPath((Join-Path $projectRoot $relative))
    if(Test-Path -LiteralPath $candidate) { $candidates.Add($candidate) }
}
$currentBuild=Join-Path $projectRoot 'build-release'
if(Test-Path -LiteralPath $currentBuild) {
    foreach($file in Get-ChildItem -LiteralPath $currentBuild -File) {
        if($file.Extension -in @('.png','.html','.out','.err') -or ($file.Extension -eq '.txt' -and $file.Name -ne 'CMakeCache.txt' -and $file.Name -ne 'install_manifest.txt')) {
            $candidates.Add($file.FullName)
        }
    }
}
$version=[regex]::Match((Get-Content -LiteralPath "$projectRoot/CMakeLists.txt" -Raw),'project\(Terlox VERSION ([0-9.]+)').Groups[1].Value
if(Test-Path -LiteralPath "$projectRoot/dist") {
    foreach($file in Get-ChildItem -LiteralPath "$projectRoot/dist" -File) {
        if($file.Name -match '^(Terlox-|terlox_|linux-x64|macos-x64|macos-arm64)' -and $file.Name -notmatch ([regex]::Escape($version))) {
            $candidates.Add($file.FullName)
        }
    }
}
foreach($candidate in $candidates) {
    $absolute=[IO.Path]::GetFullPath($candidate)
    if(!$absolute.StartsWith($projectRoot+'\',[StringComparison]::OrdinalIgnoreCase)) { throw "Cleanup target escaped workspace: $absolute" }
    $item=Get-Item -LiteralPath $absolute -Force
    if($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Refusing cleanup through a junction/link: $absolute" }
    if($item.PSIsContainer) {
        $link=Get-ChildItem -LiteralPath $absolute -Recurse -Force | Where-Object { $_.Attributes -band [IO.FileAttributes]::ReparsePoint } | Select-Object -First 1
        if($link) { throw "Refusing cleanup of a tree containing a link: $($link.FullName)" }
    }
    if($Apply) { Remove-Item -LiteralPath $absolute -Recurse -Force; Write-Host "Removed $absolute" }
    else { Write-Host "Would remove $absolute" }
}
if(!$Apply) { Write-Host 'Dry run. Use -Apply to remove these generated outputs.' }
