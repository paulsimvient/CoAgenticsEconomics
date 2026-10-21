# CoAgentics Economics — Enterprise V13

V13 makes research objects operational. The UI can create, update, delete, inspect, and advance experiments through a governed lifecycle.

Lifecycle: draft → design_review → registered → frozen → running → discovery_complete → hold_out → replication → validation_review → approved_claim.

Every mutation and lifecycle transition is written to the platform audit stream. The object store is portable JSONL for the prototype; production can move the same contract to PostgreSQL or another enterprise datastore.

Scientific observations remain DV026-backend authoritative.
