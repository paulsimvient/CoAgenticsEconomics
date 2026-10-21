# CoAgentics v17 — Scientist calibration and held-out validation

The automated scientist is calibrated on one seed partition and evaluated on a disjoint held-out partition. No live LLMs or human subjects are used in this qualification.

| Measure | Result |
|---|---:|
| Calibrated measurement noise SD | 3 |
| Training Brier score | 0.0823 |
| Held-out Brier score | 0.0782 |
| Held-out mechanism recovery | 66.0000% |
| Null false-discovery rate | 0.0000% |

| Controlled mechanism | Expected explanation | Recovery | Mean P(true) | Brier |
|---|---|---:|---:|---:|
| information-blind | blind | 100.0000% | 0.9120 | 0.0026 |
| signal-responsive | news | 10.0000% | 0.3445 | 0.1423 |
| risk-sensitive | risk | 95.0000% | 0.5611 | 0.0601 |
| peer-responsive | peer | 85.0000% | 0.7175 | 0.0396 |
| adaptive | adaptive | 40.0000% | 0.3242 | 0.1465 |

Qualification gates: calibration=PASS, null false-discovery=PASS, mechanism recovery=FAIL.

Interpretation boundary: these rates validate recovery of five known in-process mechanisms under the current CDA, probe set, priors, and seed distribution. They do not validate inference of latent intent in arbitrary models. A failure to separate remains a valid scientific outcome.
