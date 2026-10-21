# V5 — Synchronized Experiment Replay

The replay uses the same persisted decision event stream returned by `/api/state`.
When market events exist, the replay is synchronized across:
- market price/bid/ask history
- control/treatment labeling
- current event
- event stream
- replay controls and scrubber
- evidence state

If no market events are persisted, the UI explicitly shows an empty state and does not invent data.
Agent/cell decision traces remain available as a fallback when a cell is selected.

Controls: play/pause, step, jump, scrub. The replay is deterministic with respect to the returned event array.
