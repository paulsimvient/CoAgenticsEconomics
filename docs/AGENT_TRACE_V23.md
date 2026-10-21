# V23 — Agent Trace

The selected market now has an agent-level trace that explains each trader as a recorded research object.

## Trace
Private value → Decision → Order → Book → Trade

## Rules
- Read-only.
- Recorded observations only.
- No inference about internal model reasoning.
- No invented events.
- No new simulation.
- Agent identity and model labels must come from authoritative session data in production.

## UX goal
Make the 10-agent structure understandable without requiring the user to read a graph. Selecting one agent should reveal its complete path through the market session.
