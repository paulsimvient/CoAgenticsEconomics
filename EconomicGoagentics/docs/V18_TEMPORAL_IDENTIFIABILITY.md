# v18: Temporal identifiability, independent validation

v17's full-market experiment could not reliably distinguish signal responsiveness from adaptation. v18 adds a separate, domain-independent temporal protocol qualification that deliberately tests five phases: initial news pulse, repeated exposure, news withdrawal/washout, reversed news, and a peer-only intervention. Each probe has matched no-intervention controls and shared random draws; residual measurement noise remains.

Training and held-out partitions have different fixed seed prefixes. Classifier centroids are fitted on training trials only and frozen before held-out evaluation. A declared nearest/second-nearest margin permits abstention. Null false discoveries count only positive non-blind decisions on information-blind trials. Recovery counts abstentions as incorrect. Test fixtures use five known mechanisms and are not live LLMs or empirical human baselines.

IMPORTANT: The temporal chamber is an isolated policy adapter. This version DOES NOT demonstrate recovery inside the full nonlinear CDA; the prior v17 66% held-out recovery remains the full-market validation result. The next integration task is to implement temporal interventions and windowed response observations in CdaMarketDomain, then independently rerun held-out recovery and null tests. The high recovery on this controlled chamber is a protocol qualification result, not a DARPA capability claim.

Run: `./v18-temporal <output-prefix>`; outputs Markdown and JSON reports.
