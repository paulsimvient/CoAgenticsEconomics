# V24 — Evidence-Backed Agent Trace

## Purpose
Replace demonstration trader values with the authoritative archived market-cell service.

## Data path
`/api/dv026/lab/cells` selects completed markets; `/api/dv026/lab/cell?index=N` supplies the selected cell’s agents, turns, bids, trades, and market ticks.

## Agent trace
Private value → recorded decision → recorded order → end-of-session book state → recorded trade.

## Boundary
The UI does not infer hidden model reasoning. If archived evidence is unavailable, it fails closed and does not display substitute demonstration values.
