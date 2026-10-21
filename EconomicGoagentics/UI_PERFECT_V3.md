# CoAgentics Economics — PERFECT UI V3

## Purpose
V3 is the live-instrument refinement of the V2 interface. The market remains the hero while the UI exposes synchronized live state without fabricating observations.

## Implemented
- Live market ribbon tied to `/api/state` and campaign state.
- Clickable observed market events with treatment/control labeling derived from persisted event fields.
- Intervention marker when the persisted event stream exposes treatment/peer visibility.
- Selected-agent population nodes connected to the persisted cell/decision trace.
- Black-box panel updates to the selected persisted agent and latest observed action.
- Recent market-event ribbon beneath the market chart.
- Keyboard shortcuts: `P` opens Probe, `R` refreshes, `Esc` closes overlays.
- Empty states explicitly state that values are not synthesized.
- Existing dark/white themes retained.

## Scientific boundary
The interface does not infer hidden model reasoning. It distinguishes observations from hypotheses and leaves validation/claim authority with the DV026 backend.

## Validation
- `interface.html` JavaScript syntax checked with Node.js.
- Local server root returned HTTP 200.
- `/api/state` returned a valid idle-state JSON response in the build environment.
