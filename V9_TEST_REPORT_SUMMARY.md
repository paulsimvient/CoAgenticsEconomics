# V9 test report

- 19/19 CTest tests passed.
- Cross-model qualification matrix: 10 model identities × 3 seeds × 2 market mechanisms × control/treatment = 120 cells.
- The current matrix uses deterministic scripted qualification actions, not live LLM calls.
- Human-reference distributions in V9 are synthetic fixtures used only to validate comparison plumbing. They are not empirical human data and must not support scientific or DARPA performance claims.
- The 100% efficiency observed in the qualification matrix is a controlled code-path test, not a market-fidelity result.

See `docs/V9_CROSS_MODEL_HUMAN_REFERENCE_QUALIFICATION.md` and `results/v9_cross_model_qualification.json`.
