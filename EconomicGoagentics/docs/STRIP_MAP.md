# Strip map from CoAgentics 20260921-221744

## Retained conceptually
- Experiment reproducibility, paired comparisons, statistics.
- Epistemic schemas: hypotheses, rival hypotheses, discrimination plans, dataset separation.
- Domain-neutral agent/world boundary.
- Provenance/evidence philosophy.

## Replaced with smaller DV026-native implementations in v0
- Mission/domain runtime -> `market::Market`.
- COA/strategy objects -> `agents::AgentPolicy` and observable bids.
- Mission interventions -> structured information/news interventions (next step).
- Mission outcome utility -> market surplus/allocative efficiency.

## Removed from this stripped tree
All DCA, maritime, orbital/spaceport, living-city/map, organism/morphology, mission companion, mission reasoning, and scenario-specific UI/code. The original ZIP remains untouched.

## Axtell support boundary
Keep interfaces for heterogeneous agent policies, populations, neighborhoods/local interaction, and time-stepped emergence. Do not add firms/households/demography/macroeconomy unless an experiment requires them.
