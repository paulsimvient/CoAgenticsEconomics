# CoAgentics Advanced Build — Section 15

## Adaptive Scientist

Section 15 adds a deterministic experiment-selection layer for competing behavioral hypotheses. It chooses the next feasible candidate experiment by how many hypothesis pairs make different predictions, optionally penalized by experiment cost.

### Scientific contract

The selector receives only development-stage hypotheses, candidate experiments, predicted outcomes, feasibility, and cost. It does not receive held-out confirmation observations. Held-out data therefore cannot influence experiment selection.

### Flow

`Evidence -> competing hypotheses -> candidate experiments -> discrimination/cost score -> selected experiment -> run -> evidence update -> repeat -> freeze -> held-out confirmation`

### Selection rule

For each candidate:

`discrimination = distinguishable hypothesis pairs / total hypothesis pairs`

`utility = discrimination - cost_weight * cost`

A candidate is selectable only when it is feasible, has predictions for every hypothesis, distinguishes at least one pair, and meets the configured minimum utility.

### Example

For H1=information, H2=price, H3=peer:

- `news_only` predicts `[1,0,0]`
- `price_only` predicts `[0,1,0]`
- `peer_only` predicts `[0,0,1]`
- `confounded` predicts `[1,1,1]`

The confounded experiment is rejected because it cannot distinguish explanations. The selector records why the chosen experiment was selected.

### Boundary

This is an experiment-selection primitive, not a claim generator. The next stage must execute the selected experiment, update evidence, and enforce a separate frozen-explanation/held-out boundary.
