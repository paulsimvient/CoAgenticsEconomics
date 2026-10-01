# EconomicGoagentics — Scientific-Integrity Code Review / 2026-10-01

## Review basis

This pass reviewed the Cursor-updated codebase at source level and then applied a targeted corrective implementation. The focus was the DV026 economic-agent path: market validation, agent-visible information, execution semantics, live-campaign qualification, and provenance.

## Corrective changes implemented

1. **Sealed-bid economic validation** — `Sealed::submit_detailed()` now checks order shape, asset, sufficient cash for BUY, and sufficient inventory for SELL before accepting an order. Acceptance is therefore consistent with the CDA market boundary.
2. **Agent-visible vs. audit metadata** — the canonical decision payload no longer exposes request ID, seed, provider/model identity, or the experimental condition label. Those remain in `AgentTurnRecord` / `ModelRequest` for audit.
3. **History is actually exposed** — when authorized, `DecisionContext.information.history` is serialized as actual time/price/book entries rather than only `history_len`.
4. **System prompt neutrality** — the live provider prompt no longer tells buyers/sellers which prices are economically correct. It defines the interface and requires the model to use only supplied information.
5. **Seller-role execution** — the single-agent LLM experiment now initializes the LLM with inventory when it is the seller, initializes the counterparty as the buyer, and computes buyer/seller payoff with the correct sign.
6. **Round history** — the single-agent experiment now carries forward authorized market history between rounds.
7. **Sealed-bid fill accounting** — after clearing, all new trades involving the LLM are aggregated into the turn-level `SubmissionResult`, avoiding last-trade-only accounting.
8. **Strict market qualification rule** — Layer A qualification now requires every qualification trial to exceed the configured gate, rather than passing on the mean alone. The Phase I software suite uses five trials for the market qualification checks.
9. **Live acceptance metric** — the live readiness gate now separately checks non-HOLD action rate and market acceptance rate.
10. **Campaign provenance revision** — live campaign artifacts carry an implementation revision; Phase I will not treat an older/stale campaign summary as current qualification evidence.
11. **Regression tests** — added tests for sealed-bid resource rejection, audit-metadata isolation/history exposure, and LLM seller-role execution.

## Verification

Fresh build:

```text
cmake -S . -B build-final -DBUILD_TESTING=ON
cmake --build build-final -j4
```

Full CTest result:

```text
100% tests passed, 0 tests failed out of 45
Total Test time = 1.46 sec
```

## Important remaining work

This corrective pass intentionally did not pretend that several future DV026 capabilities are already demonstrated:

- Ten live LLM identities have not been experimentally qualified by this code review; the existing ten-model test is an interface/provenance fixture.
- The current behavioral-contrast fixture remains scripted and is therefore an operational classifier-path test, not a live LLM behavioral finding.
- The current transcript replay is captured-provider replay, not historical real-world replay with cutoff/vintage controls.
- Adaptive discovery is not yet selecting among real market/information interventions.
- Human-reference support is presently strongest for allocative-efficiency literature comparisons; broader empirical bidding-pattern comparison still requires appropriate reference data.

These limitations should remain explicit in proposal and campaign claims.
