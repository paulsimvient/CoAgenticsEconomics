# Q1 analysis plan (locked)

Executable copy: `enterprise_api/q1_inference.py` (`PLAN_ID` `q1-frozen-vs-sequential-eta-wilcoxon-v1`).

**Primary endpoint.** Allocative efficiency η (percent) stored in `cell_evidence`.

**Pairing.** Same `(seed, repeat)`; `frozen_snapshot` versus `sequential_interaction`. Seat rotation by seed is part of the engine, not a second factor.

**Hypothesis.** H0: median(η_sequential − η_frozen) = 0. Two-sided α = 0.05.

**Test.** Wilcoxon signed-rank. Drop zero differences. Average ranks for tied absolute differences. Normal approximation with continuity correction. One primary endpoint.

**Effect size.** Hodges–Lehmann median of pairwise averages of the nonzero differences.

**Status for run_1792582821581_6712.** Descriptive means for these 60 pairs were inspected before this plan was locked. The Wilcoxon result on this archive is **post-hoc**. A later campaign analyzed only with this plan, without prior inspection, would be confirmatory.

No CUI. No fabricated observations.
