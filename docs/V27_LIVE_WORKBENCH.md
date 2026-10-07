# v27 — Live controlled-simulation workbench

The operator can start, pause, resume, reset and export a live C++ 12-trader CDA run from localhost. The runner emits each actual `DecisionTrace` **during execution**, then blocks for an acknowledgement from the controller before advancing to the next activation. The controller paces decisions (1–250 activations/second) and can pause the C++ engine, not just the chart. All four v26 panels remain: market monitor, agent network, decision traces, and matched experimental comparison.

Interventions set an exogenous message impulse and enable/disable edge 1→2; they take effect at the **next** activation after the operator applies them. The C++ engine recomputes exposure using the existing `analysis::propagate` implementation. The baseline is computed from the identical seed, activation tape and quote draws without intervention. The event stream, interventions and outcomes can be exported as reproducibility JSON. Reproducing a run requires replaying interventions after the same event numbers (not wall-clock timestamps).

This is a real-time **controlled C++ simulation**, not a live-LLM market. The synthetic private-value schedules are protocol analogues. The UI does not report unsupported confidence scores or human baselines. The engine is not yet a general arbitrary-time intervention scheduler or a multi-user service; the localhost server manages one experiment at a time.

## Routes (current)

| URL | Surface |
|---|---|
| `http://127.0.0.1:8787/` | **Interface** — primary DV026 daily driver |
| `http://127.0.0.1:8787/live.html` | Research Console — gates, bids, Method |
| `http://127.0.0.1:8787/market_lab.html` | Market Lab — secondary guided shell |
| `http://127.0.0.1:8787/matlab.html` | This paced CDA Market Live demo |

Run `./start-workbench.sh` and open the Interface at `/`. Open `/matlab.html` for the v27 paced CDA controller. To run tests: `ctest --test-dir build --output-on-failure`.
