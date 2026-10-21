# V5 — Campaign Runner + Minimal Science UI

V5 turns the scientific components into one executable workflow.

## Campaign
A campaign specifies a market, first seed, number of matched trials, intervention time/signal/reliability, and targeted agents. Each seed runs an unchanged control and a treatment with the intervention as the only intended difference.

## Outputs
`dv026-campaign` writes:
- `results/campaign-summary.json` — aggregate market + behavioral metrics and confidence interval
- `results/evidence.jsonl` — per-seed evidence envelopes with run identity and preregistered discriminator outcome
- `results/science.html` — intentionally minimal Science surface showing Market / Target Agent / Propagation / Evidence

## Axtell extension boundary
Population topology, heterogeneous policies, adaptation, local observation, and coalition/social rules remain experimental factors. The campaign runner does not assume a representative agent or equilibrium. Those ABM mechanisms can be introduced while retaining the same matched-run and evidence lifecycle.

## Important boundary
The UI is an inspection surface, not an inference engine. Semantic claims remain downstream of primitive behavior measurements and discriminating experiments.
