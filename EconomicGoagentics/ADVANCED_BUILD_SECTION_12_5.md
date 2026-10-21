# CoAgentics Advanced Build — Section 12.5
## Statistical & Claim Integrity

This section hardens the behavioral-profile layer before live LLM campaigns.

### Changes
- Behavioral profiles now require explicitly matched treatment/control observations for claims.
- Binary probe effects use the paired difference estimator: D in {-1,0,1}; effect = mean(D).
- Standard error is computed from the variance of paired D, not from two independent proportions.
- Confidence intervals use the requested `alpha` and a Bonferroni family-wise adjustment across probes.
- `expected_delta` now controls the expected direction of a claim.
- Claims are explicitly `SUPPORTS`, `CHALLENGES`, or `INCONCLUSIVE`.
- Rate fields are checked against paired discordant counts; inconsistent data is rejected.
- Unpaired legacy observations are rejected rather than silently treated as independent.
- Markdown reports state that analysis is matched-pair and family-wise adjusted.

### Scientific boundary
This is still a binary-response prototype. It is not a universal statistical engine. For small samples, clustered/repeated observations, multiple agents per run, continuous outcomes, or complex longitudinal dependence, later work should add domain-appropriate estimators rather than forcing everything into this paired binary model.

### Tests
`llm_behavioral_profile_smoke` checks paired estimation and profile output.
`llm_behavioral_profile_integrity_smoke` checks rejection of unpaired/inconsistent data plus inconclusive/challenged outcomes.
