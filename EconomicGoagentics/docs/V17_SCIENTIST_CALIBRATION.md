# v17 — Calibration before claims

v17 validates the CoAgentics automated-scientist inference loop against mechanisms whose ground truth is known by construction. Calibration and evaluation use disjoint seed partitions.

The qualification measures mechanism recovery, multiclass Brier score, and false discovery under the information-blind null. The noise floor is selected on training seeds only, then frozen for held-out evaluation.

This is a calibration of the experimental reasoning apparatus in the current nonlinear CDA. It is not evidence that CoAgentics can infer intent, deception, collaboration, or arbitrary latent mental states. Those require separately preregistered experimental protocols and appropriate reference data.
