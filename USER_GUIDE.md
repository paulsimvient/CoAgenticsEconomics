# CoAgentics Market Explorer — User Guide


## V23 — Agent Trace

Select a trader in **Who did what?** to inspect the recorded path:

**Private value → Decision → Order → Book → Trade**

The trace is read-only. It describes recorded session state and does not infer the model's internal reasoning. If a trader did not trade, the UI explicitly says so.


## V25 — Synchronized Market Replay

Use the replay controls to move through a recorded market session. Selecting an event updates the focused agent, order book, outcome, and evidence context together.

Replay is read-only. It visualizes recorded state and does not resume, rerun, or modify an interrupted experiment.

The production implementation must source all event details from authoritative archived cell evidence.


## V26 — Evidence-Driven Replay

Replay is driven by authoritative archived cell evidence. If the evidence source is unavailable, replay is disabled rather than populated with synthetic events. The UI reports the evidence source when loaded. V26 maps recorded events conservatively and does not infer missing book states or reconstruct unrecorded trades.


## V27 — Reactive Market Cell

Use the Market Cell selector to move between independent market sessions. The selected cell is shared across the campaign and market views. Selecting a cell updates the evidence-backed replay and other subscribed panels. If authoritative evidence is unavailable, dependent views show an unavailable state rather than inventing or retaining misleading session data.


## V28 — Campaign Overview

The Campaign Overview represents each completed market as an independent cell. Use the η distribution or Market Cells table to select a session. Selecting a cell updates the canonical Market Cell state and should synchronize the downstream market, agent, replay, and evidence views. If campaign evidence is unavailable, the overview does not invent cell outcomes.
