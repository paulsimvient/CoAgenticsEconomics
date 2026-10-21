# CoAgentics Economics — Enterprise V8

## Governed Experiment Lifecycle

V8 introduces an explicit, audited lifecycle for each DV026 experiment:

DRAFT → DESIGN_REVIEW → REGISTERED → FROZEN → RUNNING → DISCOVERY_COMPLETE → HOLDOUT → REPLICATION → VALIDATION_REVIEW → APPROVED_CLAIM

### Implemented
- Persisted lifecycle object: `workbench/experiment_lifecycle.json`
- GET `/api/dv026/lifecycle`
- POST `/api/dv026/lifecycle/transition`
- One-step forward transitions with server-side validation
- Audit entries for lifecycle transitions
- Lifecycle version number and transition history
- Runner completion can close RUNNING → DISCOVERY_COMPLETE only after the campaign reports complete
- Enterprise UI lifecycle ribbon with current state, version, next governed action, and history
- Claim/validation view displays lifecycle state

### Scientific controls
- Registration/freeze is required before campaign execution.
- The lifecycle is separate from observed results.
- A completed campaign does not automatically imply a validated claim.
- Future hold-out, replication, validation, and claim transitions remain gated until their evidence APIs are implemented.
- The UI never treats a lifecycle state as evidence of scientific validity.

### Enterprise controls
Lifecycle transitions are written to the local audit JSONL store. Production deployment should bind the same event contract to centralized identity, RBAC, audit retention, and approval services.
