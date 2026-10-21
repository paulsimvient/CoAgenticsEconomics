# Q1 qualification record — 2026-10-21

## Verified in this execution environment

1. CMake configure succeeded with GNU C++ 14.2.0.
2. Initial Release build attempts timed out. A Debug build initially failed to link because the discovered `/usr/local/lib/libcurl.a` did not export its transitive link dependencies. CMake was updated to include pkg-config's libcurl dependency list when static libcurl is selected. A subsequent complete Debug build of `dv026-workbench-runner` succeeded.
3. The deterministic heterogeneous C++ smoke campaign ran successfully (`hetero-campaign 424242 1 --smoke`), returning four cells with two mock model identities. This does **not** qualify live Ollama inference. Its `minimum_model_requirement_met` is false due to the non-smoke threshold, despite the intentionally reduced mock population.
4. Python unit tests for assignment validation, Q1 archive integrity, and descriptive scoring passed (22 tests).
5. Protocol C++ syntax check passed after adding explicit persisted market trades, market fundamental, and market metrics to the per-cell evidence archive.
6. A protocol run requesting an unavailable model exited nonzero without creating a fabricated result directory.

## Not verified

- Live Ollama inference and actual two-model paired market execution.
- Real-run Q1 archive audit (tests used synthetic evidence fixtures).
- Statistical comparison with conventional benchmark and defensible confidence intervals.
- Complete GUI researcher workflow.

## Engineering changes

- `CMakeLists.txt`: static curl transitive link support.
- `src/HeterogeneousCampaign.cpp`: protocol evidence now includes recorded market fundamental, individual trades, market metrics, and checked output stream.
- `tools/q1_audit_market.py`: read-only fail-closed archive integrity audit.
- `tools/q1_score_market.py`: descriptive recalculation of market surplus and price deviation from archived trades and private values.
- `tools/av1_execute_market_plan.py`: requires evidence of two responding model identities; performs audit and descriptive scoring before marking a completed run qualified.
- `tests/test_q1_audit_market.py`, `tests/test_q1_score_market.py`: synthetic fixture unit tests.
- `Q1_HOW_TO.md`: step-by-step build, run, and evidence review instructions.

**Scientific boundary:** This is a qualification milestone, not a validated scientific finding or a D2P2 eligibility determination.
