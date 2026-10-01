# DV026 Qualification Report — v10

**Software test status:** GREEN  
**DARPA performance-claim ready:** NO

## Q1_REPRODUCIBILITY — PASS
**Evidence:** seeded deterministic tests and replay paths present

**Limitation:** live provider nondeterminism still requires captured-response replay

## Q2_MARKET_MECHANISMS — PASS
**Evidence:** continuous double auction and sealed-bid double auction implemented

**Limitation:** additional market models remain extensible

## Q3_NULL_CONTROLS — PASS
**Evidence:** null paired-response tests retained

**Limitation:** does not establish external validity

## Q4_MECHANISM_RECOVERY — PASS
**Evidence:** known reference mechanisms are recoverable in benchmark battery

**Limitation:** opaque LLM mechanism claims require discriminating experiments

## Q5_HUMAN_REFERENCE — PASS
**Evidence:** empirical allocative-efficiency reference catalog loaded from Gode & Sunder (1993) Table 2

**Limitation:** reference is condition-specific and must not be treated as a universal human norm

## Q6_HUMAN_COMPARISON — PASS
**Evidence:** observed=99%; literature mean=97.62%; z=0.331239

**Limitation:** software qualification comparison only; protocol must reproduce source conditions before claiming replication

## Q7_DARPA_90_PERCENT — NOT_CLAIMED
**Evidence:** engine can compute allocative efficiency and current code-path campaigns exceed 90%

**Limitation:** DARPA milestone requires a qualified PoC reproducing bidding/allocation behavior; current deterministic qualification is not that demonstration

## Q8_10_LLM_LIVE_SUITE — NOT_CLAIMED
**Evidence:** registry/harness supports 10+ identities

**Limitation:** ten distinct live LLMs have not been connected and experimentally run

## Q9_HUMAN_BEHAVIORAL_BASELINES — PARTIAL
**Evidence:** one empirical market-efficiency reference is source-grounded

**Limitation:** bid shading, reaction latency, information response and social-interaction baselines still need empirical datasets

## Interpretation
PASS means the software path or stated qualification check executed as designed. It does not convert a code test into an empirical claim about humans, LLMs, or DARPA milestone satisfaction.
