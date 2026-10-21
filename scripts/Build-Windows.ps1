param([ValidateSet('Debug','Release')][string]$Configuration = 'Debug', [string]$QtRoot = "$PSScriptRoot/../.tools/qt/6.8.3/msvc2022_64", [switch]$SkipTests)
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path "$PSScriptRoot/..").Path
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$installation = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$installation) { throw 'Install Visual Studio C++ Build Tools and the Windows SDK.' }
# Import the compiler environment into this process only.
Import-Module (Join-Path $installation 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll')
Enter-VsDevShell -VsInstallPath $installation -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64' | Out-Null
$cmake = Join-Path $installation 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$ninja = Join-Path $installation 'Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe'
$buildDir = Join-Path $projectRoot "build-$($Configuration.ToLower())"
& $cmake -S $projectRoot -B $buildDir -G Ninja "-DCMAKE_BUILD_TYPE=$Configuration" "-DCMAKE_MAKE_PROGRAM=$ninja" "-DCMAKE_PREFIX_PATH=$QtRoot"
if ($LASTEXITCODE) { throw 'CMake configuration failed' }
& $cmake --build $buildDir --parallel 8
if ($LASTEXITCODE) { throw 'Build failed' }
& python "$PSScriptRoot/Deploy-Conpty.py" --destination "$buildDir/bin"
if ($LASTEXITCODE) { throw 'ConPTY runtime deployment failed' }
$env:PATH = "$QtRoot/bin;$env:PATH"
if (!$SkipTests) {
    & (Join-Path (Split-Path $cmake) 'ctest.exe') --test-dir $buildDir --output-on-failure
    if ($LASTEXITCODE) { throw 'Tests failed' }
}
