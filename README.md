# CoAgentics Behavior Lab — DV026 v5

A deliberately stripped CoAgentics baseline for DARPA DPA26BZ06-DV026, **Influence Benchmarks for AI Systems**.

## What remains
- deterministic experimental core
- market/auction domain primitive
- agent-policy adapter
- paired intervention statistics
- epistemic contract schemas retained from CoAgentics
- an intentionally small ABM population/neighborhood extension point for later Axtell-style heterogeneous/local-interaction experiments

## What is intentionally absent
Mission maps, DCA/night-crossing, maritime/UAS, orbital/spaceport, mission companion, COA workflow, living-city traffic, morphology/organism experiments, RAG/RQA mission plugins, Cesium/MapLibre, and legacy release/version documentation.

## First vertical slice
One asset market; heuristic/reference agents; structured bids `{time, asset, quantity, price, side}`; valid order matching; allocative-efficiency qualification; paired control/treatment statistics.

## Build
```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/dv026-demo
```

## Next implementation order
1. Private-value double auction with true total-surplus optimum.
2. News/intervention engine with visibility, reliability, timing, and ground-truth effect.
3. Black-box `AgentAdapter` for LLM query/output only.
4. Behavior feature extraction before semantic labels/classifiers.
5. Paired seeded campaigns and evidence records.
6. Human-baseline distributions and AI↔human distance/error functions.
7. 10-model stock-agent harness.
8. Minimal Science UI: Market / Agents / Information / Evidence.


## V3 behavioral layer
See `docs/V3_BEHAVIORAL_RESPONSE.md` for paired agent-level response, reaction/recovery latency, conformity observables, and cross-agent propagation.


## V5 campaign runner
Run `./build/dv026-campaign 100` to produce JSON, JSONL evidence, and `results/science.html`. See `docs/V5_CAMPAIGN_SCIENCE_UI.md`.

## V8
Black-box multi-model harness: provider/model/version provenance, canonical observations, strict structured action validation, failure/abstention accounting, JSONL logging, and deterministic replay. See `docs/V8_BLACK_BOX_MULTI_MODEL_HARNESS.md`.

## V10 — empirical qualification
V10 adds a source-grounded human market-efficiency reference and a formal qualification gate. See `docs/V10_EMPIRICAL_QUALIFICATION.md` and `V10_QUALIFICATION_REPORT.md`. Software qualification is intentionally separated from empirical/DARPA performance claims.


## V12 market-fidelity replication
V12 adds a Gode–Sunder source-grounded ZI-C replication harness and qualification boundary. See `docs/V12_MARKET_FIDELITY_REPLICATION.md`.

## v14 — Behavioral phenotyping
V14 adds behavioral fingerprints and competing-explanation discriminators. Run `v14-phenotype` to emit Markdown/JSON reports. These are known-mechanism qualification controls; they are not live-LLM findings.

## v15 automated scientist
`v15-scientist` runs a reusable experiment selector through a DV026 agent-policy adapter. See `docs/V15_AUTOMATED_SCIENTIST.md` and `V15_SCIENTIST_REPORT.md`. These are controlled in-process policy probes, **not live LLM or full-market experiments**.

## v17 — calibrated automated scientist

v17 adds train/held-out calibration of the full-market scientist. It freezes the selected measurement-noise floor before held-out evaluation and reports multiclass Brier score, mechanism recovery, and false discoveries under an information-blind null. The current qualification intentionally fails the >=80% recovery gate (66% overall), exposing confounding between signal-responsive/adaptive mechanisms while preserving a 0% null false-discovery rate in this controlled run. See `V17_SCIENTIST_CALIBRATION_REPORT.md`.

## DV026 Research Console runtime

Use `./start-workbench.sh` to configure and rebuild the current `build-dv026` targets before starting the console. This prevents a stale `dv026-workbench-runner` from being used after source updates.

The console performs a bounded runtime preflight and reports one of: **READY**, **OFFLINE**, or **ERROR**. An offline Ollama runtime is not a platform limitation; it means the local provider is not currently reachable.
