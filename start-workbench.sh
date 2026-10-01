#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

BUILD_DIR="build-dv026"

echo "Configuring CoAgentics engines in ${BUILD_DIR}…"
cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON >/dev/null

echo "Building workbench runners…"
cmake --build "$BUILD_DIR" -j8 --target v27-live-runner dv026-workbench-runner

# Prevent an old server from causing a confusing port-8787 failure.
if command -v lsof >/dev/null 2>&1; then
  PIDS="$(lsof -tiTCP:8787 -sTCP:LISTEN 2>/dev/null || true)"
  if [[ -n "$PIDS" ]]; then
    echo "Port 8787 is in use by PID(s): $PIDS"
    echo "Stop the existing workbench first, or run: ./start-workbench.sh 8788"
    exit 2
  fi
fi

PORT="${1:-8787}"
echo "EconomicGoagentics DV026 Research Console: http://127.0.0.1:${PORT}"
python3 live_server.py "$PORT"
