# v22: Black-box robustness and abstention

This version strengthens the v21 inference boundary. It requires at least three valid responses per preregistered probe stage, at least 15 valid responses total, and 80% response reliability. It abstains when the closest known response signature has L1 distance above 0.45, even if it is separated from other known signatures. Incomplete probe coverage now abstains instead of silently looking information-blind.

The audit uses 20 untouched deterministic seeds for each of four known scripted mechanisms with Gaussian quote noise (SD 0.45 price units), plus 20 seeds for one novel scripted mechanism, missing-stage, sparse-sampling, provider-failure, shuffled-order, and model-metadata perturbation checks. Test cases are synthetic and use the same stage design as the classifier; they do not establish generalization to real LLMs, unknown prompt formats, nonlinear full-market interaction, or other unfamiliar behaviors. The distance cutoff is a qualification rule, not a calibrated confidence interval. No live provider is connected.

The next milestone is an authorized live-provider transport, paired randomized full-market experiments, independent human reference data, and preregistered validation against unfamiliar mechanisms and market settings. Never report the synthetic audit pass rate as real-model accuracy.
