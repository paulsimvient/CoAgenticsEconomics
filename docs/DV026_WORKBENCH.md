# DV026 Workbench — Market Live + Evidence

Dual-mode UI at `http://127.0.0.1:8787` ([`workbench/matlab.html`](../workbench/matlab.html)). Live entry only — ignore legacy `workbench/index.html` / `live.html`.

```bash
./start-workbench.sh
# opens Market Live by default; switch to DV026 Evidence in the title bar
```

`start-workbench.sh` prefers `build-dv026/`, then `build/`, and builds both runners if missing.

## Mental model: two products, one page

| Mode | Job | Engine | Success signal |
|---|---|---|---|
| **Market Live** | Watch / poke a paced 12-trader double auction | `v27-live-runner` | Trades, efficiency, interventions — **demo**, not DARPA |
| **DV026 Evidence** | Software gates + optional local Ollama Layer B | `dv026-workbench-runner` | Three greens below (not interchangeable) |

### Three different “greens” (Evidence)

| Flag | Means | Does **not** mean |
|---|---|---|
| **`software_green`** | Scripted Phase I A/B/C/D checks pass | Live LLM economics or DARPA milestone |
| **`live_llm`** | This run talked to Ollama | Campaign readiness or Phase II |
| **`darpa_claim_ready`** + `scope=local_ollama_poc` | Full Ollama 10×20 campaign gate passed | Phase II constructs / commercial 10-provider matrix |

Phase II cards stay scaffold / “not validated” even when `darpa_claim_ready` is true.

See [`DV026_DARPA_PHASE_I_READINESS.md`](DV026_DARPA_PHASE_I_READINESS.md) for the readiness checklist. The Research Console claim surface (`workbench/live.html` · `#claimSurface`) puts **Readiness predicate** beside **Campaign progress** at the top; bottom panels are live SVG charts from `cells.jsonl` (models × seeds, human η, evidence pipeline), not narrative placeholders. **Proposal expectations (auto)** scores white-paper M2/M6/M9/M12 items (fixture [`DV026_PROPOSAL_EXPECTATIONS.json`](DV026_PROPOSAL_EXPECTATIONS.json)) against live Layer A + campaign evidence via `GET /api/dv026/proposal-narrative` (`?format=txt` for download). Readiness is a Check/Value/Require/Status table; Start opens a market-calibration modal when Layer A is not yet QUALIFIED.

## Which button for what (DV026 ribbon)

The Evidence ribbon is three labeled groups:

### 1. Single run

| Control | What it does |
|---|---|
| Command + **Run Evidence** | `POST /api/dv026/run` → `dv026-workbench-runner <cmd> <seed>` |
| **Run Ollama slice / paired** | Same API with `ollama-slice` / `ollama-paired` + model → sets `live_llm` |
| **Export JSON** | Last single-run payload |

Useful commands: `phase-i` (checklist), `bundle` (rich pack), `layer-a`, `ten-llm`, `llm-slice`, `population`, `adaptive`, `phase-ii` (scaffold only).

### 2. Phase I sweep

| Control | What it does |
|---|---|
| Trials / Base seed + **Run N-seed validation** | Loop `phase-i` only — scripted reproducibility |
| Cancel / Export batch | Stop or download batch summary |

Pass rate here is **not** `darpa_claim_ready`.

### 3. Ollama campaign

| Control | What it does |
|---|---|
| Campaign seeds + smoke + **Run campaign** | `ollama-campaign` Layer B readiness |
| Uncheck smoke, seeds=20 | Full gate (10 models × 20) — may flip `darpa_claim_ready` |
| Export campaign | Download campaign summary |

Mutual exclusion: market vs batch vs campaign (and single runs while those run).

## Suggested first clicks

1. **Market Live** → Run once → watch chart/trace.
2. **DV026 Evidence** → command `phase-i` → Run Evidence → read the three greens.
3. Open the campaign panel / Export — confirm existing `results/dv026_ollama_campaign/` artifacts without re-running 10×20 unless needed.
4. Optional CLI-only: `hetero-pop`, `info-contrast` (not first-class ribbon buttons yet).

```bash
./build/dv026-workbench-runner hetero-pop 424242 --smoke
./build/dv026-workbench-runner info-contrast 424242
./build/dv026-workbench-runner ollama-campaign 424242 20   # full gate
./build/dv026-workbench-runner phase-i 424242             # D6 reads summary.json
```

## API

| Route | Role |
|---|---|
| `/api/start\|pause\|resume\|reset\|replay\|intervention\|state\|export` | Market Live |
| `POST /api/dv026/run` | Single evidence / Ollama slice-paired |
| `GET /api/dv026/last` · `/export` | Last DV026 payload |
| `POST/GET /api/dv026/batch/*` | Phase I seed sweep |
| `POST/GET /api/dv026/campaign/*` | Ollama campaign |

Live local LLM setup: [`DV026_OLLAMA.md`](DV026_OLLAMA.md).

## Is 1000 seed sweep enough?

| Purpose | Verdict |
|---|---|
| Scripted software-path reproducibility | **Yes** |
| Live multi-provider LLM / DARPA PoC | **No** — use Ollama campaign (10×20), not the batch |

## Claim boundaries (always)

- `constructs_validated: false` (Phase II not claimed)
- `software_green` ≠ DARPA milestone by itself
- No Phase I classifier accuracy metrics; no Phase II intent claims
- Batch summary: scripted Phase I only
