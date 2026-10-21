# v20 — Decision-level observability and held-out mechanism recovery

## What changed

The v19 classifier averaged *accepted* target quotes, so an intervention could
change the agent's intended action while simultaneously changing which quotes
were accepted. The resulting selection bias obscured the underlying mechanism.

The v20 full-CDA adapter records **every trader activation** (including exhausted
and no-legal-quote events), emitted desired quote, institutional constraints,
accepted/rejected disposition, execution price, counterparty, visible signals,
and deterministic random draw. The raw JSONL example shows both arms of a
matched market and can be replayed from its seed. The exporter demonstrates the
schema; it does not claim to be a connected LLM provider.

The experimental classifier aligns the target agent's decision events on the
same activation tape, retains only events where both arms permit an action, and
subtracts each arm's own quote-generation baseline given its prevailing
market state. This separates **agent-emitted response** from **institutional
clamping and acceptance**. If matched actions are missing, it abstains.

Five sequential sessions retain adaptive state in the target only: news pulse,
repeat, washout, reversal and peer-only. Centroids are trained only on training
seeds, then frozen for independent held-out seeds. The same seed partitions,
trial counts and market settings as the v19 baseline are used for comparison.

## Scientific boundary

The known controlled policies in this qualification add fixed news/peer
adjustments to a known random-quote generator. Subtracting that known generator
is an **instrumented synthetic-policy measurement**, not an inference procedure
available unchanged for arbitrary opaque LLMs. For live models, the equivalent
observable is their raw emitted action before the exchange validates/clamps it;
behavioral attribution will require matched contextual analysis without
assuming the model's internal generator. Neither perfect recovery nor 0% false
positives in these 150 controlled trials establishes deployment accuracy.

The full market remains a transparent analogue, not exact historical market
replication. The peer-only stimulus is exogenous and is not yet a genuinely
endogenous communication network. The five market sessions reset the market;
only the adaptive target's memory persists.

## Reproduce

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
ctest --test-dir build --output-on-failure
./build/v19-market-temporal
./build/v20-decision-trace
./build/v20-trace-export > decision_trace.jsonl
```

## Next validity gate

Introduce real model-emitted pre-validation actions, then compare classifiers
that use only information actually visible to an external observer. Run
out-of-distribution schedules, alternate trader mixes, lower quote budgets,
and held-out market configurations; report uncertainty and abstentions.
