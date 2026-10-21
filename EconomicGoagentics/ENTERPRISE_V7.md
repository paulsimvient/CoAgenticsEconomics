# CoAgentics Enterprise V7

V7 establishes a real enterprise application architecture at the UI/platform boundary without pretending local HTML is an enterprise identity system.

## Added
- Workspace registry with active workspace context.
- Workspace switching and local workspace creation.
- Governance panel for identity/access and research-integrity state.
- Local audit event API and JSONL audit trail.
- Persistent role/context display.
- Clear separation between locally enabled research controls and deployment-only capabilities such as SSO/RBAC enforcement.

## Architecture
Organization -> Workspace -> Experiment -> Versioned Run -> Evidence -> Validation -> Claim

## Integrity rule
UI state never upgrades scientific evidence. Backend/campaign evidence remains authoritative.
