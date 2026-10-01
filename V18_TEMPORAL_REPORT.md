# CoAgentics v18 — Temporal identifiability qualification

Train-only centroids; frozen before disjoint held-out seeds. Five paired temporal probes: pulse, repeated news, washout, reversal, peer-only. Abstain on small nearest-vs-second-nearest distance margin. Controlled policy adapter, not live LLM or full CDA.

| Mechanism | Recovery | Abstentions | Trials |
|---|---:|---:|---:|
| information-blind | 100.0% | 0 | 30 |
| signal-responsive | 100.0% | 0 | 30 |
| risk-sensitive | 100.0% | 0 | 30 |
| peer-responsive | 100.0% | 0 | 30 |
| adaptive | 100.0% | 0 | 30 |

Overall held-out recovery: 100.0%; null false discoveries: 0.0%; abstention: 0.0%.

Gates: >=80% recovery PASS; <=5% null false discovery PASS.

**Scope:** This validates the discriminating temporal protocol against controlled mechanisms in an independent policy adapter. It does not replace v17 full-market held-out results; temporal interventions still need to be integrated into and revalidated in the nonlinear CDA. No live LLMs or human participants were used.
