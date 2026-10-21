# V9 — Cross-model campaign + human-reference qualification

V9 adds the orchestration and comparison layer needed to move from single-model harness tests to DV026-style benchmark campaigns.

## Added
- Cross-model matrix: model identity × seed × market mechanism × intervention state.
- Aggregated efficiency/utility results with min/mean/max qualification statistics.
- `HumanReferenceSet` for empirical reference distributions and standardized comparisons.
- Explicit provenance on every reference distribution.
- Synthetic qualification fixture that is deliberately labeled non-empirical; it validates plumbing only and MUST be replaced by cited human experimental-market data before scientific claims.
- 10-model orchestration smoke test across two market mechanisms and paired intervention state.

## Scientific guardrail
An out-of-reference result is a statistical observation, not a diagnosis of bias, deception, irrationality, or harmful intent. Behavioral labels require the existing competing-hypothesis and discriminating-intervention evidence pipeline.

## Next
1. Ingest cited human experimental-economics datasets and preserve study/population/market-condition provenance.
2. Add empirical distribution distances (ECDF/KS/Wasserstein where appropriate) rather than relying only on z-scores.
3. Execute live model connectors through `ModelTransport` and pin model/version evidence.
4. Produce a DARPA qualification report covering market fidelity, >90% allocative efficiency, model-model comparison, human comparison, reproducibility, and failure/abstention rates.
