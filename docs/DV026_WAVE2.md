# DV026 Wave 2 — Economic experiment layers

Wave 2 separates **market qualification** from **AI behavioral comparison**. It does not start the 10-LLM campaign, adaptive selection, or historical replay.

## Layer A — Market qualification

```text
Programmed / heuristic reference agents
        ↓
CDA  +  Sealed-bid double auction  (existing MarketMechanism)
        ↓
allocative efficiency vs theoretical maximum (existing Metrics)
        ↓
gate: mean efficiency ≥ 90% across trials
```

API: `run_market_qualification(MarketSpec, first_seed, n_trials)`  
Evidence: `H_MARKET_ENVIRONMENT_QUALIFIED` / `D_ALLOCATIVE_EFFICIENCY_GATE`  
**Claim boundary:** qualifies the market test environment — **not** LLM performance.

## Layer B — Paired observable comparison

```text
CONTROL: programmed buyer limit + deterministic seller
TREATMENT: Wave-1 LLM path (same seed, same private values / counterparty)
        ↓
Δ efficiency, Δ surplus, Δ trades, Δ mean price
```

API: `run_paired_zi_vs_llm(ExperimentSpec, RunSpec, control_buyer_limit_price)`  
Evidence: `H_PAIRED_OBSERVABLE_DELTA` / `D_SHARED_SEED_MARKET` (direction inconclusive by design)  
**Claim boundary:** observable outcome deltas only — not rationality / bias / deception.

## What Wave 2 does not do

- No PopulationSpec / multi-LLM matrix (Wave 3)
- No operational classifier performance metrics (Wave 3 / Phase II)
- No adaptive experiment selection (Wave 4)
- Does not fix pre-existing `v25_network_market_smoke` on this host

## Tests

```bash
cmake --build build-dv026 -j8 --target dv026_wave2_smoke
./build-dv026/dv026_wave2_smoke
ctest --test-dir build-dv026 -R 'dv026_wave2|llm_market_slice' --output-on-failure
```
