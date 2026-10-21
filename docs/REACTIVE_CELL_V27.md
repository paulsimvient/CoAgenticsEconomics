# V27 — Reactive Cell Architecture

V27 establishes one selected-market state shared by the campaign selector and every market visualization.

## Canonical state
`CoAgenticsCellState.index` is the selected cell. A successful load sets `CoAgenticsCellState.cell` from authoritative archived evidence.

## Event contract
- `coagentics:cells-loaded`
- `coagentics:cell-loading`
- `coagentics:cell-selected`
- `coagentics:cell-unavailable`
- `coagentics:evidence-status`

Consumers such as replay, agent trace, order book, event tape, and evidence panels should subscribe to these events rather than maintain independent selected-cell state.

## Data rule
The cell list and selected cell are loaded from DV026 lab endpoints. If authoritative evidence is unavailable, dependent panels must show an unavailable state rather than stale or synthetic data.

## Goal
One selection updates the entire research view simultaneously.
