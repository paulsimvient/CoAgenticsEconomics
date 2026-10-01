# V14 — Behavioral Phenotyping

V14 changes the primary object of analysis from market efficiency to agent behavior. V13 demonstrated why: a strongly disciplining market institution can yield similar allocative efficiency for policies with different mechanisms.

## Fingerprint

Each agent can now be represented by a machine-readable fingerprint containing value shading, news sensitivity, response latency, peer susceptibility, price-influence proxy, utility-capture proxy, adaptation, withholding, recovery, abstentions, malformed actions, and provider failures.

Known-mechanism controls are used first. This is a mechanism-recovery test: information-blind, signal-responsive, risk-sensitive, peer-responsive, and adaptive controls should generate distinguishable fingerprints.

## Scientific boundary

The mechanism controls are not live LLMs. `price_influence` and `utility_capture` in this first phenotyping build are controlled probe proxies, not claims about real model market performance. Live provider failure/abstention/malformed-action fields must be populated from the v8 black-box harness.

The explanation scores are hypothesis-screening statistics, not semantic diagnoses. Each explanation carries a proposed discriminating experiment so CoAgentics can move from pattern recognition to intervention-based evidence.
