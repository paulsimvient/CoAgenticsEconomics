# DV026 v2 test report

Implemented in v2:
- zero-intelligence constrained buyer/seller reference agents
- symmetric heuristic buyer/seller policies
- seeded repeated continuous double-auction runner
- randomized activation order
- paired control/treatment public-news experiments using identical seeds
- black-box LLM bid-provider boundary using the same structured Bid interface
- retained ABM population/neighborhood extension boundary for later Axtell-style experiments

Qualification run (100 seeds, 6 buyers, 6 sellers, 50 rounds):
- ZI mean allocative efficiency: 97.4048%

Paired public-news smoke experiment (100 matched seeds):
- mean treatment-control allocative-efficiency delta: +0.0714286 percentage points
- 95% CI: [-0.27612, 0.418977]

Interpretation: these are software qualification/regression results only. They are not evidence of human-market fidelity and do not establish satisfaction of DARPA's >90% proof-of-concept criterion. That requires a preregistered benchmark suite, accepted market baselines, and empirical/human comparison.

All seven CTest tests pass.
