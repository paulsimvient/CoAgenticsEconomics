# v25 — Network influence in the full CoAgentics market

## What is implemented

- A reusable six-node weighted, directed information network feeds exogenous peer-message exposure to six buyer positions in the v12 12-trader CDA. Sellers remain unchanged ZI-C controls.
- The network intervention and no-message control share an identical activation/random-quote tape.
- Every activation yields the v20 decision trace (attempted/accepted quotes, standing quotes, information exposure, execution).
- Matched-attempt quote shifts are computed only when both conditions have an attempted action at the same activation. They do not claim to measure an intrinsic policy parameter.
- Edge removal and zero-impulse null tests distinguish propagation from the market background; deterministic replay is tested.

## What is NOT established

The network propagates an externally imposed message through fixed edges. Agents do not yet author, choose, or strategically route messages; there is no observed coalition formation or live-LLM experiment. Full-market efficiency remains an outcome, not evidence of agent cognition. A network-mediated quote effect can coexist with changing market opportunities; matched attempted quotes reduce but do not eliminate this ambiguity.

## Next

v26: wire the operator workbench to actual generated JSONL trace and evidence exports, with human review of experimental claims. Keep v21 live provider, v22 external validation, v23 human behavioral data, and v24 strategic communication gates open.
