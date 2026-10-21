# CoAgentics Advanced Build — Section 4

## Action → World → Outcome

Section 4 turns the Section 3 agent/action boundary into the first concrete executable `DomainEnvironment` using the existing economic market mechanisms.

### What was added

- `include/coagentics/domain/EconomicEnvironment.hpp`
- `src/EconomicEnvironment.cpp`
- `tests/economic_environment_smoke.cpp`
- CMake target `economic_environment_smoke`

### Runtime path

```text
AgentInvocation
      |
      v
AgentAdapter / ActionBoundary
      |
      v
Action
      |
      v
EconomicEnvironment::validate()
      |
      +---- rejected / abstained ----> explicit outcome, no execution
      |
      v
MarketMechanism::submit_detailed()
      |
      +---- CDA: matching occurs on submit
      |
      +---- Sealed bid: explicit clear()
      |
      v
Execution(s)
      |
      v
World tick + market state
      |
      v
TransitionResult
      |
      v
Evidence layer can bind action + execution + outcome
```

### Important distinctions

1. **Accepted is not filled.** A valid order can rest without executing.
2. **Rejected is not a model failure.** A domain-invalid action is a visible experimental result.
3. **Abstention is explicit.** `HOLD` / `ABSTAIN` advances the world but does not submit an order.
4. **The environment owns truth.** Private values and account state remain in the executable world; the agent receives only an `Observation`.
5. **Mechanism-specific execution is preserved.** CDA and sealed-bid auctions use the existing `MarketMechanism` interface.
6. **World time advances only through `step()`.** The transition returned to the experiment runner is the auditable boundary between one decision and the next state.

### Domain validation implemented

For BUY/SELL actions:

- supported action type
- known asset
- positive integral quantity
- finite non-negative price
- known agent
- sufficient cash for BUY
- sufficient inventory for SELL

For HOLD/ABSTAIN:

- accepted as an explicit abstention
- no market order submitted

### Outcome fields

The transition exposes numeric outcome measures including:

- accepted
- abstained
- filled quantity
- resting quantity
- average fill price when available
- total market trades
- realized surplus
- maximum surplus
- allocative efficiency

Execution IDs are deterministic within the run's trade ledger (`trade-<index>`).

### Test coverage

`economic_environment_smoke` verifies:

- reset and tick progression
- public information remains observable through the information layer
- private cash is observable only through permitted policy
- valid BUY action reaches the mechanism
- domain-invalid action is rejected before execution
- HOLD produces an explicit abstention
- sealed-bid mechanism clears through the same domain environment

### Known limitation / next refinement

The current `TransitionResult::outcomes` is numeric, so the environment does not yet place a string outcome fingerprint directly into that map. The evidence layer should own the canonical outcome serialization/fingerprint. This is intentional rather than overloading a numeric metric container.

## Section 5 target

Build the **persistent agent population and simulation loop** around this boundary:

```text
Population
  -> per-agent information policy
  -> observation
  -> invocation
  -> action boundary
  -> environment step
  -> outcome/evidence
  -> next tick
```

That is the point where the laboratory becomes a real multi-agent simulation rather than a collection of isolated tests.
