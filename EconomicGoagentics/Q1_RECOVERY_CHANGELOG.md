# Q1 recovery changelog

Completed cell evidence is immutable. Recovery skips completed assignments and refuses to overwrite them. The interrupted run `run_1792582821581_6712` (cells 0–83) was left byte-identical.

## Behavior

- **RUNNING** requires a live process identity (pid + executable name + `hetero-campaign`). File mtimes, including iCloud clocks after reboot, are not used.
- **INTERRUPTED** is the status when completed cells exist, unfinished assignments remain, and no matching process is alive. ETA is cleared.
- Completed `cell_evidence/cell_N.json` is written with temp + fsync + rename. An existing completed file is never replaced.
- Truncated JSON and `*.tmp*` files are incomplete writes, not completed cells.
- Resume executes only unfinished assignments. After resume, completed-cell SHA-256 values must match the pre-resume snapshot.
- Layer A qualification is persisted with config and code fingerprints and is restored only when both still match.
- The Overview interrupted card offers Inspect, Verify, Resume remaining, and Export. Resume is blocked until Verify passes. If resume is impossible, the original observations stay on disk.

## Files

| Path | Change |
|---|---|
| `enterprise_api/campaign_recovery.py` | Inventory, reconcile, verify, skip-completed resume, atomic writes, Layer A persist/restore, original-run protection; lenient parse of original snapshot files that end with a literal `\n` |
| `live_server.py` | Process-identity overlay, session files, recovery/resume APIs, Layer A fingerprint restore |
| `src/HeterogeneousCampaign.cpp` | Incremental atomic cell evidence; skip completed indices; refuse overwrite |
| `src/dv026_workbench_runner.cpp` | `COAGENTICS_RESUME_RUN_DIR`; new `run_*` directories only for non-smoke live campaigns |
| `workbench/interface.html` | Interrupted campaign card |
| `workbench/how.html` | Operator copy for INTERRUPTED |
| `tests/test_q1_recovery.py` | Deterministic recovery/integrity tests, including read-only check of the original 84 cells |
| `Q1_RECOVERY_VALIDATION.md` | Actual test results |

## Not modified

- `results/dv026_heterogeneous_campaign/runs/run_1792582821581_6712/cell_evidence/cell_0.json` … `cell_83.json`
- Matching `cell_observations` / `cell_snapshots` for those indices
- Backup tree `backups/q1_hetero_campaign_20261021T120457`
