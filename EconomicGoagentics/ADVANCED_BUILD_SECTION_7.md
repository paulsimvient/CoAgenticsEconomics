# CoAgentics Advanced Build — Section 7

## Real multi-model LLM harness: registry, availability, caching, provenance, and budgets

Section 7 connects the population architecture to a serious model-invocation boundary. The goal is not to assume every model is online. The harness makes availability and failure explicit and keeps reference agents usable as controls.

### New capability

`include/coagentics/llm/LlmHarnessV2.hpp` and `src/LlmHarnessV2.cpp` add:

- explicit provider/model/version identity;
- provider endpoint metadata;
- inference configuration;
- model availability state;
- request and response fingerprints;
- deterministic request canonicalization;
- response caching;
- repeated-inference accounting;
- request and cost budgets;
- provider binding abstraction;
- timeout/provider/error status vocabulary;
- complete invocation records.

### Why this is different from a simple API wrapper

A normal API wrapper answers: “Did the model return text?”

The research harness must answer:

1. Which exact model was used?
2. Which version/provider was used?
3. What exact observation did it receive?
4. What inference settings were used?
5. Was this a fresh request or cache hit?
6. Was the model unavailable or was the model response invalid?
7. Did the run consume a query budget?
8. What did the provider return?
9. Can the request be reproduced or at least fingerprinted?

### Research rule

**Do not silently substitute one model for another.** If a model is unavailable, the record says unavailable. A campaign may explicitly define a fallback policy later, but fallback must be an experimental decision rather than an implementation accident.

### Cache rule

Cache keys include model identity, inference configuration, observation fingerprint/content, prompt version, and run identity. For experiments that intentionally require repeated stochastic inference, the caller can vary the request/run identity or disable deterministic reuse at the campaign layer.

### Budget rule

Budgets are first-class. The harness can deny a request when the campaign has exhausted its request limit. Cost is recorded from provider estimates supplied by the adapter.

### Verification

`tests/llm_harness_v2_smoke.cpp` validates:

- two registered models;
- availability state;
- live provider binding through a mock provider;
- first invocation;
- cache hit on an identical request;
- a distinct run consuming another request;
- request-budget denial;
- explicit unavailable-model behavior;
- invocation record retention.

### Integration target

Section 7 is deliberately provider-neutral. The next integration should map `PopulationCompositionSpec.models` to `ModelIdentity` entries and select an LLM adapter for model slots while retaining reference adapters for known-mechanism controls.

The production provider adapters should sit behind `ProviderFn` or a richer async provider interface later. This keeps the experiment layer independent of provider SDKs.

## Next section

Section 8 should connect LLM responses to the existing action parser/validator and then run a complete mixed population: reference agents + 10+ LLM model slots → information boundary → LLM response → validated action → market execution → evidence record.
