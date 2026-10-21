# Q1 recovery validation

**Date:** 2026-10-21  
**Scope:** Execution recovery / integrity for the interrupted heterogeneous campaign.  
**Original run:** `results/dv026_heterogeneous_campaign/runs/run_1792582821581_6712`  
**Backup:** `backups/q1_hetero_campaign_20261021T120457` (inventory SHA-256 `dc24efc2…`)

Completed cells from the interrupted run are irreplaceable observations. A new experiment can always be started. Those 84 cells cannot be reconstructed.

## What was not done

- The original 120-cell campaign was **not** resumed, relaunched, or rewritten.
- `cell_0.json` … `cell_83.json` were **not** overwritten.
- No live Ollama recovery of cells 84–119 was attempted.

## Deterministic tests

Command:

```sh
python3 -m unittest tests.test_q1_recovery -v
```

Result: **16 tests, OK** (2026-10-21).

| Test | Result |
|---|---|
| Original run still has cells 0–83 | pass |
| Original completed cells match backup SHA-256 | pass |
| Reconcile with `persist=False` does not mutate original cells | pass |
| RUNNING requires live pid + exe + `hetero-campaign` token | pass |
| Future/iCloud mtimes do not imply RUNNING | pass |
| Live identity ⇒ RUNNING | pass |
| Atomic write refuses to overwrite a completed cell | pass |
| Truncated JSON is corrupt, not completed; originals unchanged | pass |
| `*.tmp*` orphans are detected | pass |
| Resume executes only unfinished assignments; completed hashes unchanged | pass |
| Unsafe resume raises and leaves completed files byte-identical | pass |
| Duplicate/mismatched cell files block resume | pass |
| Layer A fingerprint mismatch is stale (not a pass) | pass |
| Matching Layer A fingerprints restore | pass |
| Status display `Interrupted — n/N cells completed` | pass |

## C++ checks

| Check | Result |
|---|---|
| `dv026-workbench-runner` rebuild | pass |
| `heterogeneous_campaign_smoke` rebuild + run | pass |

Smoke uses mock transports only. It is not live-model evidence.

## Original 84/120 inventory (read-only)

- 84 completed evidence files, indices 0–83, JSON-ok, `status=completed`
- Matching observation archives and snapshots for those indices (snapshots end with a literal `\n` two-character suffix; inventory parses that without rewriting the files)
- 36 unfinished assignments (84–119)
- No `run_manifest.json` (reconstructed in memory as 20 seeds × 2 designs × 3 repeats)
- Backup hashes of `cell_evidence/cell_*.json` still match the live originals

## Resume policy

Resume is eligible only after Verify passes. Resume skips completed indices and refuses to replace existing `cell_evidence/cell_N.json`. If Verify fails, the original 84 cells stay on disk and a new experiment may be run later.

Live Verify against `run_1792582821581_6712` on 2026-10-21 returned `resume_eligible=true` with 84 protected indices. **Resume was not launched.** The 84 completed cells remain byte-identical to the backup.

RUNNING is never inferred from file timestamps.
