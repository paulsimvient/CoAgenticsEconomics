# CoAgentics Economics — Visual Reset V1

## Design intent
The primary interface is now an instrument-first economic experiment surface, not a generic dashboard.

### Visual hierarchy
1. **Market** is the hero object.
2. **Intervention** is explicit: control vs treatment.
3. **Agents** are observable actors, not hidden AI abstractions.
4. **Competing explanations** remain visible beside the experiment.
5. **Evidence** and validation remain one click away.
6. **Probe the Market** leads to the registered experiment design; it does not invent or silently execute a new causal intervention.

## Themes
- Dark blue is the default research/mission-control theme.
- White is available through the sun/moon button.
- Theme preference is stored in localStorage.

## Data integrity
The new hero market chart, order book, and trade list use `/api/state`, the existing live C++ controlled 12-trader simulation endpoint. They do not synthesize market prices from efficiency statistics.

If no live market events exist, the UI explicitly says so.

Campaign evidence continues to use the existing DV026 endpoints:
- `/api/dv026/campaign/status`
- `/api/dv026/campaign/charts`
- `/api/dv026/campaign/analysis`
- `/api/dv026/campaign/cells`
- `/api/dv026/campaign/cell`
- `/api/dv026/expectations`
- `/api/dv026/design`
- `/api/dv026/design/save`
- `/api/dv026/expectations/freeze`
- `/api/dv026/campaign/start`
- `/api/dv026/campaign/cancel`
- `/api/dv026/campaign/export`

## Deliberate boundary
The UI does not turn a registered hypothesis score into causal support. It does not fabricate an order book, trade history, memory influence, confidence score, or validation result when the backend has not returned one.


## V3 Live Instrument Layer

Implemented synchronized live-market ribbon, clickable observed market events, intervention marker, selected-agent black-box state, keyboard shortcuts (P=probe, R=refresh, Esc=close), and event chips. All values remain backend/persisted-data authoritative.
