# DV026 Research Console v3 — Runtime/UI Fixes

## Root cause of the previous stuck state

The browser was calling `/api/dv026/preflight`, but the locally selected `dv026-workbench-runner` was stale and did not contain the `ollama-preflight` command. The endpoint returned an unknown-command error. The old UI did not expose the distinction clearly enough and could leave the operator staring at a checking state.

## Corrective changes

- `start-workbench.sh` now configures and rebuilds `build-dv026` before starting the server.
- Runtime preflight has explicit READY / OFFLINE / ERROR states.
- Browser fetches have hard timeouts.
- Runner errors are shown as diagnostics rather than being presented as an indefinite runtime check.
- Runtime preflight shows four concrete steps: runner, Ollama, model catalog, ready.
- Live campaign start is gated on runtime/model availability and market qualification.
- Campaign total is established from actual available model count × requested seeds.
- Campaign progress reports completed/total, current model, current seed, and ETA.
- The agent panel no longer displays a fabricated round number before a live run.
- The information panel distinguishes baseline agent-visible inputs from optional treatment inputs.
- Human reference language remains explicitly archival/literature-based.

## Verification

- C++ build completed successfully.
- Full registered CTest suite: **45/45 passed**.
- `dv026-workbench-runner ollama-preflight 424242` executes successfully and reports Ollama offline when no Ollama service is present.
- Local HTTP endpoint `/api/dv026/preflight` returns structured `status` and `runner_ok` fields.
- HTML parser and JavaScript syntax checks pass.
