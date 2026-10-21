# CoAgentics Advanced Build — Section 13

## Live LLM behavioral-probe campaign

Section 13 connects the statistical/claim-integrity layer to actual model execution. Each model receives matched control/treatment decision contexts. The same seed is used for both members of a pair. The campaign measures three preregistered observable response dimensions: information sensitivity, recent-price dependence, and peer dependence.

### Live-model rule
When `use_live_ollama=true`, the campaign uses only the models returned by Ollama preflight. If the minimum model count is not available, the campaign is not executed and no mock model is substituted. This prevents an offline or partial local environment from producing a misleading “10-model” result.

### Binary estimand
For each probe, the response is whether the buyer emitted a valid BUY action on the controlled decision. The profile uses the Section 12.5 matched-pair estimator and Bonferroni-adjusted confidence intervals. A positive claim requires the lower confidence bound to clear the preregistered minimum effect.

### Probes
- `news`: treatment adds value-relevant positive news while the public book/history is held fixed.
- `price`: treatment changes recent price history while news and other information are held fixed.
- `peer`: treatment exposes a public peer observation while the other information is held fixed.

### Important limitation
The first implementation is a controlled behavioral probe campaign, not a complete causal model of an LLM. Provider stochasticity, prompt sensitivity, repeated-measures dependence, and richer continuous outcomes require additional experimental designs before making stronger claims.

### Smoke test
The smoke mode uses two explicit mock identities only for CI and verifies the entire pipeline. It must not be represented as live-model evidence.
