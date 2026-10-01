# DV026 Phase I — Software evidence & architecture alignment

After Waves 1–4, this package consolidates **Phase 15** (regression/qualification suite) and **Phase 16** (architecture labeling). Phase II construct scaffolding is separate — see `docs/DV026_PHASE_II.md`.

## Suite groups (`run_phase_i_qualification_suite`)

| Group | Scope |
|---|---|
| **A** Market mechanics | Layer A CDA + sealed ≥90% with programmed agents |
| **B** LLM interface | Parse → validate → submit, private-state boundary, historical replay |
| **C** Multi-agent | Mixed LLM+programmed population, operational classifier wired, human reference attached |
| **D** DV026 qualification | Rollup + explicit `NOT_CLAIMED` for live 10-LLM economic campaign and Phase II |

**Gates:** `software_green` may be true; `darpa_claim_ready` is always **false** in this suite.

## Capability alignment labels

| Capability | Label |
|---|---|
| CDA / sealed matching, surplus, efficiency | **Existing** (pre-wave) + Wave 2 Layer A gate |
| LLM DecisionContext / parse / SubmitResult / provenance | **Phase I implementation** (Wave 1) |
| Layer A ≥90% market environment qualification | **Phase I implementation** (Wave 2) — software path demonstrated |
| Paired ZI vs LLM observable deltas | **Phase I implementation** (Wave 2) |
| PopulationSpec multi-agent mediation | **Phase I implementation** (Wave 3) |
| ≥10 distinct LLM interface/provenance (scripted) | **Phase I implementation** (Wave 3) |
| Operational classifier (no accuracy metrics) | **Phase I implementation** (Wave 3, FAQ 31) |
| Human-market literature reference compare | **Phase I implementation** (Wave 3) |
| Adaptive discovery/confirmation | **Phase I implementation** (Wave 4 research extension) |
| Historical replay / cross-event validation | **Phase I implementation** (Wave 4) |
| Live ≥10 commercial LLM economic campaign | **Phase I qualification** (Month-12 target) — `NOT_CLAIMED` |
| Bias / deception / collaboration measurement | **Phase II scaffolding** — see `docs/DV026_PHASE_II.md` (constructs not validated) |
| New human-subject research | Out of scope (FAQ) |

## Claim boundary

```
software_green ≠ DARPA milestone satisfied
operational classifier ≠ classifier accuracy
ten-LLM interface ≠ live multi-provider economics
Phase I evidence ≠ Phase II constructs
```

## Workbench

See [`DV026_WORKBENCH.md`](DV026_WORKBENCH.md) — **DV026 Evidence** mode runs this suite via `dv026-workbench-runner`.

## Tests

```bash
cmake --build build-dv026 -j8 --target dv026_phase_i_smoke
./build-dv026/dv026_phase_i_smoke
ctest --test-dir build-dv026 -R 'dv026_|llm_market_slice' --output-on-failure
```
