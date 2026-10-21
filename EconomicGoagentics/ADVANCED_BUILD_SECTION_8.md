# CoAgentics Advanced Build — Section 8

## LLM -> Action -> Market integration

### Objective
Connect the Section 7 research-grade LLM harness to the domain-neutral Section 3 agent boundary and the Section 5 persistent population runner, so a real LLM adapter can participate in the existing economic environment without bypassing validation, execution, or evidence boundaries.

### Architecture

```text
Population
   ↓
AgentInvocation
   ↓
LlmAgentAdapterV2
   ↓
LlmHarnessV2
   ↓
Provider / model / version
   ↓
Raw model response
   ↓
Established market-action parser
   ↓
Domain Action
   ↓
ActionBoundary
   ↓
EconomicEnvironment::validate
   ↓
Market mechanism
   ↓
Execution / resting order
   ↓
TransitionResult / outcome
```

### Important design decisions

1. **No second action grammar.** Section 8 reuses the established `parse_market_action` parser so BUY/SELL/HOLD interpretation is not duplicated.
2. **Provider failures remain explicit.** Unavailable, budget-denied, timeout/provider errors do not become another model or a synthetic action.
3. **Parse failures are explicit.** `AgentDecisionError` carries `ParseFailure` through the agent boundary rather than silently converting malformed output to HOLD.
4. **The LLM never receives the environment.** It receives only the canonical observation supplied by `AgentInvocation`.
5. **Acceptance and execution remain separate.** A valid order may rest without filling; execution is determined by the economic mechanism.
6. **Model provenance remains in the LLM harness.** Provider/model/version, request and observation fingerprints, response fingerprint, inference configuration, cache status, and cost are retained.

### Test

`tests/llm_population_integration_smoke.cpp` creates:

- one mock LLM buyer using `LlmHarnessV2` and `LlmAgentAdapterV2`
- one reference seller using the same domain-neutral `AgentAdapter` boundary
- the existing `EconomicEnvironment`
- the persistent `PopulationRunner`
- sequential interaction for two ticks

The provider returns a canonical BUY action. The seller returns a valid SELL action. The test verifies both actions cross the same boundary, the LLM harness records invocations, and the market executes trades.

### Verification

```text
section8 llm population integration PASS turns=4 llm_invocations=2 trades=2

ctest: 1/1 passed
```

### Next section

Section 9 should move from a two-agent integration test to a **10+ model heterogeneous campaign**: model registry + population composition + LLM adapters + reference agents + balanced assignment + campaign-level provenance + repeated seeds. The goal is the first reproducible multi-model experimental campaign, not merely another unit test.
