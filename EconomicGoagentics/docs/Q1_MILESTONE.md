# Q1 milestone — independently verifiable multi-LLM trading

Git checkpoint before these changes: `a81e979`. Original campaign run `run_1792582821581_6712` is never overwritten.

## Audit (read-only)

- Engine: 10 LLM seats, even seats buy, odd seats sell, private values `120-2*seat` / `80+2*seat`, CDA midpoint match, cash 10000, sellers start with 1 unit.
- Live path: Ollama OpenAI-compatible transport when `use_live_ollama` and catalog reachable; mock JSON transport only for smoke/non-live.
- Archive: 120 `cell_evidence` files with `status=completed`. Observations 0–83 are native JSON. Observations 84–119 were written with a bid-field quote regression (`"price":90","side"`). Snapshots after cell 35 can contain trailing extra JSON. Repair is on-read only.
- Recovery: `atomic_write_*` refuses overwrite; completed cells are skipped on resume.

## What this milestone verifies

1. All 120 completed archive cells are 5-buyer / 5-seller Ollama markets. Independent replay finds genuine Ollama decisions on both sides of every archived trade. A separate C++ test (`heterogeneous_five_five_mock`) is labeled **mock inference** only.
2. Featured transcript: `results/q1_milestone/featured_transcript.json` (cell 6, `qwen3:8b` buyer vs `codellama:latest` seller, prompts and raw responses retained).
3. Independent Python CDA replay: 120/120 trade lists match, 120/120 cash and inventory conserved, 120/120 induced-value η match the engine. 20 cells have extra cash-financed units that are real trades but do not reuse the seat's single redemption value.
4. Matched comparison: 60 pairs of `(seed, repeat)` frozen vs sequential. Mean η frozen 57.5%, sequential 67.4%, paired difference +9.8 percentage points sequential. Descriptive only.
5. Visualize scatter/histogram η come from `cell_evidence`. Floor/tape come from `cell_observations` (on-read repair for cells 84–119). No fabricated series. Running `live_server.py` (pid 29061) still serves the previous Python until the operator restarts it **after** the runner child (pid 29386) exits.
6. Automated tests: 25 Python tests OK (1 skipped: backup inventory missing); C++ `market_conservation_smoke`, `heterogeneous_campaign_smoke`, `heterogeneous_five_five_mock` passed.

## Qualification procedure

From `EconomicGoagentics/`:

```
python3 -m unittest tests.test_q1_recovery tests.test_q1_multi_llm_qualification
python3 tools/q1_qualification.py
```

C++ (do not relink a live `dv026-workbench-runner` unless the campaign process has exited):

```
cmake --build build-dv026 --target market_conservation_smoke --target heterogeneous_five_five_mock
ctest --test-dir build-dv026 -R 'market_conservation_smoke|heterogeneous_five_five_mock|heterogeneous_campaign_smoke' --output-on-failure
```

Derived reports go to `results/q1_milestone/`. That directory is not the original run.

## Scientific gate

**Not passed.** Wilcoxon on these 60 pairs is post-hoc: descriptive means were inspected before the plan was locked. Native JSON for cells 84–119 is a derived corpus under `results/q1_milestone/native_archive/`; original archive bytes are preserved.

Engineering qualification can pass when tests, dual-sided Ollama 5+5 checks, conservation, induced-value η match, and the native corpus are all green. That is not a confirmatory scientific claim.

## Remaining limitations

- Observation writer regression is fixed for **future** cells only; existing 84–119 files are left byte-identical and parsed with on-read repair (36 files).
- Snapshot extra-data is ignored on read, not rewritten.
- Some frozen cells let a buyer fill extra units with leftover cash. Those trades are in the tape; induced-value η counts only the first unit per seat.
- 84 snapshot files end with a literal two-character `\n` suffix. Read-only decode strips it; files are not rewritten.
- `heterogeneous_five_five_mock` and `ten_model_campaign_smoke` are **mock inference**, not Ollama.
- Live Ollama was not started as a competing campaign while `dv026-workbench-runner` pid 29386 is still the child of `live_server.py`.
- The process currently bound to port 8787 will not show on-read repair or evidence-file chips until that server is restarted after the runner exits.
- Matched comparison has no preregistered inferential test.
- No CUI or government-sensitive content is used or produced.
