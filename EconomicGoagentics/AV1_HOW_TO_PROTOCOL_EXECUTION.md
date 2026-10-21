# CoAgentics AV-1 — How to run a protocol-controlled live decision experiment

This update adds `protocol-decision` to the real C++ workbench runner, plus `tools/av1_execute_plan.py` for balanced, matched execution. This is a **fixed-context decision probe**, not yet protocol-driven shared-market execution.

## Requirements

CMake, a C++17 compiler, Python 3, Ollama running locally, and two Ollama models recognized by the C++ catalog. Use synthetic, non-CUI data only.

## Build

```sh
cmake -S . -B build-av1 -DCMAKE_BUILD_TYPE=Release
cmake --build build-av1 --target dv026-workbench-runner -j 4
ollama list
```

## Prepare plan.json

Create a JSON object with `protocol_id`, `protocol_sha256` (64 lowercase hex characters, the SHA-256 of the separately approved protocol), and `assignments`. Each assignment must include `scenario_id`, `trial`, `model`, `condition`, and `seed`. Example (replace model tags and protocol hash):

```json
{
 "protocol_id":"price-signal-v1",
 "protocol_sha256":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
 "assignments":[
  {"scenario_id":"market-1","trial":"1","model":"MODEL_A","condition":"control","seed":424242},
  {"scenario_id":"market-1","trial":"1","model":"MODEL_A","condition":"intervention","seed":424242},
  {"scenario_id":"market-1","trial":"1","model":"MODEL_B","condition":"control","seed":424242},
  {"scenario_id":"market-1","trial":"1","model":"MODEL_B","condition":"intervention","seed":424242}
 ]
}
```

The `a...` hash is a **placeholder, not an approved protocol fingerprint**. Obtain a real hash with `shasum -a 256 approved_protocol.json` (macOS) or `sha256sum approved_protocol.json` (Linux). Use exact available model names.

## Execute

```sh
python3 tools/av1_execute_plan.py plan.json build-av1/dv026-workbench-runner results/price-signal-v1
```

Choose an empty output directory. Results include a plan copy, a running/completed/failed manifest, and SHA-256-fingerprinted per-cell JSON evidence. If Ollama is unavailable, the model is absent, or a response is missing, execution stops; **no mock fallback**. Restart with a fresh directory; resume is not supported.

## What is actually manipulated?

The C++ decision context holds market price, buyer state, and history fixed, while changing `public_signal` from 100 (control) to 70 (intervention). The runner records exact model tag, seed, scenario, trial, condition, protocol ID and claimed fingerprint, canonical request, raw response, and parse status. The plan validates paired equal seeds and at least two models.

## Critical scientific limits

This is **not yet** a shared-market campaign controlled by a protocol. The runner copies the supplied protocol hash without independently checking the protocol file; this is traceability metadata, not cryptographic approval. A raw model response is not a ground-truth decision-quality score. The experiment does not yet validate information reliability, infer economic welfare, compute statistical significance, or support independent D2P2 feasibility claims. Review `parse_ok` separately from response receipt. Use a preregistered scoring rubric and repeated trials before drawing scientific conclusions.
