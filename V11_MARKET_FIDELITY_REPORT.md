# V11 Market-Fidelity Replication

**Protocol structural alignment:** PASS  
**Exact numerical replication claim:** NO

| Market | Literature ZI-C mean | v11 protocol-analogue mean | Abs. error |
|---:|---:|---:|---:|
| 1 | 99.90% | 67.14% | 32.76 pp |
| 2 | 99.20% | 63.96% | 35.24 pp |
| 3 | 99.00% | 66.61% | 32.39 pp |
| 4 | 98.20% | 65.91% | 32.29 pp |
| 5 | 97.10% | 68.99% | 28.11 pp |

Mean absolute error: **32.16 percentage points**.

## Qualification boundary
The paper reports the protocol and Table 2 outcomes, but the exact unit-level schedules are graphical in Figures 1-5. v11 therefore qualifies structural fidelity and the measurement path; it does not claim exact numerical replication until those schedules are transcribed/verified from the figures.

## Source-grounded protocol features
- 12 traders: six buyers and six sellers.
- Private redemption values for buyers and unit costs for sellers.
- Units are traded sequentially.
- ZI-C bids are uniformly random but cannot exceed buyer value; asks are uniformly random but cannot fall below seller cost.
- Allocative efficiency is realized trader profit/surplus divided by maximum theoretical trader profit/surplus.
- Table 2 targets for ZI-C are 99.9, 99.2, 99.0, 98.2, and 97.1 percent; human targets are 99.7, 99.1, 100.0, 99.1, and 90.2 percent.
