$ErrorActionPreference = 'Stop'

$vswhere = 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere)) {
    throw 'Visual Studio Installer was not found.'
}
$visualStudio = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $visualStudio) {
    throw 'Visual Studio C++ x64 toolchain was not found.'
}
$devCommand = Join-Path $visualStudio 'Common7\Tools\VsDevCmd.bat'
$root = Split-Path -Parent $PSScriptRoot
$build = Join-Path $root 'build\nmake-release'
$line = '"{0}" -arch=x64 && cmake -S "{1}" -B "{2}" -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release && cmake --build "{2}"' -f $devCommand, $root, $build
& cmd.exe /d /s /c $line
if ($LASTEXITCODE -ne 0) { throw "LB-303 compilation failed: $LASTEXITCODE" }
Write-Host "Built: $build\LB303_artefacts\Release\VST3\LB-303.vst3"
