param([string]$QtRoot = "$PSScriptRoot/../.tools/qt/6.8.3/msvc2022_64", [string]$NsisCompiler)
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path "$PSScriptRoot/..").Path
$terloxVersion = [regex]::Match((Get-Content -LiteralPath "$projectRoot/CMakeLists.txt" -Raw),'project\(Terlox VERSION ([0-9.]+)').Groups[1].Value
$packageDir = Join-Path $projectRoot "build-release/package/windows-x64-$terloxVersion"
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$installation = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if ($installation) {
    Import-Module (Join-Path $installation 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll')
    Enter-VsDevShell -VsInstallPath $installation -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64' | Out-Null
}
# This is a generated output directory; check its exact resolved target before replacing it.
if (Test-Path -LiteralPath $packageDir) {
    $resolvedPackage = (Resolve-Path -LiteralPath $packageDir).Path
    if ($resolvedPackage -ne "$projectRoot\build-release\package\windows-x64-$terloxVersion") { throw 'Unexpected package output path' }
    Remove-Item -LiteralPath $resolvedPackage -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $packageDir | Out-Null
foreach ($exe in @('terlox.exe','terloxsh.exe','terloxctl.exe','terlox-maintenance.exe','terlox-setup.exe','terlox-setup-wizard.exe',"terlox-handoff-$terloxVersion.dll")) { Copy-Item -LiteralPath (Join-Path $projectRoot "build-release/bin/$exe") -Destination $packageDir }
& python "$PSScriptRoot/Deploy-Conpty.py" --destination $packageDir
if ($LASTEXITCODE) { throw 'ConPTY runtime deployment failed' }
& "$QtRoot/bin/windeployqt.exe" --release --no-translations --no-opengl-sw --compiler-runtime --skip-plugin-types generic,networkinformation,tls,qmltooling "$packageDir/terlox.exe"
if ($LASTEXITCODE) { throw 'Qt deployment failed' }
# Include the official headless platform plugin for release-runtime checks.
Copy-Item -LiteralPath "$QtRoot/plugins/platforms/qoffscreen.dll" -Destination "$packageDir/platforms/qoffscreen.dll"
New-Item -ItemType Directory -Force -Path "$packageDir/tls" | Out-Null
Copy-Item -LiteralPath "$QtRoot/plugins/tls/qschannelbackend.dll" -Destination "$packageDir/tls/qschannelbackend.dll"
# App-local MSVC runtime lets the folder run without installing the redistributable.
if ($env:VCToolsRedistDir) {
    $crt = Get-ChildItem -Path "$env:VCToolsRedistDir/x64/Microsoft.VC*.CRT" -Directory | Select-Object -First 1
    if ($crt) { Get-ChildItem -LiteralPath $crt.FullName -Filter '*.dll' | Copy-Item -Destination $packageDir }
}
Copy-Item -LiteralPath "$projectRoot/LICENSE","$projectRoot/NOTICE","$projectRoot/README.md","$projectRoot/CONTRIBUTING.md" -Destination $packageDir
Copy-Item -LiteralPath "$projectRoot/docs" -Destination $packageDir -Recurse
New-Item -ItemType Directory -Force -Path "$packageDir/integration/windows","$packageDir/examples" | Out-Null
Get-ChildItem -LiteralPath "$projectRoot/integration/windows" -Filter '*.ps1' -File | Copy-Item -Destination "$packageDir/integration/windows"
Get-ChildItem -LiteralPath "$projectRoot/examples" -File | Copy-Item -Destination "$packageDir/examples"
$licenseDir = Join-Path $packageDir 'licenses'
New-Item -ItemType Directory -Force -Path $licenseDir | Out-Null
Copy-Item -LiteralPath "$projectRoot/dependencies/libvterm/LICENSE" -Destination "$licenseDir/libvterm-MIT.txt"
Copy-Item -LiteralPath "$projectRoot/dependencies/fonts/cascadia/LICENSE" -Destination "$licenseDir/Cascadia-OFL-1.1.txt"
Copy-Item -LiteralPath "$projectRoot/dependencies/licenses/Microsoft-Terminal-MIT.txt" -Destination $licenseDir
Copy-Item -LiteralPath "$projectRoot/docs/Dependencies.md" -Destination "$licenseDir/Dependencies.md"
$qtLicenses = Join-Path $projectRoot 'dependencies/licenses/qt'
if (!(Test-Path -LiteralPath "$qtLicenses/LGPL-3.0-only.txt")) { throw 'Missing Qt license notices' }
Copy-Item -LiteralPath $qtLicenses -Destination $licenseDir -Recurse -Force
Get-ChildItem -LiteralPath "$QtRoot/sbom" -Filter '*.spdx' | Copy-Item -Destination $licenseDir
Write-Host "Portable build: $packageDir"
foreach ($requiredFile in @('Qt6Core.dll','Qt6Gui.dll','Qt6Widgets.dll','msvcp140.dll','vcruntime140.dll','vcruntime140_1.dll','platforms/qwindows.dll')) {
    if (!(Test-Path -LiteralPath (Join-Path $packageDir $requiredFile) -PathType Leaf)) { throw "Incomplete runtime deployment: $requiredFile" }
}
$terloxVersion = [regex]::Match((Get-Content -LiteralPath "$projectRoot/CMakeLists.txt" -Raw),'project\(Terlox VERSION ([0-9.]+)').Groups[1].Value
@{application='app.terlox.terminal'; platform='windows'; version=$terloxVersion; executable='terlox.exe'} | ConvertTo-Json | Set-Content -LiteralPath "$packageDir/.terlox-package.json" -Encoding UTF8
if (!$NsisCompiler) {
    $NsisCompiler = (& python "$PSScriptRoot/Fetch-Package-Tools.py" nsis | Select-Object -Last 1)
    if ($LASTEXITCODE) { throw 'NSIS tool download/verification failed' }
}
if (!(Test-Path -LiteralPath $NsisCompiler -PathType Leaf)) { throw 'Supply -NsisCompiler with the path to makensis.exe' }
Copy-Item -LiteralPath (Join-Path (Split-Path $NsisCompiler) 'COPYING') -Destination "$licenseDir/NSIS-COPYING.txt"
$writerDir=Join-Path $projectRoot 'build-release/package-tools'
New-Item -ItemType Directory -Force -Path $writerDir | Out-Null
$writerFile=Join-Path $writerDir 'write-uninstaller.exe'
& $NsisCompiler /V2 "/DPROJECT_ROOT=$projectRoot" "/DPAYLOAD_DIR=$packageDir" "/DWRITER_FILE=$writerFile" "/DVERSION=$terloxVersion" "$projectRoot/installer/windows/Uninstaller.nsi"
if ($LASTEXITCODE) { throw 'Uninstaller compilation failed' }
$writer=Start-Process -FilePath $writerFile -WindowStyle Hidden -Wait -PassThru
if ($writer.ExitCode -ne 0 -or !(Test-Path -LiteralPath "$packageDir/Uninstall.exe" -PathType Leaf)) { throw 'Uninstaller generation failed' }
$releaseDir=Join-Path $projectRoot 'dist'
New-Item -ItemType Directory -Force -Path $releaseDir | Out-Null
$installerFile=Join-Path $releaseDir "Terlox-$terloxVersion-windows-x64-setup.exe"
& $NsisCompiler /V2 "/DPROJECT_ROOT=$projectRoot" "/DPAYLOAD_DIR=$packageDir" "/DOUTPUT_FILE=$installerFile" "/DVERSION=$terloxVersion" "$projectRoot/installer/windows/SelfExtract.nsi"
if ($LASTEXITCODE) { throw 'Self-contained installer compilation failed' }
Write-Host "Installer: $installerFile"
Get-FileHash -LiteralPath $installerFile -Algorithm SHA256
