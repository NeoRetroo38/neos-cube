param([string]$EnvFile = 'C:\Users\Admin\daemon.codex.env.local')
$ErrorActionPreference = 'Stop'
if (-not $env:CHOISYS_LOCAL_API_TOKEN) {
    if (-not (Test-Path -LiteralPath $EnvFile)) { throw 'Local service credentials are not configured.' }
    $matchesFound = @([IO.File]::ReadAllLines($EnvFile) | Where-Object { $_ -match '^CHOISYS_LOCAL_API_TOKEN=' })
    if ($matchesFound.Count -ne 1) { throw 'Expected exactly one local service credential.' }
    $env:CHOISYS_LOCAL_API_TOKEN = $matchesFound[0].Substring('CHOISYS_LOCAL_API_TOKEN='.Length)
    $matchesFound = $null
}
if (-not $env:CHOISYS_LOCAL_LOG_DIR) {
    $env:CHOISYS_LOCAL_LOG_DIR = Join-Path $env:USERPROFILE 'Documents\Scenarys\logs\neo-cube'
}
New-Item -ItemType Directory -Force -Path $env:CHOISYS_LOCAL_LOG_DIR | Out-Null
$logFile = Join-Path $env:CHOISYS_LOCAL_LOG_DIR 'service.log'
if (-not (Test-Path -LiteralPath $logFile)) { [IO.File]::WriteAllText($logFile, '') }
& (Join-Path $PSScriptRoot 'build\neo-cube-service.exe')
if ($LASTEXITCODE -ne 0) { throw 'Local service exited with an error.' }
