$ErrorActionPreference = 'Stop'
# Remove only the exact per-user entries owned by the registration script.
foreach ($key in @('HKCU:\Software\Microsoft\Windows\CurrentVersion\App Paths\terlox.exe',
    'HKCU:\Software\Classes\Directory\shell\Terlox',
    'HKCU:\Software\Classes\Directory\Background\shell\Terlox')) {
    if (Test-Path -LiteralPath $key) { Remove-Item -LiteralPath $key -Recurse -Force }
}
$shortcut = Join-Path ([Environment]::GetFolderPath('Programs')) 'Terlox.lnk'
if (Test-Path -LiteralPath $shortcut) { Remove-Item -LiteralPath $shortcut }
$desktopShortcut = Join-Path ([Environment]::GetFolderPath('Desktop')) 'Terlox.lnk'
if (Test-Path -LiteralPath $desktopShortcut) { Remove-Item -LiteralPath $desktopShortcut }
