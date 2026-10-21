# Q1 Visual Laboratory — validation status

## Implemented in uploaded codebase

- Added a filtered, clickable, archived-event timeline with a bounded DOM window.
- Added replay speed selection and synchronized current-event cursor on the recorded market-price graph.
- Linked timeline selections, agent/trade selections, and scrubber playback to the evidence inspector.
- Added copying the selected evidence excerpt with source archive path and cell identity.
- Added fail-closed observation loading when the selected cell's observation archive is absent or mismatched.
- Added `Q1_VISUAL_LAB_HOW_TO.md`.

## Checks executed

- Python backend compilation: passed.
- Python unittest discovery: **39 tests passed / 4 skipped** (39 run total, 35 passed, 4 skipped).
- Inline JavaScript syntax: **10 of 11 script blocks passed**; the legacy research-object graph script (block 6) has a syntax error already present in the supplied source. The modified main application script (block 1) passed.
- Live browser interactions: **not tested**.
- Live Ollama inference: **not tested**.
- 84-cell archive integrity: **not tested** (archives were not provided).

## Scientific boundary

This update is a read-only evidence visualization, not proof of model behavior, causal inference, or experiment completion. No simulated agents or invented market observations are displayed.
