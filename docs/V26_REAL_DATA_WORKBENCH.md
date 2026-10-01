# v26 — Real simulation data workbench (four-panel scope)

The workbench contains ONLY four user-requested views: market monitor, agent network, decision traces, matched experimental comparisons. All displayed numbers are loaded from `workbench/experiment.json`, produced by the actual v25 C++ market and network experiment (`v26-dashboard-export`). There are no simulated live indicators, invented confidence scores, human-reference plots, or LLM results.

## Rebuild data

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
./build/v26-dashboard-export > workbench/experiment.json
ctest --test-dir build --output-on-failure
./start-workbench.sh
```

The static UI is read-only and runs on localhost. `experiment.json` contains both arms' complete 1,800-event traces, network exposure, matched attempted-quote shifts, and separately executed edge-cut and null outcomes. It is intentionally not a real-time dashboard or experiment launcher. Synthetic market marginal schedules remain analogues rather than a verified transcription of the Gode–Sunder experiments.
