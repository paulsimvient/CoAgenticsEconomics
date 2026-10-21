#!/usr/bin/env python3
"""Reproducible Q1 qualification procedure.

Runs automated tests and independent archive checks. Writes derived reports
only under results/q1_milestone/. Never writes into the original campaign run.
Does not mark a scientific qualification gate as passed.
"""
from __future__ import annotations

import json
import subprocess
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from enterprise_api import campaign_recovery as recovery
from enterprise_api.q1_independent_validate import CLAIM_BOUNDARY, validate_run
from enterprise_api.q1_native_corpus import corpus_root, emit_native_archive
from enterprise_api.q1_inference import PLAN_ID, plan_sha256

OUT = ROOT / "results" / "q1_milestone"
ORIGINAL = ROOT / "results" / "dv026_heterogeneous_campaign" / "runs" / recovery.ORIGINAL_RUN_ID


def _run_unittests() -> dict:
    loader = unittest.TestLoader()
    suite = unittest.TestSuite()
    for name in ("tests.test_q1_recovery", "tests.test_q1_multi_llm_qualification"):
        suite.addTests(loader.loadTestsFromName(name))
    result = unittest.TextTestRunner(verbosity=1).run(suite)
    return {
        "tests_run": result.testsRun,
        "failures": len(result.failures),
        "errors": len(result.errors),
        "skipped": len(result.skipped),
        "ok": result.wasSuccessful(),
    }


def _run_ctest_subset() -> dict:
    build = ROOT / "build-dv026"
    if not (build / "CTestTestfile.cmake").is_file():
        return {"ran": False, "reason": "build-dv026 CTest file missing"}
    tests = [
        "market_conservation_smoke",
        "heterogeneous_campaign_smoke",
        "heterogeneous_five_five_mock",
    ]
    cmd = ["ctest", "--output-on-failure", "-R", f"^({'|'.join(tests)})$"]
    try:
        p = subprocess.run(cmd, cwd=build, capture_output=True, text=True, timeout=180)
    except Exception as e:
        return {"ran": False, "reason": str(e)}
    return {
        "ran": True,
        "returncode": p.returncode,
        "ok": p.returncode == 0,
        "stdout_tail": (p.stdout or "")[-2000:],
        "stderr_tail": (p.stderr or "")[-1000:],
        "mode_note": "C++ smokes and five_five_mock use scripted mock transport unless a test explicitly opts into live Ollama.",
    }


def main() -> int:
    OUT.mkdir(parents=True, exist_ok=True)
    before = recovery.snapshot_completed_hashes(ORIGINAL) if ORIGINAL.is_dir() else {}
    native_manifest = None
    if ORIGINAL.is_dir():
        native_manifest = emit_native_archive(ORIGINAL, corpus_root(ROOT))
    unittest_result = _run_unittests()
    ctest_result = _run_ctest_subset()
    archive = None
    if ORIGINAL.is_dir():
        archive = validate_run(ORIGINAL)
        featured = archive.get("featured_cell") or {}
        (OUT / "featured_transcript.json").write_text(
            json.dumps({
                "source": "read-only original archive",
                "run_id": recovery.ORIGINAL_RUN_ID,
                "inference_label": featured.get("inference_label"),
                "index": featured.get("index"),
                "seed": featured.get("seed"),
                "repeat": featured.get("repeat"),
                "design": featured.get("design"),
                "agents": featured.get("agents"),
                "transcript": featured.get("transcript"),
                "llm_sides": featured.get("llm_sides"),
                "surplus": featured.get("surplus"),
                "conservation": featured.get("conservation"),
                "reported_efficiency": featured.get("reported_efficiency"),
                "claim_boundary": CLAIM_BOUNDARY,
            }, indent=2, default=str) + "\n"
        )
        (OUT / "matched_comparison.json").write_text(
            json.dumps(archive.get("matched_comparison"), indent=2, default=str) + "\n"
        )
        slim = dict(archive)
        slim.pop("featured_cell", None)
        (OUT / "validation.json").write_text(json.dumps(slim, indent=2, default=str) + "\n")
    after = recovery.snapshot_completed_hashes(ORIGINAL) if ORIGINAL.is_dir() else {}
    if before != after:
        raise RuntimeError("qualification mutated original completed evidence hashes")
    native_ok = bool(native_manifest) and native_manifest.get("n_files", 0) >= 120
    inf = None if archive is None else (archive.get("matched_comparison") or {}).get("inference")
    report = {
        "engineering_gate": "passed" if (
            unittest_result.get("ok") and ctest_result.get("ok")
            and archive and archive.get("engineering_gate") == "passed"
            and native_ok and before == after
        ) else "not_passed",
        "scientific_gate": "not_passed",
        "scientific_gate_reason": None if archive is None else archive.get("scientific_gate_reason"),
        "analysis_plan_id": PLAN_ID,
        "analysis_plan_sha256": plan_sha256(),
        "inference": None if inf is None else inf.get("test"),
        "native_archive": None if native_manifest is None else {
            "n_files": native_manifest.get("n_files"),
            "n_repaired_price_side": native_manifest.get("n_repaired_price_side"),
            "n_trailing_escaped_newline_stripped": native_manifest.get("n_trailing_escaped_newline_stripped"),
            "wrote_original_run": False,
        },
        "unittest": unittest_result,
        "ctest": ctest_result,
        "archive_summary": None if archive is None else {
            k: archive.get(k) for k in (
                "n_completed_evidence", "n_observation_readable",
                "n_observation_repaired_on_read", "n_five_plus_five",
                "n_ollama_cells", "n_both_sides_ollama_all_trades",
                "n_matching_ok", "n_conservation_ok", "n_efficiency_match",
                "n_cells_with_extra_units", "engineering_gate",
                "scientific_gate", "scientific_gate_reason", "claim_boundary",
            )
        },
        "featured_index": None if archive is None else (archive.get("featured_cell") or {}).get("index"),
        "original_run_hashes_unchanged": before == after,
        "wrote_original_run": False,
        "derived_output_dir": str(OUT.relative_to(ROOT)),
        "claim_boundary": CLAIM_BOUNDARY,
    }
    (OUT / "qualification_report.json").write_text(json.dumps(report, indent=2) + "\n")
    json.dump(report, sys.stdout, indent=2)
    sys.stdout.write("\n")
    return 0 if unittest_result.get("ok") else 1


if __name__ == "__main__":
    sys.exit(main())
