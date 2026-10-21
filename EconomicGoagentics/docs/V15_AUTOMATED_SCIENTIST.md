# V15 — CoAgentics reusable automated scientist

The domain-neutral `ExperimentalDomain` interface returns paired control/treatment observations for a preregistered `Probe`. `run_scientist` selects a probe by prior-weighted predicted separation, executes matched-seed replicates, updates a model-conditional posterior with a declared measurement-error floor, and appends provenance-tagged records to the existing CoAgentics `EvidenceStore`. It stops when a candidate passes a declared posterior threshold, no probe can distinguish candidates, or the experiment budget is exhausted.

`AgentProbeDomain` is a DV026 adapter around the v14 controlled agent policies; the controller itself has no auction dependencies. The evidence-store API is reused, not duplicated. The test checks mechanism recovery for blind/news/peer policies, deterministic replay, and an unidentifiable two-hypothesis null. Tests are also run in Debug so assertions are active.

## Scientific boundaries
- The present selection objective is predictive variance, an information-gain surrogate, not full expected information gain.
- The candidate likelihood is a simple Gaussian observation model with configured noise floor; posteriors are conditional on its assumptions and candidate completeness.
- This is a controlled **agent-policy probe**, not a complete v12 auction replay, a live LLM campaign, or a real-human comparison. Market-wide causal effects need a separate domain adapter.
- Null tests cover identical candidate predictions; they do not yet establish familywise false-discovery control or calibration under model misspecification.
- Evidence records reuse the existing EvidenceStore; experiment seeds, control/treatment identifiers and probe IDs are preserved. The evidence direction is a model-fit check, not a proof of intent.

## Run
`cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j8 && ctest --test-dir build --output-on-failure && ./build/v15-scientist`
