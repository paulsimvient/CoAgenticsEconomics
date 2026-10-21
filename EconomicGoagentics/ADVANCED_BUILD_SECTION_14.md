# CoAgentics Advanced Build — Section 14

## Evidence & Provenance

Section 14 makes every behavioral campaign observation auditable from the model-specific profile down to the matched control/treatment model invocation.

### Evidence chain

`Claim/Profile -> Probe -> Seed -> Control/Treatment Run -> Canonical Observation -> Raw Response -> Parsed Action -> Validation -> Outcome -> Fingerprints`

The campaign now retains an `EvidenceTrace` for each matched pair. It includes model identity, run IDs, seed, canonical requests, raw provider responses, parsed actions, validation status, and observation/action/outcome fingerprints.

### Audit rules

- Evidence IDs must be unique.
- Every trace must identify the probe, seed, control run and treatment run.
- Both sides must retain observation, action and outcome fingerprints.
- Both sides must retain the canonical request and raw provider response.
- Both sides must retain parsed action and validation status.
- Missing or duplicate evidence is a failed audit; no silent repair.

### Outputs

`evidence_jsonl()` emits one machine-readable record per matched pair.
`evidence_index_markdown()` creates a researcher-facing navigation table.

These outputs establish provenance, not causality. Scientific claims still depend on the preregistered estimand, statistical procedure, competing explanations, and held-out confirmation.
