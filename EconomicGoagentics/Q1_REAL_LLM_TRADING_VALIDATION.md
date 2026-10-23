# Q1 — Real LLM-to-LLM trading validation

**Verdict: PASS**

Date: 2026-10-22  
Repository: nested `EconomicGoagentics/` (current local tree; not replaced by an older ZIP)  
Git checkpoint before live work: `e6a89f2aba46a7fdf68cd1bd601da768f38e99c8`  
Protected original archive: `run_1792582821581_6712` (120 cells; hashes unchanged)  
Qualification market: `run_1792670868835_39411` (new run directory; original archive not resumed)

This document reports independently checked records. It does not mint a scientific claim. `scientific_gate` remains **not passed**. Wilcoxon / frozen-vs-sequential inference remains post-hoc.

---

## Verdict against acceptance criteria

| Criterion | Result |
|---|---|
| All ten agents use real Ollama inference | **PASS** — 10 distinct local tags, live transport |
| Complete market finishes successfully | **PASS** — runner exit 0, cell status `completed` |
| ≥1 executed trade with verified LLM buyer and LLM seller | **PASS** — 3 / 3 fills |
| Settlement and surplus calculations correct | **PASS** — independent η = 45.4545% matches archive |
| Evidence survives application restart | **PASS** — original cell hashes unchanged after UI reload; qualification files remain on disk; `/api/dv026/lab/cells` still serves `run_1792582821581_6712` |
| UI reproduces archived results | **PASS** — Visualize cell 6 tape and η match `cell_observations/cell_6.json`; clicking the first trade shows both archived model orders |

Phase 4 (paired public-signal contrast) completed as a **descriptive** 2-seat protocol experiment with **zero trades**. That slice is not a causal finding and is not a 10-agent market.

---

## Phase 1 — Inspect and preserve

Inspected in the nested tree:

- C++ engine / campaign: `src/HeterogeneousCampaign.cpp`, `src/dv026_workbench_runner.cpp`
- Production runner binary: `build-dv026/dv026-workbench-runner`
- Ollama live path (no mock agents in `hetero-campaign`)
- UI: `workbench/interface.html` served at `http://127.0.0.1:8789/interface.html`
- Original 120-cell archive (the earlier 84 completed cells are a subset of this archive):  
  `results/dv026_heterogeneous_campaign/runs/run_1792582821581_6712/`

Original evidence spot hashes at checkpoint and after this session (byte-identical):

| File | SHA-256 |
|---|---|
| `cell_evidence/cell_0.json` | `0c9736ad930f114c322d9b71526d4c87427595bf427ea7c028f0412d5218ebb4` |
| `cell_evidence/cell_6.json` | `78c95a3a9dcbd037c8bc83922d258cad63a7e652b8ce4281cb62a937425fffb0` |
| `cell_evidence/cell_83.json` | `ea414e139941f227165c58d679b755785e5b2037c6f4399a688e4f87f96fcf6f` |

Archived evidence was not rewritten. Qualification output was written only to a **new** run directory and to `results/q1_real_llm_trading/`.

---

## Phase 2 — Real 10-agent Ollama CDA

### Command

Working directory: `EconomicGoagentics/EconomicGoagentics`

```bash
export COAGENTICS_INFERENCE_REPEATS=1
export COAGENTICS_ACTIVATION_DESIGN=sequential_interaction
# COAGENTICS_RESUME_RUN_DIR unset  → new run directory
# COAGENTICS_PROTOCOL_ID unset     → 10 seats, not the 2-seat protocol path
./build-dv026/dv026-workbench-runner hetero-campaign 20261022 1
```

Runner stdout (truncated JSON) is at `results/q1_real_llm_trading/hetero-campaign-stdout.json`. Exit code 0.

### Configuration

| Field | Value |
|---|---|
| Seed | `20261022` |
| Design | `sequential_interaction` |
| Agents | 10 (seats 0,2,4,6,8 buyers; 1,3,5,7,9 sellers) |
| Mechanism | shared continuous double auction |
| Fundamental | 100 |
| Public signal (this cell) | 0 |
| Inference repeats | 1 |
| Provider | Ollama 0.18.2 at localhost:11434 |
| Mock / scripted / synthetic responses | none |

### Models used (Ollama tags and image IDs at audit)

`ollama version is 0.18.2`

| Seat | Role | Private value / cost | Model tag | Image ID |
|---|---|---|---|---|
| LLM-SEAT-0 | buyer | 120 | `llama3:latest` | `365c0bd3c000` |
| LLM-SEAT-1 | seller | 82 | `llama2:latest` | `78e26419b446` |
| LLM-SEAT-2 | buyer | 116 | `llama3.2:3b` | `a80c4f17acd5` |
| LLM-SEAT-3 | seller | 86 | `qwen3:8b` | `500a1f067a9f` |
| LLM-SEAT-4 | buyer | 112 | `qwen2.5-coder:7b` | `dae161e27b0e` |
| LLM-SEAT-5 | seller | 90 | `llama3.1:8b` | `46e0c10c039e` |
| LLM-SEAT-6 | buyer | 108 | `codellama:latest` | `8fdf8f752f6e` |
| LLM-SEAT-7 | seller | 94 | `mistral:latest` | `6577803aa9a0` |
| LLM-SEAT-8 | buyer | 104 | `gemma2:2b` | `8ccf136fdd52` |
| LLM-SEAT-9 | seller | 98 | `phi3:mini` | `4f2222927938` |

Private values follow the induced-value schedule `120-2*seat` (buyers) / `80+2*seat` (sellers). They were not altered to force a trade.

### Counts (qualification cell 0)

| Quantity | N |
|---|---|
| Real inference calls | 14 |
| Submitted orders | 7 |
| Executed trades | 3 |
| Verified LLM-to-LLM trades | 3 |
| HOLD / abstain turns | present (`llama3:latest`, `llama3.1:8b`) |
| BUY and SELL actions | present |

Independent proof: `python3 tools/q1_prove_hetero_cell.py` → `results/q1_real_llm_trading/live_cell0_prove.json`.  
Observation JSON required **on-read repair** of a `"price":90","side"` quote regression. The archive file was **not** rewritten (`wrote_archive: false`).

---

## Phase 3 — Prove the transactions

Every fill in the qualification cell has independently verifiable buyer and seller Ollama turns.

### Trade 0 (qualifying LLM-to-LLM)

Human-readable trace: `results/q1_real_llm_trading/TRACE_trade0.md`

| Field | Buyer | Seller |
|---|---|---|
| Identity | LLM-SEAT-2 | LLM-SEAT-3 |
| Model | `llama3.2:3b` | `qwen3:8b` |
| Request id | `ollama:llama3.2:3b:20261022:0` | `ollama:qwen3:8b:20261022:0` |
| Latency | 2545 ms | 11541 ms |
| Raw response | `{"action":"BUY",...,"price":90}` | `{"action":"SELL",...,"price":90}` |
| Submitted order | buy 1 @ 90 | sell 1 @ 90 |
| Private value / cost | 116 | 86 |
| Surplus | 26 | 4 |
| Execution | qty 1 @ **90**, t = 0 | |

### Trade 1

| Field | Buyer | Seller |
|---|---|---|
| Identity | LLM-SEAT-6 | LLM-SEAT-7 |
| Model | `codellama:latest` | `mistral:latest` |
| Price / qty | 90 / 1 | |
| Private value / cost | 108 | 94 |
| Surplus | 18 | **−4** |
| Total surplus | 14 | |

Seller surplus is negative because the seller’s accepted offer (90) is below private cost (94). That is recorded LLM behavior, not a settlement bug. It is included in η.

### Trade 2

| Field | Buyer | Seller |
|---|---|---|
| Identity | LLM-SEAT-8 | LLM-SEAT-9 |
| Model | `gemma2:2b` | `phi3:mini` |
| Price / qty | 90 / 1 | |
| Private value / cost | 104 | 98 |
| Surplus | 14 | **−8** |
| Total surplus | 6 | |

### Independent economics

Competitive surplus from the first-unit induced values (gains from trade for units with buyer value ≥ seller cost, in the usual pairing): **110**.  
Realized surplus: 30 + 14 + 6 = **50**.  
η = 50 / 110 = **45.4545…%**.  
Archive `efficiency`: **45.4545**. Match: yes.

Cash conservation: Σ cash = 100000 (initial 10 × 10000).  
Inventory conservation: Σ inventory = 5 (initial five seller units).  
No negative cash or inventory.

---

## Phase 4 — Paired information contrast (descriptive only)

Protocol `q1-real-llm-info-v1`:

- Manipulated variable: `public_signal` shown to agents (control **100**, treatment **70**)
- Unchanged: fundamental **100**, private values, roles, mechanism, seed
- Path: `COAGENTICS_PROTOCOL_ID` production runner via `tools/av1_execute_market_plan.py`
- **Limitation:** this protocol path is a **2-seat** market (requested buyer model + `llama3:latest` seller), not the 10-agent CDA

Command:

```bash
PYTHONPATH=tools python3 tools/av1_execute_market_plan.py \
  results/q1_real_llm_trading/info_plan.json \
  build-dv026/dv026-workbench-runner \
  results/q1_real_llm_trading/info_contrast
```

Exit 0. 8 / 8 cells completed. 32 / 32 inference turns parsed.

### Recorded outcomes (not a causal estimate)

| Pair | Seed | Buyer model | Control bid | Treatment bid | Seller (`llama3:latest`) | Trades |
|---|---|---|---|---|---|---|
| 0 / 1 | 20261022 | `qwen3:8b` | BUY @ 100 | BUY @ 70 | HOLD, HOLD | 0 / 0 |
| 2 / 3 | 20261022 | `codellama:latest` | BUY @ 90 | BUY @ 90 | HOLD, HOLD | 0 / 0 |
| 4 / 5 | 20261023 | `qwen3:8b` | BUY @ 100 | BUY @ 70 | HOLD, HOLD | 0 / 0 |
| 6 / 7 | 20261023 | `codellama:latest` | BUY @ 90 | BUY @ 90 | HOLD, HOLD | 0 / 0 |

**Descriptive observation:** under this protocol, `qwen3:8b` bid the displayed public signal in both seeds; `codellama:latest` bid 90 in both conditions; the seller held in every cell; **no trade executed**. Efficiency is 0 because there are no fills. Mean absolute price deviation is missing (not estimated).

**Do not claim** that public_signal causes, or fails to cause, trading. n = 4 pairs, seller always the same model, 2 seats. This is an information-intervention **engineering check**, not a confirmatory experiment.

---

## Phase 5 — Interface

Read-only UI: `http://127.0.0.1:8789/interface.html`  
API: `GET /api/dv026/lab/cells` → `run_id=run_1792582821581_6712`, `n_completed=120`, `read_only=true`, `protected=true`.

Visualize is bound to the **original** archive by design (`ORIGINAL_RUN_ID`). It does **not** display the new qualification run. That is preservation, not a missing-data invention.

### Checks

| Check | Result |
|---|---|
| Displayed Market 6 trades = archived observation | **PASS** — Agent 2↔5 @ 95; 0↔7 @ 90; 6↔1 @ 90; 8↔3 @ 90 |
| η | UI 87.3% = archive 87.2727 |
| Click trade → both model-generated orders | **PASS after smallest UI fix** (see below) |
| Replay | Play advanced event index 16 → 18 on the archived stream; no new observations |
| Graphs | `graphs_backed_by_observation=true`, `graphs_backed_by_evidence=true`; η from `cell_evidence` |
| Missing data | empty / “—” / “not recorded”; not interpolated |
| Live vs completed vs failed vs interrupted | Visualize `ARCHIVE` / `Archive complete · 120/120`; `campaignMode()` distinguishes running / interrupted / complete / stopped. Campaign parent on 8787 was not started. **Resume remaining was not clicked.** |

Smallest UI corrective change (this session, cream Visualize only):

1. `renderLabEvidence()` now shows **buyer and seller** archived turns when a trade is selected.
2. `#labEvidencePanel` is no longer `display:none` in `html.lab-light`.

Without (1) the panel showed only one counterparty. Without (2) the panel was hidden in the cream lab. Archived bytes were not modified.

Screenshots:

- `results/q1_real_llm_trading/ui_archive_120.png` — archive 120/120, original run id
- `results/q1_real_llm_trading/ui_market6_selected.png` — Market 6 selected, η 87.3%, 4 trades
- `results/q1_real_llm_trading/ui_market6_tape.png` — tape, TRADE Agent 2 ↔ Agent 5 @ $95 selected
- `results/q1_real_llm_trading/ui_market6_both_orders.png` — buyer `qwen3:8b` BUY @ 100 and seller `codellama:latest` SELL @ 90 for that archived fill

---

## Phase 6 — Counts for the qualification run

| Item | Value |
|---|---|
| Commands | see Phase 2 and Phase 4 |
| Models | 10 Ollama tags listed above |
| Market / seed | 10-agent CDA, seed `20261022`, sequential_interaction |
| Real inference calls | 14 |
| Submitted orders | 7 |
| Executed trades | 3 |
| Verified LLM-to-LLM trades | 3 |
| Independent η | 45.4545% (matches archive) |
| Control/treatment | 8 protocol cells; 0 trades; descriptive only |

### Evidence SHA-256

Paths relative to `EconomicGoagentics/EconomicGoagentics/`.

| Path | SHA-256 |
|---|---|
| `results/q1_real_llm_trading/live_cell0_prove.json` | `346b90f2df1816063b7cd97bf4853f7cbbf9e09e6024c94d5f7a82153a23346d` |
| `results/q1_real_llm_trading/TRACE_trade0.md` | `53c177fb1e8084ddd4f2722d272d5a4ce5e4fcb7708f8d65bb25cfb5f63f2e6a` |
| `results/dv026_heterogeneous_campaign/runs/run_1792670868835_39411/cell_evidence/cell_0.json` | `bbab0c9e335b3e8035bad35f70a351c3e6c6a8e5d176e860738b7e1efd6c2057` |
| `results/dv026_heterogeneous_campaign/runs/run_1792670868835_39411/cell_observations/cell_0.json` | `4d3ecd49968f9f147840a0d05090bfa462be673d1f0cc91a313f4d0a07cead33` |
| `results/dv026_heterogeneous_campaign/runs/run_1792670868835_39411/cell_snapshots/cell_0.json` | `b3868fc29a87e4f944ea786ee4f5f7730c6cb035aee7cf416a05837ae2d5160b` |
| `results/dv026_heterogeneous_campaign/runs/run_1792582821581_6712/cell_evidence/cell_0.json` | `0c9736ad930f114c322d9b71526d4c87427595bf427ea7c028f0412d5218ebb4` |
| `results/dv026_heterogeneous_campaign/runs/run_1792582821581_6712/cell_evidence/cell_6.json` | `78c95a3a9dcbd037c8bc83922d258cad63a7e652b8ce4281cb62a937425fffb0` |
| `results/dv026_heterogeneous_campaign/runs/run_1792582821581_6712/cell_observations/cell_6.json` | `fb8a482363cc93c10d51bbbbb51822f69899d25f18997f1e5eab55583b5af8da` |
| `results/q1_real_llm_trading/info_plan.json` | `d943566fd22f619964debfc41c02b6abd395118eeb7f85c624d25b0795a91124` |
| `results/q1_real_llm_trading/info_protocol.md` | `3a6183105572d8dfa50838d3b85df419c69dbbc357e900c7972709376dc81f06` |
| `results/q1_real_llm_trading/info_contrast/manifest.json` | `c77726d91f5796e5b1ac6492aee126eb5010ab14caa4f710d06ed3d56411031b` |
| `results/q1_real_llm_trading/ui_archive_120.png` | `d28da15df4c090bdf7a73a7410770068f784a343fc013690bfefd54f4c72005a` |
| `results/q1_real_llm_trading/ui_market6_tape.png` | `a6fbbbf4f6c3a5e51972262ec2b53f10a15d60f8f611205f4c53f6187bd0855d` |
| `results/q1_real_llm_trading/ui_market6_both_orders.png` | `bcc05926e0dbe746f9953a7b5f1c761502875f38b07c7fc3c5b1534179489178` |
| `results/q1_real_llm_trading/ui_market6_selected.png` | `54724b522de4e351a90d497b3fa2be37a39aef5d17f2bafd1ee2c584c8c4fa1b` |

Full inventory: `results/q1_real_llm_trading/q1_file_hashes.json`.

---

## Tests and failures

| Item | Result |
|---|---|
| Independent prover on qualification cell 0 | `qualified_llm_to_llm: true`, `errors: []` |
| Independent prover dry-run on native cell 6 | 4 LLM-to-LLM trades, η 87.2727 match (supporting; not the live Q1 cell) |
| Info-contrast archive audit | 8/8 cells, 32/32 parse_ok |
| Observation JSON of live cell 0 | invalid as raw UTF-8 JSON (`"price":90","side"`); **on-read repair only** |
| Info-contrast trades | **0** — not a test failure; recorded market outcome |
| Protocol path agent count | 2 seats, not 10 — documented limitation |
| Cream UI before this session | trade click showed one side; evidence panel hidden — **fixed in `interface.html`** |
| `scientific_gate` | still **not passed** |

No mock agents, scripted decisions, synthetic LLM responses, or fabricated fills were used in the qualification cell.

---

## Remaining limitations

1. Live observation writer can emit a broken quote around `"price"` / `"side"`. Proof uses on-read repair; archive bytes of the qualification observation were left as written.
2. Visualize binds the protected original run, not `run_1792670868835_39411`. Qualification trading is proven from files, not from the cream scatter.
3. Original `cell_evidence` documents omit seat→model assignment; assignment is recovered from `agent_turns` + canonical requests.
4. Phase 4 is a 2-seat protocol, not a 10-agent information RCT. Zero trades. No causal language.
5. Some fills have negative seller surplus (LLM sold below cost). Economics are reported as recorded.
6. Frozen vs sequential Wilcoxon remains post-hoc. This Q1 does not reopen that claim.
7. Footer still mentions `127.0.0.1:8787`. That campaign parent was not restarted.
8. A leftover instrument bar still says “PEER OFF / PEER ON”; the archived campaign contrast that Visualize actually plots is frozen_snapshot vs sequential_interaction.

---

## Human-readable transaction trace (qualification trade 0)

See `results/q1_real_llm_trading/TRACE_trade0.md` for the full canonical requests.

Buyer `LLM-SEAT-2` (`llama3.2:3b`) independently emitted BUY @ 90.  
Seller `LLM-SEAT-3` (`qwen3:8b`) independently emitted SELL @ 90.  
The C++ engine matched qty 1 at 90. Buyer surplus 26, seller surplus 4, total 30.
