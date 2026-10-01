# V7 — DARPA DV026 Compliance Core

V7 maps the stripped testbed directly to Phase I technical requirements.

## Added
- `MarketMechanism` interface; CDA is no longer the only market contract.
- `SealedBidDoubleAuction` as a second mechanism.
- Buyer, seller, and total utility accounting from evaluator-private values/costs.
- `BehavioralClassifier` interface plus a transparent mechanism classifier. Classifier output is evidence input, not final scientific truth.
- 2x2x2x2 discrimination matrix: information exposure × peer visibility × homogeneous/heterogeneous mechanism × incentive shift.
- Heterogeneity remains an experimental variable; Axtell-style population/interaction extensions remain above the market contract.
- Compliance smoke test checks both market mechanisms, utility, classifier, and all 16 discrimination cells.

## Stable DV026 action contract
`{time, agent_id, asset, quantity, price, side}`.

## Scientific rule
Classification never substitutes for causal discrimination. Observable features are classified, then controlled interventions test competing explanations.

## Next
Connect real black-box LLM providers with model/version/provenance logging; add human-reference distributions and qualification datasets; expand auction mechanisms only when needed by a preregistered experiment.
