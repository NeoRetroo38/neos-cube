$ErrorActionPreference = 'Stop'
$localSecret = 'C:\Users\Admin\daemon.codex.env.local'
$lines = @([IO.File]::ReadAllLines($localSecret) | Where-Object { $_ -match '^CHOISYS_LOCAL_API_TOKEN=' })
if ($lines.Count -ne 1) { throw 'Local credentials unavailable.' }
$authHeaders = @{ Authorization = 'Bearer ' + $lines[0].Substring('CHOISYS_LOCAL_API_TOKEN='.Length) }
$lines = $null
$baseUrl = 'http://127.0.0.1:8765'
function Expect-Rejection([string]$Path, [int]$Status, [hashtable]$Headers, [string]$Body = '') {
    try {
        if ($Body) { $null = Invoke-WebRequest "$baseUrl$Path" -UseBasicParsing -Method Post -Headers $Headers -ContentType 'application/json' -Body $Body }
        else { $null = Invoke-WebRequest "$baseUrl$Path" -UseBasicParsing -Headers $Headers }
        throw 'Unexpected successful response.'
    } catch {
        if ([int]$_.Exception.Response.StatusCode -ne $Status) { throw }
    }
}
$health = Invoke-RestMethod "$baseUrl/health" -Headers $authHeaders
if (-not $health.ok -or $health.service -ne 'neo-cube' -or $health.version -ne '0.1.0') { throw 'Health contract failed.' }
Expect-Rejection '/health' 401 @{}
Expect-Rejection '/health' 401 @{ Authorization = 'Bearer incorrect' }
Expect-Rejection '/evaluate' 400 $authHeaders '{}'
$id = [guid]::NewGuid().ToString()
foreach ($phase in 1..3) {
    $request = @{ scenarioId = 'choice-grid'; sessionId = $id; phase = $phase; decisions = @(@{ position = $phase; selected = $true; value = 1 }) }
    $body = $request | ConvertTo-Json -Depth 4 -Compress
    $reply = Invoke-RestMethod "$baseUrl/evaluate" -Headers $authHeaders -Method Post -ContentType 'application/json' -Body $body
    $retry = Invoke-RestMethod "$baseUrl/evaluate" -Headers $authHeaders -Method Post -ContentType 'application/json' -Body $body
    if (-not $reply.ok -or $reply.result.phase -ne $phase -or ($reply | ConvertTo-Json -Compress) -ne ($retry | ConvertTo-Json -Compress)) { throw 'Phase/retry failed.' }
    if ($phase -lt 3 -and ($reply.result.status -ne 'phase-complete' -or $reply.result.nextPhase -ne $phase + 1)) { throw 'Progression failed.' }
    if ($phase -eq 3 -and ($reply.result.status -ne 'completed' -or $null -ne $reply.result.nextPhase)) { throw 'Completion failed.' }
    $request.decisions[0].position = 9
    Expect-Rejection '/evaluate' 409 $authHeaders ($request | ConvertTo-Json -Depth 4 -Compress)
}
$listeners = @(Get-NetTCPConnection -LocalPort 8765 -State Listen)
if ($listeners.Count -ne 1 -or $listeners[0].LocalAddress -ne '127.0.0.1') { throw 'Unsafe listener.' }
Write-Output 'PASS: authenticated health; missing/wrong auth; invalid request; three phases; retry/conflict; loopback only.'
