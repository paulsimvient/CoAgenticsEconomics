# DV026 Wave 4 — Research extensions

Wave 4 adds adaptive discovery/confirmation and historical replay on top of Waves 1–3. It does **not** claim Phase I classifier accuracy or Phase II behavioral constructs.

## Delivered

### Adaptive discovery / confirmation
Wraps the existing domain-neutral scientist (`run_scientist`):

1. **Discovery** — prior-weighted probe selection until SEPARATED or budget exhausted  
2. **Confirmation** — held-out seeds must recover the same leading hypothesis at a confirmation posterior threshold

API: `run_adaptive_discovery_confirmation`  
Default smoke domain: `AgentProbeDomain` (controlled policy probes). Full-market domains (`CdaMarketDomain`) plug in via the same `ExperimentalDomain` interface.

### Historical replay + cross-event validation
Capture raw provider responses from an LLM market `RunResult`, rebuild a `ReplayTransport`, re-execute under the same seed/spec, and validate **each** turn (raw / parse / fill).

API: `capture_historical_replay_log`, `replay_and_validate`

## Claim boundaries

| Capability | Claim |
|---|---|
| Discovery | Model-conditional probe selection / mechanism recovery on controlled domains |
| Confirmation | Held-out agreement with discovery leader — not empirical Phase I readiness |
| Historical replay | Captured-payload reproducibility + cross-event match |
| Live providers | Not claimed; transcript fidelity only |

## Not in Wave 4

- Live multi-provider economic campaign
- Phase II bias / deception / collaboration measurement
- Familywise FDR control under model misspecification
- Claiming Wave 2 Layer A market qualification from scientist posteriors

**Follow-on:** Phase I software evidence suite — `docs/DV026_PHASE_I.md`.

## Tests

```bash
cmake --build build-dv026 -j8 --target dv026_wave4_smoke
./build-dv026/dv026_wave4_smoke
ctest --test-dir build-dv026 -R 'dv026_wave|llm_market_slice' --output-on-failure
```
