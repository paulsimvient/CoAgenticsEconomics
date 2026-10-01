# V11 — Market-Fidelity Replication

V11 adds a source-grounded replication harness based on Gode & Sunder (1993), *Journal of Political Economy* 101(1):119–137, DOI 10.1086/261868.

The paper's protocol supplies a strong institutional control for DV026 because constrained zero-intelligence (ZI-C) agents can achieve very high allocative efficiency without learning or strategic cognition. This prevents the benchmark from attributing market-level efficiency to an LLM when the auction institution itself can generate it.

## Implemented

- Literature targets for ZI-C and human mean allocative efficiency from Table 2.
- Six-buyer/six-seller laboratory-market protocol shape.
- Private buyer redemption values and seller unit costs.
- ZI-C no-loss action constraint.
- Repeated seeded periods.
- Market-by-market fidelity report and absolute-error calculation.
- Deterministic replay check.

## Scientific boundary

This release does **not** claim an exact numerical replication of Gode & Sunder's five markets. The exact unit-level demand/supply schedules are shown graphically in Figures 1–5; until those graphical schedules are independently transcribed and verified, the code uses explicit protocol-analogue schedules and labels the resulting numerical comparison accordingly.

That boundary is intentional. A software path that resembles a paper is not evidence of replication.
