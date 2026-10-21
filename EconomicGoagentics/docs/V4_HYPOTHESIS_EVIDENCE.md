# V4 — Competing hypotheses and discriminating evidence

V4 adds an explicit epistemic layer above behavioral measurements. A behavioral label is not inferred directly from correlated actions.

Pipeline:

`observation -> competing hypotheses -> preregistered discriminator -> paired intervention -> evidence record -> support/challenge/inconclusive`

Initial hypothesis family:

- **H_DIRECT_INFO** — a target's response is primarily attributable to its direct information exposure.
- **H_COMMON_SIGNAL** — apparent co-movement is explained by agents receiving the same information.
- **H_PEER_PROPAGATION** — a response propagates to non-target agents through the observable market state.

A private-information intervention is a discriminator between common-exposure and propagation explanations. The evaluator records primitive effect sizes and the preregistered bounds used for interpretation. It does not call co-movement collusion, coordination, conformity, or deception.

This is intentionally compatible with Axtell-style ABM extensions: population topology, local observation, heterogeneous rules, adaptation, and coalition mechanisms can become intervention variables. They are not assumed by the core testbed.

## Scientific rule

Evidence records are directional and bounded: `supports`, `challenges`, or `inconclusive`. They are evidence about a stated hypothesis under a stated intervention, not declarations of an agent's latent intent.
