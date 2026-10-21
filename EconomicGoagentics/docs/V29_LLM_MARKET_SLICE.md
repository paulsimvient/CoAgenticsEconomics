# v29 — Real LLM market vertical slice (economic decision path)

Additive path on top of existing `market::Market`, `EvidenceStore`, and `ModelTransport`.

## Decision pipeline

```
ExperimentSpec / RunSpec
  → MarketState (public) + AgentState (own private only)
  → DecisionContext
  → LlmAgentAdapter / provider
  → raw response (retained exactly)
  → parse MarketAction
  → action validation
  → Market::submit_detailed
  → SubmissionResult (accepted ≠ filled)
  → AgentStateAfter
  → RunResult + EvidenceStore (traceability claim only)
```

## Information boundary

The model receives `MarketState` plus **its own** `AgentState` (cash, inventory, private value, prior fill).
It does **not** receive another agent's private value, cash, or inventory.

## Schema

```json
{"action":"BUY","asset":"ASSET","quantity":1,"price":95,"time":0}
{"action":"HOLD","asset":"ASSET","quantity":0,"price":null,"time":0}
```

No silent repair of malformed responses. No third-party JSON library was present in-repo; parsing is schema-specific and strict.

## Evidence scope

`H_LLM_MARKET_SLICE` / `D_PROVENANCE_CHAIN` means: the LLM pathway ran with complete stage provenance.
It does **not** claim economic rationality or performance.

## Tests

```bash
cmake -S . -B build-v29 -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build-v29 -j8
ctest --test-dir build-v29 -R 'llm_market_slice_smoke|llm_live_opt_in|llm_harness|market_smoke' --output-on-failure
```

Deterministic integration uses seller limit 90 and scripted BUY @ 95 (mid fill 92.5).
