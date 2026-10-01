# v28 Scientific Workbench

MATLAB-inspired, functional browser UI at `http://127.0.0.1:8787` using the existing v27 paced C++ engine. It includes run/pause/resume/reset/replay, seed/speed/event controls, news/withdraw/link interventions, market transactions, agent network, decision trace filters, quote histograms, individual agent behavior, matched final market comparisons, intervention history, JSON/CSV/SVG exports and a printable experiment report.

**Evidence boundary:** This is a local, controlled 12-trader C++ simulation, not a live LLM market. The engine currently models a known 0→1→2 communication route, not a general 12-node social graph. The UI does not fabricate calibrated hypothesis confidence, human comparisons, provisional efficiency or live-LLM activity. The matched market comparison is shown only after the engine produces both outcomes.

Run `./start-workbench.sh` and open `http://127.0.0.1:8787`.
