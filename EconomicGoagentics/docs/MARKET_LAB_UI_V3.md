# Market Lab UI v3

The integrated Market Lab frontend is `workbench/market_lab.html` and is served at `/market_lab.html` by `live_server.py`. Interface is primary at `/` / `/index.html`. The Research Console remains at `/live.html`.

## Workflow
1. Experiment design
2. Research expectations
3. Qualification
4. Run
5. Results
6. Evidence

## Scientific boundary
The frontend does not fabricate market or LLM results. Empty result fields remain empty until backed by persisted campaign data. The design page configures the experiment; expectations are frozen through the existing SHA-bound API; qualification calls the existing Phase I runner; live runs call the existing campaign API; results read the existing charts/analysis endpoints; evidence opens persisted cell traces.


## Integration note (2026-10-04)
The Market Lab UI is not a standalone prototype. `live_server.py` serves `workbench/market_lab.html` at `/market_lab.html`, and the UI reads/writes the live experiment state through the existing DV026 endpoints. The design layer is persisted through `/api/dv026/design` and `/api/dv026/design/save`; expectations remain bound through the existing expectations save/freeze endpoints; qualification uses `/api/dv026/preflight` and `/api/dv026/last`; campaign execution uses `/api/dv026/campaign/start`, `/api/dv026/campaign/status`, `/api/dv026/campaign/charts`, `/api/dv026/campaign/analysis`, `/api/dv026/campaign/cells`, and `/api/dv026/campaign/cell`.

The visual Market view is therefore a projection of retained campaign state. It does not invent market outcomes, model actions, prices, or evidence. Unsupported configuration choices are persisted as design metadata and are not presented as if they alter the runner unless the backend accepts them.
