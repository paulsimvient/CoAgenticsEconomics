# DV026 vertical slice

The first scientific slice is intentionally small and auditable.

1. A private-value double auction supplies evaluator truth (marginal buyer values and seller costs).
2. The market computes theoretical maximum total surplus independently of transaction price.
3. Public or agent-targeted information events perturb only the observation stream.
4. Control/treatment experiments reuse seeds and market truth.
5. Primitive behavioral features are recorded before labels/classifiers are applied.
6. Evidence should report the observed delta and uncertainty before assigning a behavior category.

## Axtell boundary

The market and experiment controller accept heterogeneous AgentPolicy implementations. Population topology, local interaction, adaptation, and coalition/social mechanisms belong behind this boundary. They are not required by the base market and should be introduced only as explicit experimental factors. This keeps emergent behavior testable rather than embedding it in the infrastructure.

## Next qualification targets

- Multi-unit private values and inventories across multiple assets.
- Seeded randomized arrival/order scheduling.
- Paired news interventions with identical latent market truth.
- Human-calibrated and zero-intelligence reference policies.
- Distributional comparison metrics and classifier interfaces.
- Repeated-market qualification cases targeting >90% allocative efficiency without hard-coding efficient bids.

## v2 reference-agent experiment layer
- Added zero-intelligence constrained (ZI-C) reference traders as a non-LLM control.
- Added symmetric buyer/seller heuristic policies.
- Added seeded repeated continuous double-auction runner with randomized activation order.
- Added paired control/treatment news experiments using identical seeds.
- Added a black-box `LlmBidProvider` boundary: provider/model-specific code cannot bypass the structured market action interface.
- Axtell extension remains above the policy interface: heterogeneous populations, neighborhoods, adaptation, and social interactions can be experimental factors without coupling them to market mechanics.

The ZI efficiency test is a qualification/regression statistic, not a claim that the DARPA >90% criterion is already met. Market/human fidelity requires calibration against accepted auction baselines and experimental data.
