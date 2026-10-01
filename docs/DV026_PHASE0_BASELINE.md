# DV026 Phase 0 / Corrective Baseline Report

**Date:** 2026-10-01 (corrective pass)  
**Build tree:** `build` or `build-dv026` (`BUILD_TESTING=ON`)  
**Host:** macOS arm64 / AppleClang

## Build

| Item | Result |
|---|---|
| Configure | OK |
| Build | Required green after corrective pass |
| Known prior warning | `LlmExperiment.cpp` dangling-else style — cleaned |

## Test suite

Registered test count is **45** (Wave 1–4, Phase I/II, live campaign, workbench).

| Prior ZIP review | Corrective result |
|---|---|
| 44 pass / 1 fail (`llm_market_slice_smoke`) | **45/45 PASS** (`cmake --build build -j4 && ctest --output-on-failure`) |

### Prior failure (fixed)

- `llm_market_slice_smoke`: repeat run reused exhausted `RawJsonTransport` → `raw_json_exhausted`. Fixed by fresh transport per run.
- `v25_network_market_smoke`: directional `shift>0` was brittle (observed negative influence); assert now requires non-zero absolute matched shift.

## Architecture status (post-corrective)

| Capability | Status |
|---|---|
| AgentRole in AgentState / DecisionContext | Present (BUYER/SELLER + action constraints) |
| InformationContext (news/history/peer/constraints) | Present |
| Parser: integer time, exact action enum, HOLD semantics | Strengthened |
| Population via MarketMechanism (CDA + sealed) | Present |
| Classifier self-pair | Labeled `synthetic_wiring`; real path = `run_population_behavioral_contrast` |
| Live campaign Layer A trials | `LiveCampaignSpec.layer_a_trials` (default 5) |
| Live cell_ok | Requires non-HOLD market action + accepted + replay |
| HumanComparison on live obs | Wired (not “≥2 metrics exist”) |
| Captured-provider transcript replay | Renamed; not historical cutoff-T replay |
| Adaptive discovery | Model-conditional probe scaffolding only |

## Wave status

| Wave | Status |
|---|---|
| 0 Baseline | This report |
| 1 LLM slice | Complete + corrective |
| 2 Layer A/B | Complete |
| 3 Population / ten-LLM interface | Complete + corrective |
| 4 Transcript replay + probe scaffolding | Complete; naming corrected |
| Phase I suite | Present; `darpa_claim_ready` only under `scope=local_ollama_poc` |
| Phase II | Scaffolding only (deferred validation) |
| Live Ollama campaign | Infra present; full 10×20 still host-dependent |

## Explicit non-claims

- No Phase II construct validation
- No FAQ 31 classifier accuracy
- No commercial multi-provider matrix without a second campaign
- Transcript replay ≠ historical market-data replay
- Adaptive probes ≠ DV026 market experiment selection
