#!/bin/sh
# Starts the local service on macOS/Linux. The token comes from CHOISYS_LOCAL_API_TOKEN or from the
# external file CHOISYS_ENV_FILE (default: ~/.choisys.env.local). It is never printed or stored here.
set -eu
here=$(cd "$(dirname "$0")" && pwd)
env_file=${CHOISYS_ENV_FILE:-$HOME/.choisys.env.local}

if [ -z "${CHOISYS_LOCAL_API_TOKEN:-}" ]; then
  [ -f "$env_file" ] || { echo "Local service credentials are not configured." >&2; exit 1; }
  count=$(grep -c '^CHOISYS_LOCAL_API_TOKEN=' "$env_file" || true)
  [ "$count" -eq 1 ] || { echo "Expected exactly one local service credential." >&2; exit 1; }
  CHOISYS_LOCAL_API_TOKEN=$(grep '^CHOISYS_LOCAL_API_TOKEN=' "$env_file" | cut -d= -f2-)
  export CHOISYS_LOCAL_API_TOKEN
fi

CHOISYS_LOCAL_LOG_DIR=${CHOISYS_LOCAL_LOG_DIR:-$HOME/.local/state/neo-cube}
export CHOISYS_LOCAL_LOG_DIR
mkdir -p "$CHOISYS_LOCAL_LOG_DIR"

[ -x "$here/build/neo-cube-service" ] || { echo "Build first: make service" >&2; exit 1; }
exec "$here/build/neo-cube-service"
