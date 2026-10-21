# V10 — Empirical Reference + Qualification Gate

V10 replaces the *allocative-efficiency* synthetic human fixture in the qualification path with a source-grounded reference from Gode & Sunder (1993), Table 2. The five human market mean efficiencies reported there are 99.7, 99.1, 100.0, 99.1, and 90.2 percent. V10 stores those reported values, source identity, population and condition metadata. The across-market mean (97.62) and sample SD are computed from those five reported market means.

This is deliberately **not** a universal human baseline. It is a condition-specific literature reference for laboratory double auctions. A CoAgentics experiment must reproduce or explicitly map the relevant market conditions before claiming replication.

The V10 Qualification Report separates software PASS states from empirical/DARPA claims. In particular, the >90% allocative-efficiency milestone remains NOT_CLAIMED until a qualified proof-of-concept reproduces bidding/allocation behavior under a documented protocol. The 10-LLM requirement remains NOT_CLAIMED until ten distinct live models are actually connected and run.

Still needed: empirical datasets for bid shading, reaction latency, response to dynamic news, collaboration/social influence, and deception-relevant strategic behavior. Those should be added as source-pinned datasets, not guessed normal distributions.

Primary reference: D. K. Gode and S. Sunder, “Allocative Efficiency of Markets with Zero-Intelligence Traders: Market as a Partial Substitute for Individual Rationality,” Journal of Political Economy 101(1), 1993, pp. 119–137, DOI 10.1086/261868.
