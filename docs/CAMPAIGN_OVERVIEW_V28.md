# V28 — Campaign Overview

## Purpose
Represent the campaign as independent market sessions rather than as one giant graph.

## Primary view
- Completed markets / planned markets
- Mean η
- Total recorded trades
- Selected cell
- η distribution
- Market-cell table
- Zoom path: Campaign → Market → Agent → Event → Trade → η

## Data rule
The overview is driven by the authoritative `/api/dv026/lab/cells` response. No synthetic 84-cell dataset is displayed.

## Interaction
Selecting a bar or row calls the canonical `CoAgenticsCellState.select(index)` so the market-level workspace synchronizes to the same cell.
