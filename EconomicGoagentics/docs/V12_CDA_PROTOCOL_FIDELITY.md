# V12 — CDA protocol fidelity

V12 fixes the main v11 replication defect: the replication harness now implements the source-described continuous-double-auction institution rather than feeding unconstrained persistent quotes into the generic market engine.

Implemented protocol controls:
1. single-unit bids/asks;
2. a new bid must improve the standing bid and a new ask must improve the standing ask;
3. crossing bid/ask executes immediately;
4. transaction price is the earlier of the two crossing quotes;
5. every transaction cancels all unaccepted quotes;
6. each trader must transact marginal unit i before unit i+1;
7. ZI-C buyers never bid above redemption value and sellers never ask below cost.

The five private-value schedules remain transparent analogues. They are not represented as exact transcriptions of Gode & Sunder Figures 1–5. Therefore Table 2 numerical replication remains an open qualification item.
