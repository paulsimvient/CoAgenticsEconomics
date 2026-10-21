# CoAgentics Advanced Build — Section 6

## Population composition and heterogeneous strategy configuration

Section 6 turns the persistent population runner into a reusable population-construction layer. It separates **who is in the experiment** from **how the population is executed**.

### New capability

A `PopulationCompositionSpec` can now define:

- model/provider/version slots;
- role/side slots;
- reference behavioral strategies;
- reservation/value schedules;
- deterministic seed-based rotation;
- assignment fingerprints.

The same composition machinery can create a 10-, 25-, 50-, or 100-seat population without changing the population runner.

### New types

`include/coagentics/abm/PopulationComposition.hpp`

- `ReferenceStrategy`
- `ModelSlot`
- `RoleSlot`
- `AgentSlotSpec`
- `PopulationCompositionSpec`
- `ComposedPopulation`
- `balanced_assignments(...)`
- `compose_population(...)`

### Design decision

The composition layer does **not** hard-code the economic market. It creates domain-neutral `AgentRef`, `InformationPolicy`, and `AgentAdapter` objects. The initial reference adapters emit generic BUY/SELL/HOLD actions so the same population construction seam can later be connected to logistics or finance adapters.

### Balancing

For each seed, model, role, strategy, and value schedules rotate independently. This makes seat identity different across matched seeds while preserving the underlying model catalog. The assignment metadata is fingerprinted for provenance.

This is intentionally deterministic: given the same composition specification and seed, the assignment is identical.

### Reference strategies

The initial known-mechanism strategies are:

- `fixed_price`
- `signal_responsive`
- `risk_sensitive`
- `peer_responsive`
- `adaptive`
- `hold`

These are controls and experimental fixtures, not claims about live LLM behavior.

### Verification

`tests/population_composition_smoke.cpp` validates:

- composition size;
- preservation of the complete model catalog across repeated seats;
- owned adapters and population wiring;
- deterministic seed-driven model rotation;
- required-input validation;
- non-empty assignment fingerprint.

Targeted test: **PASS**.

The complete repository build remains affected by the existing static libcurl linkage environment (OpenSSL/nghttp2/zstd/brotli/PSL dependencies). The Section 6 target itself builds and runs successfully.

## Why this matters

Section 5 gave us a persistent population. Section 6 gives us a **population design language**. That is the bridge to controlled heterogeneity: the same model can occupy different seats, roles, strategies, and value conditions across matched runs.

## Next section

Section 7 should connect these population compositions to the **real 10+ LLM harness** and add model-slot lifecycle/provenance, provider availability, inference caching, repeated-inference accounting, and cost/query budgets. Reference agents should remain available as controls throughout.
