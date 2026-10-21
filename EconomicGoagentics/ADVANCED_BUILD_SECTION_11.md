# CoAgentics Advanced Build — Section 11

## Unknown-Agent Behavioral Identification

### Objective

Section 11 closes the loop between the Section 10 calibration gate and the eventual unknown-LLM experiment. A hidden reference mechanism is exposed only through black-box observations. The scientist receives a candidate explanation set and development probes, selects an explanation, and then the explanation is frozen before held-out observations are evaluated.

### Flow

```text
hidden mechanism (test harness only)
        ↓
black-box observation
        ↓
development probes
        ↓
competing candidate explanations
        ↓
automated scientist
        ↓
FROZEN EXPLANATION
        ↓
held-out probes / new seeds
        ↓
predictive score for every candidate
        ↓
held-out winner + separation gate
```

### Scientific boundary

The hidden mechanism is **not supplied to the scientist**. It exists only inside the black-box domain adapter so the test harness can evaluate whether the identification procedure recovered the known mechanism.

The held-out observations are not fed back into the scientist. They are used only after the development explanation is frozen.

The current implementation evaluates predictive agreement with declared candidate response models. It does not infer latent beliefs, intent, or psychological traits.

### Current implementation

`include/coagentics/analysis/BehavioralIdentification.hpp`

`src/BehavioralIdentification.cpp`

`tests/behavioral_identification_smoke.cpp`

The procedure:

1. Constructs a black-box `AgentProbeDomain` using a hidden mechanism.
2. Runs `run_scientist(...)` on the development probe partition.
3. Records the scientist's selected/frozen hypothesis.
4. Prevents held-out data from changing that hypothesis.
5. Runs the held-out probe partition with new seeds.
6. Scores every candidate using held-out MSE and mean absolute error.
7. Checks whether the held-out winner equals the frozen explanation.
8. Applies a separation gate requiring the best MSE to be materially lower than the next-best candidate.

### Verification

`behavioral_identification_smoke` passes with:

- 4 competing candidate explanations
- 4 development probes
- 12 development replicates
- 4 held-out probes
- 20 held-out replicates
- 80 held-out trials
- hidden `signal-responsive` mechanism
- frozen explanation = `news`
- held-out winner = `news`
- held-out separation gate = PASS

### Why this matters

Section 10 demonstrated that the scientist can recover deliberately planted mechanisms under controlled calibration conditions. Section 11 demonstrates the next property: an explanation can be selected from black-box development observations, frozen, and then evaluated independently on new observations.

That is the minimum research pattern required before moving from known reference mechanisms toward unknown live LLM behavior.

### Next section

Section 12 should move from a single hidden agent to **multiple unknown LLM agents inside the heterogeneous population**, while preserving the same development/holdout boundary. The next implementation should also introduce explicit experiment manifests and claim-level evidence so the final behavioral claim can be reproduced from one campaign artifact.
