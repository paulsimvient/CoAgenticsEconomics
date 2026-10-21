# CoAgentics Advanced Build — Section 3

## Agent → Action boundary

This section establishes a domain-neutral boundary between an agent and the executable world.

### Implemented

- `ReferenceAgentAdapter`: reference/heuristic/optimization/human-proxy agents can use the same adapter boundary as an LLM.
- `ActionBoundary`: receives an `AgentInvocation`, calls the adapter, validates the returned `Action`, and produces an `AgentTurn`.
- Explicit statuses: `accepted`, `abstained`, `rejected`, `provider_failure`.
- Action type allow-list and numeric bounds.
- Observation and action fingerprints for traceability.
- Exceptions become recorded provider failures instead of crashing the campaign.
- No adapter receives a `DomainEnvironment`; it only receives the invocation/observation.

### Why this matters

The market should not know whether an action came from an LLM, a scripted strategy, an optimizer, or another reference model. All of them cross the same boundary. That makes heterogeneous populations possible without special-casing the market for each model class.

### Next section

Section 4 should connect this boundary to the executable world transition: accepted actions → domain legality/account constraints → mechanism execution → outcome → agent state after → evidence record. The key rule is that an accepted proposal is still not necessarily an execution.
