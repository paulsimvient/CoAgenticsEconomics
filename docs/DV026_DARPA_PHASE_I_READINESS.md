# DV026 DARPA Phase I Readiness (local Ollama PoC)

This document defines when `darpa_claim_ready` may flip to **true**, and only under **`scope=local_ollama_poc`**.

## Claim boundary

| Allowed when ready | Still forbidden |
|---|---|
| Local open-weight Ollama Layer B matrix met the gate | Phase II bias / deception / collaboration **validation** |
| Observable market + interface + provenance evidence | Phase I classifier **accuracy** metrics (FAQ 31) |
| LiveProvider observations compared via `HumanComparison` | Commercial 10-provider matrix without a second campaign |

`software_green=true` from the Phase I suite is **not** the same as `darpa_claim_ready`.

## Readiness gate checklist

| Check | Pass rule |
|---|---|
| Layer A | `layer_a_trials` (default **5**) programmed agents ≥90% CDA and sealed |
| Live count | ≥10 distinct Ollama identities with ≥1 **economic** cell_ok |
| Coverage | Each of those 10 completes ≥N seeds (default **N=20**) |
| Provenance | ≥95% attempted cells captured-provider transcript `replay_ok` |
| Interface health | ≥90% cells parse + action_valid |
| Market action | ≥80% cells non-HOLD accepted market actions (HOLD-only ≠ economic success) |
| Human ref | LiveProvider `allocative_efficiency` observations reach `DESCRIPTIVE_COMPARISON` via HumanComparison |
| Non-claims | Report still states no Phase II construct validation |

`cell_ok` requires: `interface_ok` ∧ `market_action_ok` ∧ `market_accepted` ∧ `replay_ok`.

## Smoke vs full 10×20

```bash
# Catalog (≥10): smollm2:1.7b, llama3, llama2, qwen3:8b, qwen2.5-coder:1.5b,
# qwen2.5:1.5b, codellama, mistral, gemma2:2b, phi3:mini
ollama list   # pull any missing tags before the full run
./build/dv026-workbench-runner ollama-campaign 424242 2 --smoke   # never flips ready
./build/dv026-workbench-runner ollama-campaign 424242 20          # full gate
./build/dv026-workbench-runner phase-i 424242                     # D6 reads campaign summary.json

# Scientific next (scripted/mock by default; hetero-pop --smoke for CI):
./build/dv026-workbench-runner hetero-pop 424242 --smoke
./build/dv026-workbench-runner info-contrast 424242
```

Artifacts: `results/dv026_ollama_campaign/summary.json` (`darpa_claim_ready`, `scope=local_ollama_poc`).

See also `docs/DV026_PHASE0_BASELINE.md` for corrective-pass status.
