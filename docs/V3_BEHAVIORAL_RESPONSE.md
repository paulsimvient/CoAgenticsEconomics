# V3 — Behavioral Response and Propagation

V3 moves the lab from market-level outcome measurement to agent-level response measurement.

## Added primitive observables
- best bid, best ask, and last trade are now part of the black-box observation supplied to an agent
- targeted/public information remains controlled by the intervention timeline
- information events can expire, enabling transient-signal and recovery experiments

## Paired response metrics
Control and treatment runs are aligned by seed, agent, and tick. The analyzer reports:
- mean signed bid shift
- mean absolute bid shift
- reaction latency after an intervention
- recovery latency after a transient intervention
- peak absolute bid shift
- change in distance from contemporaneous peer bids (a primitive conformity observable)
- targeted vs non-target response magnitude
- propagation ratio = non-target mean absolute shift / targeted mean absolute shift

These are measurements, not semantic labels. V3 does not call a response "bias," "collusion," "deception," or "conformity" merely because a metric changes.

## Axtell/ABM support
The reference zero-intelligence policy now has a small market-mediated interaction term: after observing a transaction, its perceived value can move toward the observed market price. This creates a minimal path by which a targeted perturbation can propagate through interaction rather than direct exposure. Population/neighborhood interfaces remain separate so later experiments can replace market-mediated visibility with explicit local networks.

## Qualification result
The targeted-news smoke experiment produces a large direct response in the targeted agent and a small non-target response through the changed market trajectory. This is a mechanism check, not evidence about real LLM behavior.

Null paired runs remain exactly zero, protecting the paired-analysis path against manufactured differences.
