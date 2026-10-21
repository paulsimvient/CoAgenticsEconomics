# CoAgentics Economics v16 — UI Implementation Status

This package replaces the primary `/` workbench interface with the integrated dark Economic Intelligence Lab UI.

Implemented in the single primary interface:

1. What Do We Know? dashboard
2. Experiment Builder (save design, freeze expectations, run registered campaign)
3. Agent / Decision Trace (live persisted cell index + cell detail endpoint)
4. Explanation Graph (registered hypotheses + selection)
5. Experiment Replay (persisted decision turns)
6. Memory Microscope (Section 16 persisted-memory inspection boundary)
7. Model / Condition Comparison (backend analysis by model)
8. Evidence Provenance (raw persisted turn + evidence chain)
9. Validation / Claim (backend-authoritative campaign gates and claim boundary)
10. Integrated navigation / persistent campaign context

Backend endpoints used are existing DV026 endpoints; no fake results are inserted by the UI.

Validation performed:
- JavaScript syntax check: PASS
- Local server root route: PASS
- `/api/dv026/campaign/status`: PASS

The C++ engine and evidence layer remain authoritative. UI-derived interpretation is explicitly labeled as such.
