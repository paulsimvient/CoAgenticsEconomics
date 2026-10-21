"""Locked paired Wilcoxon procedure for Q1 frozen vs sequential η.

This module is the analysis plan in executable form. Applying it to the
already-described 120-cell archive is post-hoc. A later campaign analyzed
only with this module, without prior descriptive inspection, would be
confirmatory.
"""
from __future__ import annotations

import hashlib
import json
import math
from collections import Counter
from pathlib import Path

PLAN_ID = "q1-frozen-vs-sequential-eta-wilcoxon-v1"
PLAN_TEXT = """Q1 locked analysis plan
Primary endpoint: allocative efficiency (η, percent) from cell_evidence.
Pairing: identical (seed, repeat); frozen_snapshot vs sequential_interaction.
Hypothesis: H0 median(η_sequential - η_frozen) = 0. Two-sided α = 0.05.
Test: Wilcoxon signed-rank, zeros dropped, average ranks for |d| ties,
normal approximation with continuity correction.
One primary endpoint. No other family-wise tests in this plan.
Effect size: Hodges–Lehmann median of pairwise averages of nonzero differences.
This archive's descriptive means were inspected before the plan was locked;
the p-value below is therefore post-hoc for run_1792582821581_6712.
"""


def plan_sha256() -> str:
    return hashlib.sha256(PLAN_TEXT.encode()).hexdigest()


def _ranks(values: list[float]) -> list[float]:
    order = sorted(range(len(values)), key=lambda i: values[i])
    ranks = [0.0] * len(values)
    i = 0
    while i < len(values):
        j = i
        while j + 1 < len(values) and values[order[j + 1]] == values[order[i]]:
            j += 1
        avg = 0.5 * ((i + 1) + (j + 1))
        for k in range(i, j + 1):
            ranks[order[k]] = avg
        i = j + 1
    return ranks


def _norm_two_sided_p(abs_z: float) -> float:
    return min(1.0, math.erfc(abs_z / math.sqrt(2.0)))


def hodges_lehmann(diffs: list[float]) -> float | None:
    if not diffs:
        return None
    pairwise = []
    for i, a in enumerate(diffs):
        for b in diffs[i:]:
            pairwise.append(0.5 * (a + b))
    pairwise.sort()
    n = len(pairwise)
    mid = n // 2
    if n % 2:
        return pairwise[mid]
    return 0.5 * (pairwise[mid - 1] + pairwise[mid])


def wilcoxon_signed_rank(diffs: list[float]) -> dict:
    zeros = sum(1 for d in diffs if d == 0)
    nonzero = [d for d in diffs if d != 0]
    n = len(nonzero)
    if n < 1:
        return {
            "n_pairs": len(diffs),
            "n_nonzero": 0,
            "n_zero": zeros,
            "w_plus": None,
            "z": None,
            "p_two_sided": None,
            "hodges_lehmann": None,
            "reject_h0_alpha_05": False,
            "note": "no nonzero paired differences",
        }
    absvals = [abs(d) for d in nonzero]
    ranks = _ranks(absvals)
    w_plus = sum(r for r, d in zip(ranks, nonzero) if d > 0)
    mu = n * (n + 1) / 4.0
    tie_adj = sum(t ** 3 - t for t in Counter(absvals).values()) / 48.0
    var = n * (n + 1) * (2 * n + 1) / 24.0 - tie_adj
    sigma = math.sqrt(var) if var > 0 else 0.0
    if sigma == 0:
        z = 0.0
    else:
        z = (w_plus - mu - math.copysign(0.5, w_plus - mu)) / sigma
    p = _norm_two_sided_p(abs(z))
    return {
        "n_pairs": len(diffs),
        "n_nonzero": n,
        "n_zero": zeros,
        "w_plus": w_plus,
        "z": z,
        "p_two_sided": p,
        "hodges_lehmann": hodges_lehmann(nonzero),
        "reject_h0_alpha_05": bool(p is not None and p < 0.05),
        "plan_id": PLAN_ID,
        "plan_sha256": plan_sha256(),
    }


def infer_matched_eta(pairs: list[dict]) -> dict:
    diffs = [float(p["paired_difference_seq_minus_frozen"]) for p in pairs]
    test = wilcoxon_signed_rank(diffs)
    return {
        "plan_id": PLAN_ID,
        "plan_sha256": plan_sha256(),
        "plan_text": PLAN_TEXT,
        "application": "post_hoc_on_already_described_archive",
        "primary_endpoint": "allocative_efficiency",
        "alpha": 0.05,
        "alternative": "two_sided",
        "test": test,
        "claim_boundary": (
            "Post-hoc Wilcoxon on paired η for this archive. "
            "Not a preregistered confirmatory test of a later campaign. "
            "Does not rank models or claim a causal activation-design effect beyond this sample."
        ),
    }


def plan_path(project_root: Path) -> Path:
    return Path(project_root) / "docs" / "Q1_ANALYSIS_PLAN.md"
