# V8 — Black-box multi-model harness

V8 adds the provider-neutral boundary required to scale DV026 experiments across model families without changing market or experiment code.

## Added
- `ModelIdentity`: provider, model, version, endpoint class.
- `ModelRegistry`: explicit registry capable of enumerating 10+ distinct model identities.
- Canonical observation serialization: every model receives the same market fields.
- Strict structured action validation against the DV026 action contract: time, asset, quantity, price, side.
- Explicit abstention and malformed/invalid-output accounting.
- JSONL provenance log containing request identity, seed, model identity, observation, raw response, validated action/error, and latency.
- Replay transport for deterministic reproduction without re-querying a model.
- Scripted transport for qualification tests.

## Scientific boundary
The harness records only black-box inputs/outputs. Provider-specific reasoning traces are neither required nor treated as evidence. Model APIs are intentionally not embedded in the scientific core; production connectors implement `ModelTransport`.

## Next
Add provider connectors at deployment time, a cross-model campaign runner, rate/cost accounting, and model-version pinning evidence. The repository can test the 10-model orchestration path with registered identities before credentials are supplied.
