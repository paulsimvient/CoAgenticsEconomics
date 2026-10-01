# CoAgentics v20 — Full-market decision observability

Five full-CDA temporal sessions with event-level decision traces. The observation is each agent's emitted desired quote before institutional clamping, aligned on identical activation events. Local market-state and random-draw effects are subtracted separately within each paired branch. Train-only centroids are frozen before disjoint held-out seeds. Missing matched legal actions cause abstention. This is controlled-mechanism recovery, not live LLM validation.

| Mechanism | Held-out recovery | Abstained | Trials |
|---|---:|---:|---:|
| information-blind | 100.0% | 0 | 30 |
| signal-responsive | 100.0% | 0 | 30 |
| risk-sensitive | 100.0% | 0 | 30 |
| peer-responsive | 100.0% | 0 | 30 |
| adaptive | 100.0% | 0 | 30 |

Overall recovery 100.0%; null false discoveries 0.0%; abstention 0.0%; matched legal-action coverage 97.5%.

Gates: recovery PASS, null PASS.

**Important:** The emitted desired quote is observable in this controlled agent adapter, not inferred from executed trades. Live-model integration must log raw actions before validation. The intervention signal is known to the experimenter. Five market schedules remain analogues. No live LLMs, human subjects, or exact historical market replication.
