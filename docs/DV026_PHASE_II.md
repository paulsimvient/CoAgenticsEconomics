# DV026 Phase II — Bias / deception / collaboration (operational scaffolding)

Phase II adds **observable** measurement protocols for the three constructs DARPA places beyond Phase I. It does **not** claim validated detectors, intent inference, or Phase II milestone completion.

## Delivered

| Protocol | API | Observable |
|---|---|---|
| Bias | `measure_signal_response_bias` | Asymmetric bid response to signed public signals |
| Deception | `run_deception_misrepresentation_protocol` | Statement≠belief counts via v24 `analyze_communication`; `intent_identifiable=false` |
| Collaboration | `measure_bid_co_movement` | Closer bids under shared vs independent signals |

Suite: `run_phase_ii_measurement_suite`  
Gates: `software_green` may be true; `constructs_validated` is always **false**.

## Claim boundaries

```
asymmetry_index     ≠ cognitive-bias diagnosis
misrepresentation   ≠ deceptive intent
bid co-movement     ≠ collusion / conspiracy
software_green      ≠ Phase II empirically complete
```

## Relation to Phase I

Phase I demonstrates an operational classifier **without** performance metrics (FAQ 31) and defers these constructs. Phase II wires the deferred measurement paths on controlled fixtures. Live LLM / human validation of the constructs remains outstanding.

## Workbench

See [`DV026_WORKBENCH.md`](DV026_WORKBENCH.md) — Phase II observables appear in **DV026 Evidence** mode after a `bundle` run.

## Tests

```bash
cmake --build build-dv026 -j8 --target dv026_phase_ii_smoke
./build-dv026/dv026_phase_ii_smoke
ctest --test-dir build-dv026 -R 'dv026_' --output-on-failure
```
