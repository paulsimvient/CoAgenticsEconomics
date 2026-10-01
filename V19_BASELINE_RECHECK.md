# CoAgentics v19 — Temporal interventions inside the full CDA

Five sequential 12-trader CDA sessions: news pulse, repeated news, washout, reversal, peer-only. Paired common-random-number control/treatment within each session; persistent adaptive target state; train-only centroids frozen before disjoint held-out seeds. This is a controlled-mechanism qualification, not live LLM validation. Quote composition can change after interventions.

| Mechanism | Held-out recovery | Abstained | Trials |
|---|---:|---:|---:|
| information-blind | 100.0% | 0 | 30 |
| signal-responsive | 43.3% | 0 | 30 |
| risk-sensitive | 50.0% | 0 | 30 |
| peer-responsive | 83.3% | 1 | 30 |
| adaptive | 40.0% | 2 | 30 |

Overall recovery 63.3%; null false discoveries 0.0%; abstention 2.0%.

Gates: recovery FAIL, null PASS.

**Limits:** Full market dynamics, but independent market resets between sessions; only the adaptive target's state persists. The five marginal schedules remain analogues rather than exact published schedules. No live models or human behavioral baseline are involved.
