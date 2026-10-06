#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

BUILD_DIR="build-dv026"
PORT="${1:-8787}"

# CMake caches the compiler and SDK. Xcode/Command Line Tools upgrades can leave
# those cached paths invalid. Detect that state and reconfigure cleanly instead
# of silently using an old runner or failing with a stale compiler path.
if [[ -f "$BUILD_DIR/CMakeCache.txt" ]]; then
  CACHED_CXX="$(sed -n 's/^CMAKE_CXX_COMPILER:FILEPATH=//p' "$BUILD_DIR/CMakeCache.txt" | head -1)"
  CACHED_SYSROOT="$(sed -n 's/^CMAKE_OSX_SYSROOT:PATH=//p' "$BUILD_DIR/CMakeCache.txt" | head -1)"
  STALE=0
  if [[ -n "$CACHED_CXX" && ! -x "$CACHED_CXX" ]]; then
    echo "Cached C++ compiler no longer exists: $CACHED_CXX"
    STALE=1
  fi
  if [[ -n "$CACHED_SYSROOT" && "$CACHED_SYSROOT" = /* && ! -d "$CACHED_SYSROOT" ]]; then
    echo "Cached macOS SDK no longer exists: $CACHED_SYSROOT"
    STALE=1
  fi
  if [[ "$STALE" -eq 1 ]]; then
    echo "Removing stale CMake cache in ${BUILD_DIR}…"
    rm -rf "$BUILD_DIR"
  fi
fi

# On macOS prefer the currently selected Apple toolchain, but do not override
# an explicit user CC/CXX. This also avoids inheriting stale SDKROOT values.
if [[ "$(uname -s)" == "Darwin" ]]; then
  unset SDKROOT CMAKE_OSX_SYSROOT || true
  if [[ -z "${CXX:-}" ]] && command -v xcrun >/dev/null 2>&1; then
    CURRENT_CXX="$(xcrun --find c++ 2>/dev/null || true)"
    if [[ -n "$CURRENT_CXX" ]]; then export CXX="$CURRENT_CXX"; fi
  fi
  if [[ -z "${CC:-}" ]] && command -v xcrun >/dev/null 2>&1; then
    CURRENT_CC="$(xcrun --find cc 2>/dev/null || true)"
    if [[ -n "$CURRENT_CC" ]]; then export CC="$CURRENT_CC"; fi
  fi
fi

echo "Configuring CoAgentics engines in ${BUILD_DIR}…"
cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON

echo "Building workbench runners…"
cmake --build "$BUILD_DIR" -j8 --target v27-live-runner dv026-workbench-runner

# Prevent an existing server from causing a confusing bind failure.
if command -v lsof >/dev/null 2>&1; then
  PIDS="$(lsof -tiTCP:"$PORT" -sTCP:LISTEN 2>/dev/null || true)"
  if [[ -n "$PIDS" ]]; then
    echo "Port ${PORT} is in use by PID(s): $PIDS"
    echo "Stop the existing workbench first, or choose another port: ./start-workbench.sh 8788"
    exit 2
  fi
fi

echo "EconomicGoagentics"
echo "  Interface (primary): http://127.0.0.1:${PORT}/"
echo "  Research Console:    http://127.0.0.1:${PORT}/live.html"
echo "  Market Lab:          http://127.0.0.1:${PORT}/market_lab.html"
python3 live_server.py "$PORT"
