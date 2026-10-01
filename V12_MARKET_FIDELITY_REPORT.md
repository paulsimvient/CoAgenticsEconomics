# V12 CDA Protocol-Fidelity Replication

**Protocol structural alignment:** PASS  
**Exact numerical replication claim:** NO

| Market | Literature ZI-C mean | v12 protocol-analogue mean | Abs. error |
|---:|---:|---:|---:|
| 1 | 99.90% | 99.30% | 0.60 pp |
| 2 | 99.20% | 99.56% | 0.36 pp |
| 3 | 99.00% | 99.35% | 0.35 pp |
| 4 | 98.20% | 99.09% | 0.89 pp |
| 5 | 97.10% | 99.10% | 2.00 pp |

Mean absolute error: **0.84 pp**.

## Protocol audit
- Single-unit quotes: PASS
- New bids improve standing bid; new asks improve standing ask: PASS
- Crossing quotes execute immediately: PASS
- Transaction price is the earlier quote: PASS
- Transaction cancels standing quotes: PASS
- Marginal units must trade sequentially: PASS
- ZI-C never bids above value / asks below cost: PASS

## Qualification boundary
v12 corrects the CDA protocol: improving standing quotes, single-unit orders, execution on crossing, price equal to the earlier quote, cancellation of all standing quotes after a trade, sequential marginal units, and ZI-C no-loss bounds. Exact Figure 1-5 unit schedules remain unverified, so numerical equality with Table 2 is not claimed.
