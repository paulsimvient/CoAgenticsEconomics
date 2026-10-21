# V25 — Synchronized Market Replay

## Purpose
Synchronize the selected agent, event tape, order book, trade path, and evidence context into one read-only market timeline.

## Interaction
- Select an event or replay the session.
- The order-book projection changes with the selected event.
- The focused agent changes with the event.
- Decision, order, and outcome are shown together.
- Evidence context explicitly states that replay creates no new observations.

## Production data rule
The V25 interaction model must be populated from the authoritative archived cell evidence. The bundled interaction model is a UI prototype and must not be treated as experimental evidence.

## Design principle
One timeline should coordinate the views rather than making the researcher mentally synchronize separate panels.
