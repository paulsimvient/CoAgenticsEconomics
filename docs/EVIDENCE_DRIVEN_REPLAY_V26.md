# V26 — Evidence-Driven Replay

V26 removes hard-coded replay events from the new replay controller.

The replay requests authoritative archived cell evidence from the DV026 lab endpoints, normalizes only fields actually returned, and fails closed if evidence is unavailable.

### Data path
Selected cell → archived cell endpoint → normalization → replay.

### Fail closed
No archived evidence = no replay. The UI explicitly reports that replay is disabled.

### Conservative mapping
V26 does not infer missing order-book states, invent trades, or reconstruct unrecorded events. Such reconstruction requires a separately versioned analysis function with provenance.
