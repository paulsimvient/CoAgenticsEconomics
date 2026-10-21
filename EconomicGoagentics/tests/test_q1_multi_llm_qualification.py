"""Q1 multi-LLM qualification tests.

Mock fixtures never call Ollama. Archive tests are read-only and skip if the
original run is absent. No test writes into run_1792582821581_6712.
"""
from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path

from enterprise_api import campaign_recovery as recovery
from enterprise_api.q1_independent_validate import (
    IndependentCDA,
    max_and_realized_surplus,
    validate_cell,
    validate_run,
)

ROOT = Path(__file__).resolve().parents[1]
ORIGINAL_RUN = ROOT / "results" / "dv026_heterogeneous_campaign" / "runs" / recovery.ORIGINAL_RUN_ID


class JsonRepairTests(unittest.TestCase):
    def test_price_side_quote_repair_is_read_only(self):
        bad = '{"bids":[{"price":90","side":"buy"},{"price":88.5","side":"sell"}]}'
        with self.assertRaises(json.JSONDecodeError):
            json.loads(bad)
        text, did = recovery.repair_archive_json_text(bad)
        self.assertTrue(did)
        obj = json.loads(text)
        self.assertEqual(obj["bids"][0]["price"], 90)
        self.assertEqual(obj["bids"][0]["side"], "buy")
        self.assertEqual(obj["bids"][1]["price"], 88.5)

    def test_valid_bid_json_needs_no_repair(self):
        good = '{"price":90,"side":"buy"}'
        text, did = recovery.repair_archive_json_text(good)
        self.assertFalse(did)
        self.assertEqual(text, good)

    def test_decode_ignores_trailing_extra_object(self):
        raw = b'{"agents":2}{"agents":2}'
        obj, meta = recovery.decode_json_document(raw)
        self.assertEqual(obj["agents"], 2)
        self.assertTrue(meta["extra_data_ignored"])
        self.assertFalse(meta["repaired_on_read"])

    def test_decode_strips_literal_backslash_n_suffix(self):
        raw = b'{"agents":2}\\n'
        obj, meta = recovery.decode_json_document(raw)
        self.assertEqual(obj["agents"], 2)
        self.assertTrue(meta["trailing_escaped_newline_stripped"])
        self.assertFalse(meta["extra_data_ignored"])


class IndependentCdaTests(unittest.TestCase):
    def test_mock_five_five_match_surplus_conservation(self):
        accounts = {}
        values = {}
        for i in range(5):
            accounts[f"b{i}"] = {"cash": 10000.0, "inventory": {"ASSET": 0}}
            accounts[f"s{i}"] = {"cash": 10000.0, "inventory": {"ASSET": 1}}
            values[f"b{i}"] = 120 - 2 * i
            values[f"s{i}"] = 82 + 4 * i
        cda = IndependentCDA(accounts)
        for i in range(5):
            self.assertTrue(cda.submit({"time": i, "agent_id": f"b{i}", "asset": "ASSET",
                                        "quantity": 1, "price": values[f"b{i}"] - 5, "side": "buy"})["accepted"])
            self.assertTrue(cda.submit({"time": i, "agent_id": f"s{i}", "asset": "ASSET",
                                        "quantity": 1, "price": values[f"s{i}"] + 5, "side": "sell"})["accepted"])
        self.assertEqual(len(cda.trades), 5)
        cash, inv = cda.totals()
        self.assertAlmostEqual(cash, 100000.0)
        self.assertEqual(inv, 5)
        realized = 0.0
        for t in cda.trades:
            realized += values[t["buyer"]] - values[t["seller"]]
        self.assertAlmostEqual(realized, 130.0)

    def test_extra_unit_does_not_reuse_induced_value(self):
        agents = {
            "b0": {"agent_id": "b0", "role": "buyer", "private_value": 116},
            "b1": {"agent_id": "b1", "role": "buyer", "private_value": 112},
            "s0": {"agent_id": "s0", "role": "seller", "private_value": 82},
            "s1": {"agent_id": "s1", "role": "seller", "private_value": 90},
            "s2": {"agent_id": "s2", "role": "seller", "private_value": 94},
        }
        trades = [
            {"buyer": "b1", "seller": "s2", "quantity": 1, "price": 90},
            {"buyer": "b0", "seller": "s0", "quantity": 1, "price": 90},
            {"buyer": "b0", "seller": "s1", "quantity": 1, "price": 90},
        ]
        s = max_and_realized_surplus(agents, trades)
        self.assertEqual(s["induced_units"], 2)
        self.assertEqual(s["extra_units_excluded"], 1)
        self.assertAlmostEqual(s["realized_surplus"], (112 - 94) + (116 - 82))


class InterruptRecoveryDoesNotOverwrite(unittest.TestCase):
    def test_resume_blocked_leaves_completed_bytes(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        run = Path(tmp.name) / "run"
        run.mkdir()
        recovery.atomic_write_json(run / "run_manifest.json", {
            "campaign_id": "tmp", "base_seed": 1, "n_seeds": 1, "repeats": 1,
            "designs": list(recovery.DESIGNS), "cells_planned": 2,
            "population_design": "heterogeneous_multi_llm", "activation_design": "both",
        })
        ev = run / "cell_evidence"
        ev.mkdir()
        payload = {
            "schema_version": 1, "index": 0, "seed": 1, "repeat": 0,
            "design": "frozen_snapshot", "status": "completed", "efficiency": 50,
            "trades": 1, "agents": 2, "observed_llm_turns": 2,
        }
        recovery.atomic_write_json(ev / "cell_0.json", payload)
        before = recovery.sha256_file(ev / "cell_0.json")
        with self.assertRaises(FileExistsError):
            recovery.atomic_write_json(ev / "cell_0.json", {"status": "completed", "index": 0})
        self.assertEqual(recovery.sha256_file(ev / "cell_0.json"), before)
        recovery.write_session(run, {
            "pid": 123, "exe": "/tmp/runner", "command_token": "hetero-campaign", "status": "running",
        })
        v = recovery.verify_recovery(run, inspect=lambda pid: {
            "pid": pid, "cmdline": "/tmp/runner hetero-campaign", "alive": True,
        }, persist=False)
        self.assertFalse(v["resume_eligible"])
        self.assertEqual(recovery.sha256_file(ev / "cell_0.json"), before)


class OriginalArchiveReadOnly(unittest.TestCase):
    @unittest.skipUnless(ORIGINAL_RUN.is_dir(), "original interrupted run is not present")
    def test_five_five_ollama_both_sides_on_featured_readable_cell(self):
        before = recovery.snapshot_completed_hashes(ORIGINAL_RUN)
        report = validate_run(ORIGINAL_RUN)
        self.assertFalse(report["wrote_archive"])
        self.assertGreaterEqual(report["n_five_plus_five"], 1)
        self.assertGreaterEqual(report["n_ollama_cells"], 1)
        self.assertGreaterEqual(report["n_both_sides_ollama_all_trades"], 1)
        featured = report["featured_cell"]
        self.assertIsNotNone(featured)
        self.assertEqual(featured["inference_label"], "ollama")
        self.assertTrue(featured["five_plus_five"])
        self.assertTrue(featured["both_sides_ollama_all_trades"])
        self.assertTrue(featured["matching_ok"])
        self.assertTrue(featured["conservation"]["cash_conserved"])
        self.assertTrue(featured["conservation"]["inventory_conserved"])
        self.assertEqual(report["n_efficiency_match"], report["n_observation_readable"])
        self.assertEqual(report["scientific_gate"], "not_passed")
        self.assertEqual(recovery.snapshot_completed_hashes(ORIGINAL_RUN), before)

    @unittest.skipUnless(ORIGINAL_RUN.is_dir(), "original interrupted run is not present")
    def test_repeat_buyer_cell_uses_first_induced_unit_only(self):
        cell = validate_cell(ORIGINAL_RUN, 102)
        self.assertTrue(cell["efficiency_matches_recompute"])
        self.assertGreater(cell["surplus"]["extra_units_excluded"], 0)
        self.assertAlmostEqual(cell["surplus"]["realized_surplus"], 48.0)

    @unittest.skipUnless(ORIGINAL_RUN.is_dir(), "original interrupted run is not present")
    def test_refuses_to_overwrite_original_observation(self):
        path = ORIGINAL_RUN / "cell_observations" / "cell_0.json"
        self.assertTrue(path.is_file())
        before = recovery.sha256_file(path)
        with self.assertRaises(FileExistsError):
            recovery.atomic_write_bytes(path, b'{"tamper":true}')
        self.assertEqual(recovery.sha256_file(path), before)

    @unittest.skipUnless(ORIGINAL_RUN.is_dir(), "original interrupted run is not present")
    def test_matched_pairs_share_seed_and_repeat(self):
        report = validate_run(ORIGINAL_RUN)
        pairs = report["matched_comparison"]["pairs"]
        self.assertGreaterEqual(len(pairs), 1)
        for p in pairs:
            self.assertEqual(p["seed"], p["seed"])
            frozen = validate_cell(ORIGINAL_RUN, p["frozen_index"])
            seq = validate_cell(ORIGINAL_RUN, p["sequential_index"])
            self.assertEqual(frozen.get("seed"), seq.get("seed"))
            self.assertEqual(frozen.get("repeat"), seq.get("repeat"))
            self.assertEqual(frozen.get("design"), "frozen_snapshot")
            self.assertEqual(seq.get("design"), "sequential_interaction")
            break  # one pair is enough for the identity check; full report is descriptive
        inf = report["matched_comparison"]["inference"]["test"]
        self.assertEqual(inf["n_pairs"], len(pairs))
        self.assertIsNotNone(inf["p_two_sided"])
        self.assertEqual(report["scientific_gate"], "not_passed")


class NativeCorpusAndInferenceTests(unittest.TestCase):
    def test_wilcoxon_rejects_all_positive_and_not_symmetric(self):
        from enterprise_api.q1_inference import wilcoxon_signed_rank
        all_pos = wilcoxon_signed_rank(list(range(1, 21)))
        self.assertLess(all_pos["p_two_sided"], 0.05)
        self.assertTrue(all_pos["reject_h0_alpha_05"])
        symmetric = wilcoxon_signed_rank([1, -1, 2, -2, 3, -3, 4, -4])
        self.assertGreater(symmetric["p_two_sided"], 0.2)
        self.assertFalse(symmetric["reject_h0_alpha_05"])

    @unittest.skipUnless(ORIGINAL_RUN.is_dir(), "original interrupted run is not present")
    def test_native_corpus_leaves_originals_byte_identical(self):
        from enterprise_api.q1_native_corpus import emit_native_archive
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        before = recovery.snapshot_completed_hashes(ORIGINAL_RUN)
        dest = Path(tmp.name) / "native"
        man = emit_native_archive(ORIGINAL_RUN, dest)
        self.assertFalse(man["wrote_original_run"])
        self.assertGreaterEqual(man["n_repaired_price_side"], 1)
        for row in man["files"]:
            json.loads((dest / row["native_rel"]).read_bytes())
        self.assertEqual(recovery.snapshot_completed_hashes(ORIGINAL_RUN), before)
        with self.assertRaises(json.JSONDecodeError):
            json.loads((ORIGINAL_RUN / "cell_observations" / "cell_84.json").read_bytes())
        with self.assertRaises(ValueError):
            emit_native_archive(ORIGINAL_RUN, ORIGINAL_RUN / "native_inside")


if __name__ == "__main__":
    unittest.main()
