#!/bin/sh
# End-to-end check of the service on macOS/Linux; mirrors tests/smoke.ps1.
# Starts its own service with a throwaway token and log directory, then stops it.
set -eu
here=$(cd "$(dirname "$0")/.." && pwd)
service="$here/build/neo-cube-service"
[ -x "$service" ] || { echo "Build first: make service" >&2; exit 1; }

work=$(mktemp -d)
token=$(openssl rand -hex 32)
CHOISYS_LOCAL_API_TOKEN=$token CHOISYS_LOCAL_LOG_DIR=$work "$service" >"$work/out.txt" 2>&1 &
pid=$!
trap 'kill "$pid" 2>/dev/null || true; rm -rf "$work"' EXIT

base=http://127.0.0.1:8765
auth="Authorization: Bearer $token"

i=0
until curl -s -o /dev/null -H "$auth" "$base/health"; do
  i=$((i + 1)); [ "$i" -lt 50 ] || { echo "FAIL: service did not start"; cat "$work/out.txt"; exit 1; }
  sleep 0.1
done

fail() { echo "FAIL: $1"; exit 1; }
code() { curl -s -o /dev/null -w '%{http_code}' "$@"; }

health=$(curl -s -H "$auth" "$base/health")
[ "$health" = '{"ok":true,"service":"neo-cube","version":"0.1.0"}' ] || fail "health contract"
[ "$(code "$base/health")" = 401 ] || fail "missing auth must be 401"
[ "$(code -H 'Authorization: Bearer incorrect' "$base/health")" = 401 ] || fail "wrong auth must be 401"
[ "$(code -H "$auth" -H 'Content-Type: application/json' -d '{}' "$base/evaluate")" = 400 ] || fail "invalid request must be 400"

id=$(openssl rand -hex 16 | sed -E 's/^(.{8})(.{4})(.{4})(.{4})(.{12})$/\1-\2-\3-\4-\5/')
for phase in 1 2 3; do
  body="{\"scenarioId\":\"choice-grid\",\"sessionId\":\"$id\",\"phase\":$phase,\"decisions\":[{\"position\":$phase,\"selected\":true,\"value\":1}]}"
  reply=$(curl -s -H "$auth" -H 'Content-Type: application/json' -d "$body" "$base/evaluate")
  retry=$(curl -s -H "$auth" -H 'Content-Type: application/json' -d "$body" "$base/evaluate")
  [ "$reply" = "$retry" ] || fail "retry must be idempotent (phase $phase)"
  case "$reply" in *'"ok":true'*) ;; *) fail "phase $phase rejected" ;; esac
  if [ "$phase" -lt 3 ]; then
    case "$reply" in *'"status":"phase-complete"'*) ;; *) fail "phase $phase should be phase-complete" ;; esac
  else
    case "$reply" in *'"status":"completed"'*'"measurements":[{"phase":1,"row":1,"column":1},{"phase":2,"row":1,"column":2},{"phase":3,"row":1,"column":3}]'*) ;; *) fail "completion must include measurements" ;; esac
  fi
  conflict="{\"scenarioId\":\"choice-grid\",\"sessionId\":\"$id\",\"phase\":$phase,\"decisions\":[{\"position\":9,\"selected\":true,\"value\":1}]}"
  [ "$(code -H "$auth" -H 'Content-Type: application/json' -d "$conflict" "$base/evaluate")" = 409 ] || fail "changed selection must be 409"
done

echo "PASS: authenticated health; missing/wrong auth; invalid request; three phases with measurements; retry/conflict."
