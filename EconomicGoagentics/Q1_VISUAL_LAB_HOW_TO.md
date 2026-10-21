# CoAgentics Q1 — Visual Laboratory (read-only)

## Purpose

The **Visualize & Analyze** workspace links the archived multi-agent market event stream to a schematic agent map, recorded price graph, and evidence inspector. It is not a simulation generator and does not change, resume, or overwrite the interrupted campaign.

## How to use

1. Start the existing backend with `python3 live_server.py` and open `http://127.0.0.1:8787/`.
2. Choose **Visualize & Analyze** in the Research navigation.
3. Select a **completed cell**. The picker shows its archived design, seed, and efficiency.
4. Use **Replay**, previous/next, the scrubber, and the **Speed** control (0.5×, 1×, 2×, 5×). Replay follows the stored event ordering.
5. Filter the **Archived event timeline** to decisions, bids, or trades. Select an event to synchronize the agent seating map, price cursor, and evidence inspector.
6. Click an agent or a trade connection to inspect related archived evidence. The **Copy selected evidence** button copies a JSON excerpt including its archive path and event metadata.
7. Switch between **Market Lab**, **Scientific Analysis**, and **Evidence Explorer** using the view tabs.

## Interpretation

- Agent positions are **schematic seating** based on recorded buyer/seller roles, not observed spatial movement.
- Market price paths are drawn only from recorded ticks and trades. The vertical cursor represents the currently selected event's recorded time.
- The completed-cell histogram and design summaries are **descriptive**, not causal estimates. A campaign with 84/120 completed cells is not a completed study.
- A cell being marked completed does **not** prove scientific validity, independent model inference, or audit acceptance.
- The UI never synthesizes missing observation archives: a missing or mismatched observation causes the cell endpoint to report an error.
- All replay features are **read-only**. Recovery and new campaign execution belong in the separate Run workflow.

## Known limits

- Agent decision-to-trade attribution depends on identifiers actually persisted by the engine.
- The graph does not estimate unobserved market prices or agent locations.
- The current replay does not implement paired intervention inference, uncertainty intervals, or validated outcome scoring.
- Existing legacy pages and browser qualification remain separate validation gates.

## Validation

`node --check` each inline script block, `python3 -m py_compile live_server.py`, then open the UI and verify the selected event synchronizes across the timeline, price cursor, and evidence. Test with an incomplete or missing observation file in a disposable fixture; **do not edit the real 84-cell archive**.
