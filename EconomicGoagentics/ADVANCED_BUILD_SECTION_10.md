# CoAgentics Advanced Build — Section 10
## Known-Mechanism Reference Calibration

### Objective
Section 10 adds a calibration gate for behavioral identification. Known in-process mechanisms are deliberately planted and measured through controlled probes before unknown/live LLM behavior is interpreted.

### Why this exists
A classifier that appears to find patterns in an LLM is not useful if the measurement pipeline cannot recover mechanisms whose behavior is already known. This section therefore tests five controls in the same conceptual probe space:

- information-blind
- signal-responsive
- risk-sensitive
- peer-responsive
- adaptive

### What was added
- `include/coagentics/analysis/ReferenceMechanismCalibration.hpp`
- `src/ReferenceMechanismCalibration.cpp`
- `tests/reference_mechanism_calibration_smoke.cpp`
- CMake target/test registration

### Probe logic
Each known mechanism is evaluated with controlled changes to:

1. public information signal;
2. peer/last-trade displacement;
3. response magnitude under a known signal;
4. repeated evidence to expose state updating.

The calibration classifier is deliberately simple and mechanism-specific. It is a **calibration control**, not a claim that these rules are the final classifier for live LLMs.

### Gate
Default: 20 trials per mechanism (100 total).

Pass requires:
- >=95% known-mechanism recovery;
- <=5% false positive rate for the information-blind null control.

### Verification
`reference_mechanism_calibration_smoke` builds cleanly with C++20 and strict warnings and passes:

`reference mechanism recovery=1 null_fp=0`

CTest: **1/1 passed**.

### Interpretation boundary
A pass means the controlled probe pipeline can recover the five known mechanisms under these definitions. It does not establish classifier performance on arbitrary LLMs. Live-model claims remain subject to the later experimental, competing-explanation, and held-out-validation gates.

### Next section
Section 11 should connect this calibrated reference population to the first full behavioral-identification campaign: controlled interventions, competing explanations, discriminating experiments, effect estimates, and held-out predictive confirmation.
