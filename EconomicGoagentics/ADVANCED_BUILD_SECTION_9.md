# CoAgentics Advanced Build — Section 9

## Purpose
Section 9 turns the heterogeneous population machinery into a campaign-level, 10-model research harness. It does not claim ten live models are available. A live campaign is allowed to run only when the configured minimum distinct-model requirement is met; smoke mode may use explicit mock identities.

## Added
- `campaign_id` for campaign-level provenance.
- `minimum_distinct_models` and `require_minimum_distinct_models` gates.
- Explicit `distinct_models_used` and `minimum_model_requirement_met` report fields.
- Per-model descriptive campaign summaries: assignments, turns, BUY/SELL/HOLD-or-abstain counts, accepted and rejected counts.
- Campaign JSON now records the model-count gate and model summaries.
- Dedicated `ten_model_campaign_smoke` exercising 10 model identities × 3 seeds × 2 activation designs × 2 inference repeats, plus fixed-context counterfactual substitution.

## Scientific boundary
The campaign summary is descriptive. It does not by itself establish causal differences between models. Causal claims require the controlled intervention/counterfactual and validation layers already in the architecture.

## Important behavior
If a non-smoke live campaign requests a minimum of 10 distinct models but fewer are actually available, the campaign stops rather than silently filling the missing slots with mocks. This prevents a fake “10-model” result.

## Verification
- `HeterogeneousCampaign.cpp`: syntax-only compilation PASS with `-Wall -Wextra -Wpedantic`.
- `ten_model_campaign_smoke.cpp`: syntax-only compilation PASS.
- Full executable link is currently blocked by the repository's pre-existing static libcurl dependency chain (OpenSSL/nghttp2/zstd/brotli/PSL symbols). This is an environment/linkage issue, not a Section 9 source compilation failure.

## Campaign structure
10 models × 3 seeds × 2 designs × 2 repeats = 120 population cells.
Fixed-context substitution adds 10 models × 2 repeats × 3 seeds = 60 model decisions.

## Next section
Section 10 should add the reference-agent population into the same campaign, rather than treating all seats as LLM seats. That gives us known-mechanism controls alongside the 10+ LLM population and enables the first real behavioral identification experiment.
