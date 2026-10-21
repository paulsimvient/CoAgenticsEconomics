# CoAgentics Advanced Build — Section 1

## Domain-neutral contracts

Section 1 establishes additive contracts that let the existing economic laboratory evolve into a domain-neutral experimental platform without rewriting the current Market/Agent/Experiment machinery.

### Added

- `include/coagentics/domain/AdvancedContracts.hpp`
  - `WorldRef`
  - `AgentRef`
  - `InformationEvent`
  - `InformationPolicy`
  - domain-neutral `Observation`
  - domain-neutral `Action`
  - `AgentInvocation`
  - `ActionResult`
  - `TransitionResult`
  - `ExperimentSpec`
  - `RunSpec`
  - `EvidenceRecord`
  - `DomainEnvironment`
  - `AgentAdapter`
  - `InformationEnvironment`
  - validation helpers
- `tests/advanced_contracts_smoke.cpp`
- CMake registration for the new smoke test

## Design rule

The new contracts are additive. Existing economic APIs remain intact. The next sections will move the current economic implementation behind these boundaries incrementally, with regression tests at each step.

## Verification

```text
advanced_contracts_smoke: PASS
```

The full repository build was not used as the gate because the repository contains a large number of historical smoke targets. The new target and its library dependency compile and link successfully.

## Next section

Section 2 will implement the **explicit information environment** behind these contracts: public/private visibility, reliability, delayed availability, expiration, history windows, and leakage tests. This is the first step that materially changes experimental capability rather than only defining interfaces.
