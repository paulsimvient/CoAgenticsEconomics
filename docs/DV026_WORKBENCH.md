# DV026 Workbench — Interface + Research Console

```bash
./start-workbench.sh
# GET /                → interface.html (primary NetLogo-style lab)
# GET /live.html       → live.html (full Research Console: Gate · Bids · Method)
# GET /market_lab.html → market_lab.html (secondary guided shell)
# GET /interface.html  → same as /
```

`start-workbench.sh` configures/builds runners in **`build-dv026/`** (`find_exe` may fall back to `build/` if a binary is missing).

## Mental model

| Surface | URL | Job | Success signal |
|---|---|---|---|
| **Interface** | `/` · [`interface.html`](../workbench/interface.html) | Daily driver — watch run, Go/Stop, monitors | Same campaign APIs |
| **Research Console** | `/live.html` · [`live.html`](../workbench/live.html) | Full Gate · Bids · Method · proposal narrative | `darpa_claim_ready` under `scope=local_ollama_poc` |
| **Market Lab** | `/market_lab.html` · [`market_lab.html`](../workbench/market_lab.html) | Secondary guided shell | Same API gates; thinner UI |
| **Market Live** | `/matlab.html` · [`matlab.html`](../workbench/matlab.html) | Paced CDA demo | Demo only — not DARPA |

### Three different “greens”

| Flag | Means | Does **not** mean |
|---|---|---|
| **`software_green`** | Scripted Phase I A/B/C/D checks pass | Live LLM economics or DARPA milestone |
| **`live_llm`** | This run talked to Ollama | Campaign readiness or Phase II |
| **`darpa_claim_ready`** + `scope=local_ollama_poc` | Full Ollama campaign gate passed under current revision | Phase II constructs / commercial 10-provider matrix |

### Two “expectations” (do not conflate)

| Surface | API / fixture |
|---|---|
| **Preregistered hypotheses (freeze)** | `/api/dv026/expectations` · SHA bound before full campaign |
| **Proposal readiness (gates)** | `/api/dv026/proposal-narrative` · fixture [`DV026_PROPOSAL_EXPECTATIONS.json`](DV026_PROPOSAL_EXPECTATIONS.json) |

Full campaign start requires frozen preregistration **and** Layer A qualification (server-enforced; Smoke exempt). Start also requires live runtime (Ollama + installed models).

**Reset** (`POST /api/dv026/reset`) returns the console to workflow start: cancel running campaign, clear READY hydrate, unfreeze hypotheses to draft, clear in-session Layer A qualification (on-disk `summary.json` is ignored until a fresh `layer-a` run), and hide chart/bid aggregates until a new run. Prior `cells.jsonl` / `cell_*.jsonl` files are retained on disk.

Each chart/analysis panel has a **Method** control documenting how aggregates are computed (read-only provenance).

See [`DV026_DARPA_PHASE_I_READINESS.md`](DV026_DARPA_PHASE_I_READINESS.md) for the readiness checklist. Charts aggregate `cells.jsonl` (models × seeds, human η, evidence pipeline). If on-disk `summary.json` has a different `implementation_revision` than the running server, it is **not** hydrated as READY — re-run under the current build.

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

1. Open `http://127.0.0.1:8787/` (Interface) to watch runs; use `/live.html` for science gates.
2. **Qualify market** with **Layer A** (η > 90% every trial) — not Phase I `software_green`.
3. Fill **Preregistered hypotheses** → **Freeze expectations** (required for Full; Smoke exempt).
4. **Start live campaign** (Interface Go or Console) → Inspect seat / evidence.
5. On Research Console, open **Method** on charts for formula provenance.
6. Optional CLI: `hetero-pop`, `info-contrast`, or full `ollama-campaign`.

```bash
./build-dv026/dv026-workbench-runner hetero-pop 424242 --smoke
./build-dv026/dv026-workbench-runner info-contrast 424242
./build-dv026/dv026-workbench-runner ollama-campaign 424242 20   # full gate
./build-dv026/dv026-workbench-runner phase-i 424242             # D6 reads summary.json
```

## API

| Route | Role |
|---|---|
| `GET /` · `/interface.html` | Interface (`interface.html`) — primary |
| `GET /live.html` | Research Console (`live.html`) |
| `GET /market_lab.html` | Market Lab (`market_lab.html`) — secondary |
| `GET /matlab.html` | Market Live paced CDA demo (not DARPA readiness) |
| `GET /api.js` | Shared Interface/Console fetch helpers |
| `POST /api/dv026/reset` | Console Reset (workflow start; does not delete evidence files) |
| `GET/POST /api/dv026/expectations` · `/freeze` | Preregistered hypotheses |
| `GET /api/dv026/proposal-narrative` | Proposal readiness (gates) |
| `POST/GET /api/dv026/campaign/*` | Ollama campaign (start rejects unfrozen Full) |
| `POST /api/dv026/run` | Single evidence / Ollama slice-paired |
| `GET /api/dv026/last` · `/export` | Last DV026 payload |
| `POST/GET /api/dv026/batch/*` | Phase I seed sweep |
| `/api/start\|pause\|resume\|…` | Market Live (`matlab.html` only) |

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
