# CoAgentics Q1 — How to build, run, and qualify a market experiment

**Release name:** Q1. **Status:** Engineering qualification in progress, not D2P2-ready.

This guide is for the *protocol-controlled heterogeneous C++ market experiment* in this source package. It uses synthetic market scenarios only. **Never submit CUI or other restricted information to this workflow.**

## 1. Requirements

- CMake 3.20+, C++20 compiler, Python 3.
- Ollama running locally, with at least **two distinct supported model tags** available.
- Sufficient memory to load the selected models.
- No network connection to a hosted AI service is needed when using local Ollama.

Confirm the models with `ollama list`. The exact tags must be present in the C++ preflight catalog; arbitrary Ollama model tags may not be supported by the existing runner.

## 2. Build

```sh
cmake -S . -B build-q1 -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS_DEBUG='-O0 -g0'
cmake --build build-q1 --target dv026-workbench-runner -j 4
```

The CMake configuration includes a fix for static libcurl transitive dependencies when pkg-config is available. A Debug build is recommended for first qualification; use Release after functional qualification.

## 3. Check the deterministic smoke path

```sh
./build-q1/dv026-workbench-runner hetero-campaign 424242 1 --smoke > q1-smoke.json
```

This runs mock transports **only** and checks the basic market execution path. Do not present it as evidence of live AI behavior. In the current environment, this command completed and returned four cells. The smoke report's minimum-model flag is false under the non-smoke model threshold; it is not a live-model qualification result.

## 4. Prepare an approved paired assignment plan

The market plan format is the same as `AV1_HOW_TO_MARKET_EXPERIMENT.md`. For a minimal test, use two model tags, each assigned one control and one intervention cell at the **same seed** (four assignments total). Each assignment requires:

- `scenario_id` and `trial` identifying a matched test;
- `model` containing an exact available Ollama model tag;
- `condition`: `control` or `intervention`;
- `seed`: a nonnegative integer less than 2^64.

The top-level `protocol_id` and `protocol_sha256` identify the approved experiment. The C++ runner currently **records but does not independently verify** the protocol fingerprint against the source protocol file. Keep the approved protocol alongside the plan for independent review.

In the current engine, the intervention changes the public signal from **100** to **70** while the underlying fundamental remains **100**. The assignment's `model` is a *required participant*, not an exclusive model: each market cell runs two models. Do **not** interpret the market outcome as attributable solely to the requested model.

## 5. Run the live market experiment

```sh
python3 tools/av1_execute_market_plan.py \
  plan.json \
  build-q1/dv026-workbench-runner \
  results/q1-pilot-001
```

Use a fresh output directory. The controller stops on failed cells and refuses mock fallback. It creates a plan copy, manifest, per-cell `campaign.json` and `protocol_cell_0.json`, and on full success `q1_evidence_audit.json` and `q1_descriptive_scores.json`.

The workbench heterogeneous campaign can **resume unfinished assignments** after Verify. Completed `cell_evidence` files are never overwritten. The interrupted 84-cell run is preserved even if resume is blocked; see `Q1_RECOVERY_VALIDATION.md`.

## 6. Independently audit and score the result

```sh
python3 tools/q1_audit_market.py results/q1-pilot-001
python3 tools/q1_score_market.py results/q1-pilot-001 --output results/q1-pilot-001/review_scores.json
```

The read-only audit verifies plan hash, completed cells, model identities, matched assignments, archived model responses, trade counts, and evidence SHA-256 fingerprints. Scoring **recalculates realized surplus and average absolute trade-price deviation** using archived trades, assigned private values, and the recorded market fundamental. It does not impute a price deviation for zero-trade cells. The engine's allocative-efficiency number is retained as a **reported** metric, not independently recalculated.

### Qualification interpretation

| Gate | Current evidence |
|---|---|
| CMake configure | Passed in this execution environment |
| Debug C++ build | Passed after static-libcurl link fix |
| Deterministic heterogeneous smoke | Passed; four mock cells |
| Q1 Python audit/scoring tests | Passed; see `Q1_VALIDATION.md` |
| Protocol-driven live Ollama experiment | **Not run here** |
| Paired scientific inference and baseline comparison | **Not validated** |
| Researcher-facing GUI end-to-end | **Not qualified** |

**Do not claim scientific feasibility, model superiority, or D2P2 readiness from these engineering checks.** A real live-model run and preregistered, replicated statistical analysis remain necessary.

## Troubleshooting

- **Build linker errors involving curl/nghttp2/SSL:** Install `pkg-config` and development libraries for libcurl dependencies, then reconfigure. Prefer a system shared libcurl if your environment's static dependencies cannot be resolved.
- **Ollama unavailable:** Start Ollama, then check `ollama list` and the local service.
- **Unknown model:** Match the exact model tag supported by the runner's preflight catalog.
- **Failed cell:** Review `manifest.json` and runner stderr. Use a new results directory after fixing the issue; resume is not supported.
- **Audit failed:** Do not edit archives to force a pass. Re-run the experiment and preserve both the original failure and the new evidence.


## V22 Market Microstructure
The selected market view shows persistent buyer/seller agents around a shared order book. The compact strip reports best ask, spread, fundamental, best bid, and trade count for the selected archived event. The trade tape lists recorded matches only.
