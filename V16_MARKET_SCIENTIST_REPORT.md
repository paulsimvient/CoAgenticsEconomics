# CoAgentics v16 — Full-market automated scientist

# CoAgentics v15 — Automated Scientist

Domain adapter: DV026 12-trader CDA (full market)

Status: **SEPARATED (controlled-model assumption)**

Leading explanation (not necessarily established): news

| Probe | Matched delta | SE | Decision |
|---|---:|---:|---|
| news-only | 6.489 | 1.258 | SEPARATED (controlled-model assumption) |

## Posterior by step (conditional on candidate set and noise model)

news-only: blind=0.005 news=0.989 peer=0.005 

These are controlled in-process agents, not live LLMs. Posterior probabilities are model-conditional, not empirical probabilities of intent.

## Secondary market outcomes (first matched seed per selected probe)

| Probe | Control efficiency | Treatment efficiency | Δ efficiency | Control trades | Treatment trades | Target quotes (C/T) |
|---|---:|---:|---:|---:|---:|---:|
| news-only | 99.55% | 99.55% | 0.00 pp | 22 | 22 | 7/9 |

The CDA uses transparent analogue marginal schedules, not verified published schedules. The targeted agent is a controlled policy, not a live LLM. A matched activation/random-number tape controls exogenous randomness, but changed trades alter subsequent market state; quote averages may also change composition. Posterior inference uses the v15 linear probe approximation, which is not calibrated to this nonlinear market; treat its probabilities as exploratory, not validated mechanism recovery.
