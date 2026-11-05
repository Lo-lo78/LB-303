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

$searchTest = Join-Path $build 'LB303SearchTest.exe'
if (-not (Test-Path -LiteralPath $searchTest)) {
    throw "Incremental search test missing: $searchTest"
}
& $searchTest
if ($LASTEXITCODE -ne 0) { throw "Incremental search test failed: $LASTEXITCODE" }
Write-Host "PASS: LB-303 incremental parameter search test"

$freeLfoTest = Join-Path $build 'LB303FreeLfoTest.exe'
if (-not (Test-Path -LiteralPath $freeLfoTest)) {
    throw "Free LFO host-start test missing: $freeLfoTest"
}
& $freeLfoTest
if ($LASTEXITCODE -ne 0) { throw "Free LFO host-start test failed: $LASTEXITCODE" }
Write-Host "PASS: LB-303 host transport Free LFO start test"
