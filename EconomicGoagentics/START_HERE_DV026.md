# EconomicGoagentics — DV026 entry

## Start

From the repository root:

```bash
./start-workbench.sh
```

Then open:

| URL | UI | Use for |
|---|---|---|
| `http://127.0.0.1:8787/` | **Interface** ([`workbench/interface.html`](workbench/interface.html)) | Daily driver — NetLogo-style Setup · World · Monitors |
| `http://127.0.0.1:8787/live.html` | Research Console ([`workbench/live.html`](workbench/live.html)) | Full Gate · Bids · Method · proposal readiness |
| `http://127.0.0.1:8787/market_lab.html` | Market Lab ([`workbench/market_lab.html`](workbench/market_lab.html)) | Secondary guided shell |

**Primary entry is Interface at `/`.** Readiness claims and Method provenance live on `/live.html`. Market Lab is secondary — do not treat Phase I `software_green` as market Layer A qualification.

Legacy `workbench/matlab.html` is not the DV026 readiness entry.

The start script **reconfigures and rebuilds** runners in `build-dv026/`.

```bash
./start-workbench.sh 8788   # alternate port
```

## First run (easiest)

1. Open `http://127.0.0.1:8787/` (Interface).
2. Press **Go** — Smoke · 2 seeds · defaults already set (no freeze, no Layer A).
3. Watch seats move; click a seat or **Inspect** for evidence on Research Console.

## Workflow (full campaign)

1. **Reset** (Research Console) — return to workflow start (cancels a running campaign, clears READY display, clears in-session Layer A so qualify must be re-run, unfreezes hypotheses; prior evidence files stay on disk).
2. **Qualify market** — Layer A (`layer-a`: CDA + sealed-bid, every trial η > 90%). Required server-side for Full campaigns (Smoke exempt); on-disk summary is not reused after Reset.
3. **Accept H01 defaults & freeze** — or edit then freeze. Required for Full mode.
4. **Start live campaign** — model × seed cells; inspect bids.
5. Read **Proposal readiness (gates)** — separate from the freeze form.
6. Use **Method** on chart/table panels (Research Console) for formula provenance.

**Smoke** skips freeze and Layer A — software path only, not a DARPA readiness claim.

## Live LLM runtime

- **READY** — runner works, Ollama responds, and at least one configured model is installed.
- **OFFLINE** — Ollama is not reachable.
- **ERROR** — runner / runtime check failed.

## Two different “expectations”

| Surface | What it is |
|---|---|
| **Preregistered hypotheses (freeze)** | Locked research questions / analysis plan before observation |
| **Proposal readiness (gates)** | Auto-scored white-paper gate items from Layer A + `cells.jsonl` |

If a frozen expectations file fails SHA-256 verification, the API surfaces `integrity_error` and Full campaigns are blocked until you re-save / re-freeze.

## Stale on-disk summary

Mismatched `implementation_revision` on `summary.json` is not loaded as READY. After **Reset**, on-disk cells remain but charts/bids hide them until a new campaign run.
