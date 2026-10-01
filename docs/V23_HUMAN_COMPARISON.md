# v23 Human behavioral comparison: implemented boundary and outstanding evidence

This version adds a source-identified, condition-matched human comparison API and CLI. The empirical catalog contains the five HUMAN market-level mean allocative efficiencies from Gode and Sunder (1993), Journal of Political Economy 101(1):119–137, Table 2, DOI 10.1086/261868: 99.7, 99.1, 100.0, 99.1, 90.2. Their mean is 97.62. These five observations are market means, **not five independent people**, and should not be pooled as individual-level human observations. A normal-approximation comparison across these five market means is not a robust human population inference. The v23 report therefore uses descriptive differences and an empirical rank only.

The v23 API refuses to compare mismatched experimental conditions, distinguishes scripted-control observations from live-provider observations, rejects pooling mixed origins, and exposes missing human reference coverage for bidding, information response, adaptation, and social influence. No human data were invented. No live provider was connected in this build. The v23 CLI intentionally reports no condition-matched model observations until an authenticated run is supplied.

## Preserved six-stage CoAgentics roadmap and acceptance gates

- **v21 Real-model integration — PARTIAL:** canonical prompts/actions, provenance, decision traces, black-box classifier exist. An authenticated live-provider transport and real model runs are still missing. The prior scripted-control tests must not be called live-model validation.
- **v22 Independent scientific validation — PARTIAL:** independent seeds, noisy and unfamiliar scripted mechanisms, abstention and null checks exist. Untouched external scenarios, independently sourced populations, full-market live-provider validation and uncertainty calibration are still outstanding.
- **v23 Human behavioral comparison — PARTIAL:** condition/provenance-gated comparison with one documented market-efficiency reference is implemented. Published or newly collected compatible human bid shading, information response, adaptation and social-interaction distributions remain missing; no claims about human-equivalent LLM behavior.
- **v24 Influence and deception experiments — NOT YET IMPLEMENTED:** controlled communication, private information, conflicting incentives, verifiable statements, and interventions distinguishing misrepresentation from inference about intent.
- **v25 Emergent multi-agent behavior — NOT YET IMPLEMENTED:** information propagation, coordination, coalitions, and interventions on communication networks and agent composition.
- **v26 CoAgentics operator workbench — NOT YET IMPLEMENTED:** integrated experiment controller, action traces, alternative causal explanations, evidence registry and human review.

## Next scientific action

Complete the missing live-provider transport and run authorized real models on preregistered, matched market experiments. Separately obtain condition-matched human microdata or transcribed published distributions for behavioral outcomes. Do not substitute the existing synthetic qualification fixtures for human empirical data.
