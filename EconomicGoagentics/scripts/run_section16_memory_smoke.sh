#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build-section16-memory"
rm -rf "$BUILD"
mkdir -p "$BUILD"
c++ -std=c++17 -Wall -Wextra -Wpedantic -I"$ROOT/include" \
  "$ROOT/src/PersistentMemory.cpp" \
  "$ROOT/src/PersistentMemoryAgent.cpp" \
  "$ROOT/tests/persistent_memory_agent_smoke.cpp" \
  -o "$BUILD/persistent_memory_agent_smoke"
"$BUILD/persistent_memory_agent_smoke"
