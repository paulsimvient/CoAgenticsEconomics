# DV026 Wave 3 — Multi-agent / 10-LLM / operational measurement

Wave 3 scales the qualified market environment without claiming Phase II behavioral constructs or classifier accuracy.

## Delivered

### PopulationSpec
Shared-market seats with:
- programmed heuristic / ZI agents
- LLM agents (Wave-1 `DecisionContext` boundary: own private state only)

API: `run_population_market`

### Ten distinct LLM interface qualification
Catalog of 10 identities differing by **provider / model family / version** (config-only duplicates do not count).

API: `dv026_distinct_llm_catalog`, `run_ten_llm_interface_qualification`  
CI uses `ScriptedTransport` (no network). Live transports remain opt-in elsewhere.

### Operational behavioral measurement
Reuses `compare_paired_bids` + `MechanismClassifier`.  
**Explicitly no Phase I classifier performance metrics** (FAQ 31).

### Human-market reference
Compares population allocative efficiency to the existing Gode–Sunder (1993) literature catalog via `empirical_market_reference_catalog()`.

## Claim boundaries

| Capability | Claim |
|---|---|
| Layer A (Wave 2) | Market environment ≥90% with programmed agents |
| Ten-LLM (Wave 3) | Interface + provenance across ≥10 distinct identities |
| Classifier | Operational wiring only — no accuracy metrics |
| Population | Observable multi-agent market mediation |

## Not in Wave 3

- Adaptive experiment selection / discovery-confirmation (Wave 4 — see `docs/DV026_WAVE4.md`)
- Historical replay (Wave 4 — see `docs/DV026_WAVE4.md`)
- Live commercial multi-provider matrix execution
- Phase II bias / deception / collaboration measurement

## Tests

```bash
cmake --build build-dv026 -j8 --target dv026_wave3_smoke
./build-dv026/dv026_wave3_smoke
ctest --test-dir build-dv026 -R 'dv026_wave3|dv026_wave2|llm_market_slice' --output-on-failure
```
