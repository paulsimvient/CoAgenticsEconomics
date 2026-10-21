# Market Lab v31 Integration

## Changes

1. Market Lab is the primary front door at `/` and `/index.html`.
2. The navigation now treats Interface as the general/live view and Research Console as the technical inspection surface.
3. LLM runtime status is granular rather than a binary online/offline label:
   - Ollama offline
   - Ollama connected with no configured models
   - Ollama connected with insufficient models
   - Ollama ready
   - LLM check paused while a campaign runs
   - DV026 runner unavailable (separate from Ollama state)
4. The server directly probes `http://127.0.0.1:11434/api/tags` if the DV026 runner cannot provide preflight, so a missing runner cannot be mistaken for an offline LLM runtime.
5. The Run workspace continues to use `/api/dv026/campaign/cell` for retained model × seed evidence and displays the selected retained turn, market state, action, execution and evidence chain.
6. Runtime details are available from a compact top-bar status popover rather than persistent explanatory text.
7. Existing contextual `?` help remains in place.

## Verification

- JavaScript extracted from `workbench/market_lab.html`: `node --check` passed.
- `live_server.py`: `python3 -m py_compile` passed.
- Clean CMake Release build completed in `build-dv026` from the current source tree.
- CTest: **50/50 tests passed**.
- Fresh `dv026-workbench-runner ollama-preflight 424242` executed successfully and reported Ollama unavailable in this environment without crashing.
- Server smoke check: `/` served Market Lab and `/api/dv026/preflight` returned granular runtime state.
