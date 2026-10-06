param(
    [ValidateSet('service', 'console', 'gui', 'tests', 'all')][string]$Target = 'service',
    [string]$Compiler = 'C:\msys64\mingw64\bin\g++.exe'
)
$ErrorActionPreference = 'Stop'
if (-not (Test-Path -LiteralPath $Compiler)) { throw 'MSYS2 g++ compiler not found; specify -Compiler.' }
$env:PATH = (Split-Path -Parent $Compiler) + ';' + $env:PATH
$buildDir = Join-Path $PSScriptRoot 'build'
New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
$flags = @('-std=c++17', '-Wall', '-Wextra', '-Wpedantic', '-static')
$targets = if ($Target -eq 'all') { @('service', 'console', 'gui', 'tests') } else { @($Target) }
foreach ($entry in $targets) {
    switch ($entry) {
        'service' { $source = 'src\api\server.cpp'; $binary = 'neo-cube-service.exe'; $extra = @('-lws2_32') }
        'console' { $source = 'experimental\console.cpp'; $binary = 'neo-cube.exe'; $extra = @() }
        'gui' { $source = 'experimental\gui.cpp'; $binary = 'neo-cube-3-phases.exe'; $extra = @('-mwindows', '-lgdiplus') }
        'tests' { $source = 'tests\core-tests.cpp'; $binary = 'neo-cube-tests.exe'; $extra = @() }
    }
    & $Compiler @flags (Join-Path $PSScriptRoot $source) -o (Join-Path $buildDir $binary) @extra
    if ($LASTEXITCODE -ne 0) { throw "Compilation failed: $entry" }
    Write-Output "Built: $(Join-Path $buildDir $binary)"
    if ($entry -eq 'tests') {
        & (Join-Path $buildDir $binary)
        if ($LASTEXITCODE -ne 0) { throw 'Core tests failed.' }
    }
}
