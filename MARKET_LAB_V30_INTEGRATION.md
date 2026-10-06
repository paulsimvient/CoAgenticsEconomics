# Market Lab integration pass

## Changes
- Market Lab is now the primary web application at `/` and `/index.html`.
- `/interface.html` remains available as the general simulation/interface view.
- Run visualization now reads persisted `/api/dv026/campaign/cell` turn records.
- The abstract A/B/C/D layer highlights the retained agent/seat when a cell turn is selected.
- Market state, parsed action, acceptance, execution, and payoff are populated from the retained turn; no synthetic values are created.
- Model/seed selectors in Run inspect persisted cells directly.
- Tooltips were extended to the new Run controls and visualization concepts.

## Verification
- Clean CMake build completed.
- CTest: 50/50 passed.
- HTTP smoke test: `/`, `/market_lab.html`, `/interface.html`, `/api/dv026/design`, `/api/dv026/campaign/status`, and `/api/dv026/campaign/cells` returned successfully.
- Cell detail smoke test loaded a persisted cell and 13 retained turns.
