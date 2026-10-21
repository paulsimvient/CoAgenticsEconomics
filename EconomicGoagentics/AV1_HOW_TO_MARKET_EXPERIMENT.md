# AV-1 How to run protocol-controlled multi-agent markets

This update executes each approved assignment in the **heterogeneous C++ market engine** (not just the fixed-context probe). The named model participates with one other available Ollama model, with both buyer/seller roles rotated by the existing market assignment logic. All model responses must be live; mock fallback is prohibited.

1. Start Ollama; `ollama list` must show at least two installed models supported by the C++ catalog. Use synthetic, non-CUI data.
2. Build: `cmake -S . -B build-av1 -DCMAKE_BUILD_TYPE=Release` then `cmake --build build-av1 --target dv026-workbench-runner -j 4`.
3. Create `plan.json` as described in `AV1_HOW_TO_PROTOCOL_EXECUTION.md`, with real model tags and the SHA-256 of your approved protocol. Use at least one matched control/intervention pair per model. The Python controller checks paired seeds and duplicate assignments.
4. Run: `python3 tools/av1_execute_market_plan.py plan.json build-av1/dv026-workbench-runner results/market-pilot-1`.
5. Inspect `results/market-pilot-1/manifest.json`; it must say `completed`. Each `cell_0000/` contains `campaign.json` (market metrics) and `protocol_cell_0.json` (protocol identity, intervention, model identities, canonical prompts and raw responses). The controller hashes each observation file in the manifest.

**Interpretation:** Control sets `public_signal=100`; intervention sets `public_signal=70`, while the market fundamental remains 100. This is a controlled **signal perturbation**, not yet a validated manipulation of information reliability. All agents in a given market see the same condition. The named model is present, but the other model is selected from the preflight catalog; its identity is recorded in the campaign assignment. The trial is not an isolated per-model causal effect, since both agents interact. Runs are independent markets, and treatment effects need appropriate replication and scoring. No resume, automatic scientific scoring, or complete statistical qualification is claimed. The C++ runner accepts the protocol fingerprint as metadata and does not independently verify the protocol source file. Raw model outputs may include sensitive information; do not use CUI.

**Failure behavior:** If Ollama is down, a required model is absent, a response is missing, or the campaign fails, the controller marks the manifest failed and stops. Use a new output directory to retry.
