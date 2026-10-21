"""Deterministic Q1 recovery/integrity tests.

These tests never launch the interrupted original 120-cell campaign and never
write into results/dv026_heterogeneous_campaign/runs/run_1792582821581_6712.
Completed original cells are treated as irreplaceable observations.
"""
from __future__ import annotations

import json
import os
import tempfile
import time
import unittest
from pathlib import Path

from enterprise_api import campaign_recovery as recovery

ROOT = Path(__file__).resolve().parents[1]
ORIGINAL_RUN = ROOT / "results" / "dv026_heterogeneous_campaign" / "runs" / recovery.ORIGINAL_RUN_ID
BACKUP_SUMS = ROOT / "backups" / "q1_hetero_campaign_20261021T120457" / "SHA256SUMS.json"


def _cell(index, seed, repeat, design, **extra):
    doc = {
        "schema_version": 1,
        "index": index,
        "seed": seed,
        "repeat": repeat,
        "design": design,
        "status": "completed",
        "error": "",
        "efficiency": 10.0 + index,
        "trades": 1,
        "agents": 2,
        "observed_llm_turns": 2,
        "worker_id": 0,
        "elapsed_seconds": 1.0,
    }
    doc.update(extra)
    return doc


def _obs(index, seed, repeat, design):
    return {
        "schema_version": 1,
        "index": index,
        "seed": seed,
        "repeat": repeat,
        "design": design,
        "trades": [],
        "bids": [],
        "agent_turns": [{"model": "mock-a", "agent_id": "LLM-SEAT-0"}],
    }


def _write_run(run_dir: Path, n_completed: int, planned: int = 8, base: int = 424242):
    designs = list(recovery.DESIGNS)
    repeats = 2
    n_seeds = planned // (len(designs) * repeats)
    plan = recovery.assignment_plan(base, n_seeds, repeats, designs)
    cfg = {
        "campaign_id": run_dir.name,
        "base_seed": base,
        "n_seeds": n_seeds,
        "repeats": repeats,
        "designs": designs,
        "population_design": "heterogeneous_multi_llm",
        "activation_design": "both",
    }
    recovery.atomic_write_json(run_dir / "run_manifest.json", {
        **cfg,
        "cells_planned": planned,
        "config_fingerprint": recovery.config_fingerprint(cfg),
    })
    for cell in plan[:n_completed]:
        recovery.write_completed_cell(
            run_dir,
            _cell(cell["index"], cell["seed"], cell["repeat"], cell["design"]),
            _obs(cell["index"], cell["seed"], cell["repeat"], cell["design"]),
            {"agents": 2, "trades": 1, "efficiency": 10.0},
        )
    return plan


class Q1RecoveryTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.run = Path(self.tmp.name) / "run_test"
        self.run.mkdir()

    def test_running_requires_live_process_identity(self):
        _write_run(self.run, 4)
        recovery.write_session(self.run, {
            "pid": 999999,
            "exe": "/tmp/dv026-workbench-runner",
            "cmdline": "dv026-workbench-runner hetero-campaign 424242 2",
            "command_token": "hetero-campaign",
            "status": "running",
        })
        rec = recovery.reconcile(self.run, inspect=lambda pid: None, persist=False)
        self.assertFalse(rec["running"])
        self.assertEqual(rec["execution_state"], "interrupted")
        self.assertIsNone(rec["eta_sec"])

    def test_mtime_does_not_imply_running(self):
        _write_run(self.run, 4)
        future = time.time() + 86400
        for p in (self.run / "cell_evidence").glob("cell_*.json"):
            os.utime(p, (future, future))
        rec = recovery.reconcile(self.run, inspect=lambda pid: None, persist=False)
        self.assertFalse(rec["running"])
        self.assertTrue(rec["interrupted"])

    def test_live_identity_is_running(self):
        _write_run(self.run, 2)
        sess = {
            "pid": 4242,
            "exe": "/opt/dv026-workbench-runner",
            "command_token": "hetero-campaign",
            "status": "running",
        }
        recovery.write_session(self.run, sess)
        rec = recovery.reconcile(self.run, inspect=lambda pid: {
            "pid": pid, "cmdline": "/opt/dv026-workbench-runner hetero-campaign 424242 2", "alive": True,
        }, persist=False)
        self.assertTrue(rec["running"])
        self.assertFalse(rec["interrupted"])

    def test_atomic_write_refuses_overwrite(self):
        _write_run(self.run, 1)
        before = recovery.snapshot_completed_hashes(self.run)
        with self.assertRaises(FileExistsError):
            recovery.write_completed_cell(
                self.run,
                _cell(0, 424242, 0, "frozen_snapshot", efficiency=99),
                _obs(0, 424242, 0, "frozen_snapshot"),
            )
        self.assertEqual(recovery.snapshot_completed_hashes(self.run), before)

    def test_truncated_json_is_corrupt_not_completed(self):
        _write_run(self.run, 2)
        bad = self.run / "cell_evidence" / "cell_2.json"
        bad.write_text('{"index":2,"status":"completed","seed":424242')
        inv = recovery.inventory_run(self.run)
        self.assertIn("cell_2.json", inv["corrupt_files"])
        self.assertEqual(inv["completed"], 2)
        before = dict(inv["completed_sha256"])
        v = recovery.verify_recovery(self.run, persist=False)
        self.assertFalse(v["resume_eligible"])
        self.assertEqual(recovery.snapshot_completed_hashes(self.run), before)

    def test_tmp_orphan_detected(self):
        _write_run(self.run, 2)
        (self.run / "cell_evidence" / "cell_2.json.tmp.1").write_text("{")
        inv = recovery.inventory_run(self.run)
        self.assertTrue(inv["tmp_orphans"])

    def test_skip_completed_on_resume(self):
        plan = _write_run(self.run, 4, planned=8)
        executed = []

        def execute_cell(cell, run_dir):
            executed.append(cell["index"])
            recovery.write_completed_cell(
                run_dir,
                _cell(cell["index"], cell["seed"], cell["repeat"], cell["design"]),
                _obs(cell["index"], cell["seed"], cell["repeat"], cell["design"]),
                {"agents": 2, "trades": 1, "efficiency": 10.0},
            )

        before = recovery.snapshot_completed_hashes(self.run)
        out = recovery.execute_resume(self.run, execute_cell, inspect=lambda pid: None)
        self.assertEqual(executed, [c["index"] for c in plan[4:]])
        self.assertEqual(out["unchanged_completed"], list(before))
        after = recovery.snapshot_completed_hashes(self.run)
        for idx, digest in before.items():
            self.assertEqual(after[idx], digest)

    def test_resume_blocked_leaves_originals(self):
        _write_run(self.run, 4)
        (self.run / "cell_evidence" / "cell_1.json").write_text("{not-json")
        before = {
            i: recovery.sha256_file(self.run / "cell_evidence" / f"cell_{i}.json")
            for i in (0, 2, 3)
        }

        def execute_cell(cell, run_dir):
            raise AssertionError("must not execute when recovery is unsafe")

        with self.assertRaises(ValueError):
            recovery.execute_resume(self.run, execute_cell, inspect=lambda pid: None)
        for i, digest in before.items():
            self.assertEqual(recovery.sha256_file(self.run / "cell_evidence" / f"cell_{i}.json"), digest)

    def test_duplicate_index_fails_verify(self):
        _write_run(self.run, 2)
        dup = self.run / "cell_evidence" / "cell_99.json"
        dup.write_text((self.run / "cell_evidence" / "cell_0.json").read_text().replace('"index":0', '"index":0'))
        # Force a second file claiming index 0.
        data = json.loads((self.run / "cell_evidence" / "cell_0.json").read_text())
        recovery.atomic_write_json(self.run / "cell_evidence" / "cell_0b.json", data, overwrite=False)
        # Filename mismatch is an inventory error; resume must stay blocked.
        v = recovery.verify_recovery(self.run, persist=False)
        self.assertFalse(v["resume_eligible"])

    def test_layer_a_fingerprint_mismatch_is_stale(self):
        root = Path(self.tmp.name)
        recovery.persist_layer_a_gate(
            root,
            {"layer_a_pass": True, "pass": True, "mean_efficiency_cda": 99},
            config_fingerprint="aaa",
            code_fingerprint="bbb",
            evidence_path=None,
        )
        out = recovery.restore_layer_a_gate(root, config_fingerprint="aaa", code_fingerprint="CHANGED")
        self.assertTrue(out["stale"])
        self.assertFalse(out["pass"])

    def test_layer_a_matching_fingerprints_restore(self):
        root = Path(self.tmp.name)
        recovery.persist_layer_a_gate(
            root,
            {"layer_a_pass": True, "pass": True, "mean_efficiency_cda": 91.0},
            config_fingerprint="cfg",
            code_fingerprint="code",
            evidence_path=None,
        )
        out = recovery.restore_layer_a_gate(root, config_fingerprint="cfg", code_fingerprint="code")
        self.assertTrue(out["restored"])
        self.assertTrue(out["layer_a_pass"])

    def test_reconcile_persist_false_does_not_write_session(self):
        _write_run(self.run, 3)
        recovery.reconcile(self.run, persist=False)
        self.assertFalse((self.run / recovery.SESSION_NAME).exists())
        self.assertFalse((self.run / recovery.AUDIT_NAME).exists())

    def test_display_interrupted_fraction(self):
        _write_run(self.run, 4, planned=8)
        rec = recovery.reconcile(self.run, persist=False)
        self.assertEqual(rec["display"], "Interrupted — 4/8 cells completed")


class OriginalEightyFourCellsUntouched(unittest.TestCase):
    """Read-only: the interrupted run's completed cells still match the backup inventory."""

    @unittest.skipUnless(ORIGINAL_RUN.is_dir(), "original interrupted run is not present")
    def test_original_run_still_has_cells_0_83(self):
        inv = recovery.inventory_run(ORIGINAL_RUN)
        self.assertGreaterEqual(inv["completed"], 84)
        self.assertTrue(set(range(84)).issubset(inv["completed_indices"]))
        self.assertTrue(inv["original_run"])
        self.assertTrue(inv["protected_completed"])

    @unittest.skipUnless(ORIGINAL_RUN.is_dir() and BACKUP_SUMS.is_file(), "backup inventory missing")
    def test_original_completed_cells_match_backup_hashes(self):
        sums = json.loads(BACKUP_SUMS.read_text())
        wanted = {
            row["path"].split("/")[-1]: row["sha256"]
            for row in sums.get("files") or []
            if "/cell_evidence/cell_" in row.get("path", "")
        }
        self.assertGreaterEqual(len(wanted), 84)
        for name, digest in wanted.items():
            path = ORIGINAL_RUN / "cell_evidence" / name
            self.assertTrue(path.is_file(), name)
            self.assertEqual(recovery.sha256_file(path), digest, name)

    @unittest.skipUnless(ORIGINAL_RUN.is_dir(), "original interrupted run is not present")
    def test_reconcile_does_not_mutate_original_cells(self):
        before = recovery.snapshot_completed_hashes(ORIGINAL_RUN)
        session_path = ORIGINAL_RUN / recovery.SESSION_NAME
        session_before = session_path.read_bytes() if session_path.is_file() else None
        recovery.reconcile(ORIGINAL_RUN, persist=False)
        self.assertEqual(recovery.snapshot_completed_hashes(ORIGINAL_RUN), before)
        if session_before is None:
            self.assertFalse(session_path.exists())
        else:
            self.assertEqual(session_path.read_bytes(), session_before)

    @unittest.skipUnless(ORIGINAL_RUN.is_dir(), "original interrupted run is not present")
    def test_lab_replay_is_read_only_and_evidence_backed(self):
        from live_server import lab_cell_by_index, lab_cells_index, lab_summary
        before = recovery.snapshot_completed_hashes(ORIGINAL_RUN)
        idx = lab_cells_index()
        self.assertGreaterEqual(idx["n_completed"], 84)
        self.assertTrue(set(range(84)).issubset({c["index"] for c in idx["cells"]}))
        self.assertTrue(idx.get("read_only"))
        self.assertEqual(idx.get("layout"), "schematic_seating")
        cell = lab_cell_by_index(0)
        self.assertNotIn("error", cell)
        self.assertEqual(cell["layout"], "schematic_seating")
        self.assertTrue(cell.get("protected"))
        self.assertTrue(cell.get("graphs_backed_by_observation"))
        self.assertTrue(cell.get("agents"))
        self.assertTrue(any((a.get("role") or "").lower() in ("buyer", "seller") for a in cell["agents"]))
        self.assertFalse(any("x" in a or "y" in a for a in cell["agents"]))
        later = ORIGINAL_RUN / "cell_evidence" / "cell_84.json"
        cell84 = lab_cell_by_index(84)
        if later.is_file():
            self.assertNotIn("error", cell84)
            self.assertTrue(cell84.get("observation_parse", {}).get("ok"))
            self.assertFalse(cell84.get("observation_parse", {}).get("wrote_archive"))
        else:
            self.assertIn("error", cell84)
        summary = lab_summary()
        self.assertGreaterEqual(summary["n_completed"], 84)
        self.assertEqual(recovery.snapshot_completed_hashes(ORIGINAL_RUN), before)


if __name__ == "__main__":
    unittest.main()
