# CoAgentics Economics — Perfect UI V2

## Visual direction
The primary interface is now an instrument-first research workbench rather than a generic dashboard.

### Core hierarchy
1. Market experiment is the visual hero.
2. Control vs treatment is explicit.
3. The AI agent remains an observable black box; hidden cognition is not inferred.
4. Competing explanations are visible alongside the experiment.
5. Evidence remains traceable to persisted records.
6. `Probe the Market` opens the registered design before execution.
7. Dark blue is the operational mode; light theme is the review/briefing mode.

## New interaction layer
- Universal evidence drawer.
- Probe review modal.
- Population map linked to persisted cells.
- What Changed strip based on persisted campaign state deltas.
- Black-box observation → action visualization.
- Persistent control/treatment context.

## Data-integrity rule
The UI does not invent market prices, trades, hypothesis support, causal claims, memory objects, or validation results. Values shown as observed come from the existing DV026 endpoints or persisted campaign records.

## Validation
- `workbench/interface.html` JavaScript extracted and passed `node --check`.
- `live_server.py` served `/`, `/api/state`, and `/api/dv026/campaign/status` successfully in the build environment.
- Full C++ runner compilation was attempted; the environment's static libcurl link currently reports missing nghttp2/OpenSSL symbols. This is a toolchain/linking issue, not a frontend syntax failure.
