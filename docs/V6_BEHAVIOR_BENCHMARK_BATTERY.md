# V6 Behavioral Benchmark Battery

V6 introduces known-mechanism control agents before adding external LLMs. The purpose is mechanism recovery: a benchmark should detect a mechanism deliberately placed in an agent before interpreting opaque-model behavior.

Controls: information-blind ZI, signal-responsive, risk-sensitive, peer-responsive, and adaptive. Each implements the same AgentPolicy and receives the same Observation contract.

The battery separately perturbs information and peer-observed market state. Recovery is based on primitive response measurements, not semantic labels. This is the first stage of the DV026 discrimination workflow: observation -> candidate mechanism -> orthogonal perturbations -> measured response -> evidence.

Axtell-compatible extension remains above AgentPolicy: heterogeneous mixtures, local interaction topology, repeated adaptation, and coalition structure can be introduced as experimental variables without changing the market kernel.

LLM providers remain black-box: Observation in, structured Bid out. No model internals are required.
