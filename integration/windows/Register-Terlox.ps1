param([string]$ApplicationDirectory = (Join-Path $PSScriptRoot '../..'), [switch]$DesktopShortcut)
$ErrorActionPreference = 'Stop'
$terloxDirectory = (Resolve-Path -LiteralPath $ApplicationDirectory).Path
$terloxExe = Join-Path $terloxDirectory 'terlox.exe'
if (!(Test-Path -LiteralPath $terloxExe -PathType Leaf)) { throw "No packaged Terlox at $terloxExe" }
# Per-user launchers and command access. Default-console selection is a separate
# explicit installer choice or `terloxctl default-terminal set` command.
$appKey = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\App Paths\terlox.exe'
New-Item -Path $appKey -Force | Out-Null
Set-Item -LiteralPath $appKey -Value $terloxExe
New-ItemProperty -LiteralPath $appKey -Name Path -Value $terloxDirectory -PropertyType String -Force | Out-Null
foreach ($location in @('Directory\shell\Terlox','Directory\Background\shell\Terlox')) {
    $key = "HKCU:\Software\Classes\$location"
    New-Item -Path "$key\command" -Force | Out-Null
    Set-Item -LiteralPath $key -Value 'Open in Terlox'
    New-ItemProperty -LiteralPath $key -Name Icon -Value $terloxExe -PropertyType String -Force | Out-Null
    $folder = if ($location -eq 'Directory\shell\Terlox') { '%1' } else { '%V' }
    Set-Item -LiteralPath "$key\command" -Value ('"'+$terloxExe+'" --directory "'+$folder+'\."')
}
$shortcutPath = Join-Path ([Environment]::GetFolderPath('Programs')) 'Terlox.lnk'
$shortcut = (New-Object -ComObject WScript.Shell).CreateShortcut($shortcutPath)
$shortcut.TargetPath = $terloxExe
$shortcut.WorkingDirectory = [Environment]::GetFolderPath('UserProfile')
$shortcut.Description = 'Terlox terminal emulator'
$shortcut.IconLocation = "$terloxExe,0"
$shortcut.Save()
if ($DesktopShortcut) {
    $desktopLink = (New-Object -ComObject WScript.Shell).CreateShortcut((Join-Path ([Environment]::GetFolderPath('Desktop')) 'Terlox.lnk'))
    $desktopLink.TargetPath = $terloxExe
    $desktopLink.IconLocation = "$terloxExe,0"
    $desktopLink.Description = 'Terlox terminal emulator'
    $desktopLink.Save()
}
# App Paths supports Run, but shells search PATH. Keep existing user entries.
$userPath = [Environment]::GetEnvironmentVariable('Path','User')
$entries = @($userPath -split ';' | Where-Object { $_ })
if (!($entries | Where-Object { [Environment]::ExpandEnvironmentVariables($_).TrimEnd('\') -ieq $terloxDirectory.TrimEnd('\') })) {
    [Environment]::SetEnvironmentVariable('Path',(($entries + $terloxDirectory) -join ';'),'User')
}
Write-Host 'Registered Run/Start, Explorer Open in Terlox and command access in new shells.'
