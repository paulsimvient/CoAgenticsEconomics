# CoAgentics Advanced Build — Section 12

## Unknown-LLM behavioral profiles

Section 12 turns the controlled-probe machinery into a model-specific behavioral profile. The analysis receives only observable treatment/control response rates and model identity. It does not receive a hidden mechanism label.

### Flow

Unknown LLM → controlled probes → observable response rates → effect + 95% CI → behavioral dimensions → model profile.

### Dimensions

- information sensitivity
- recent-price dependence
- peer dependence
- persistence/adaptation

A dimension is marked **supported** only when the lower 95% confidence bound is at or above the prespecified minimum effect. A positive point estimate alone is not treated as sufficient evidence.

### Claim boundary

The profile describes reproducible observable response patterns. It does not infer beliefs, intent, consciousness, or psychological traits.

### Important next integration

The current Section 12 analysis consumes probe results. The next integration should generate those probe results directly from the Section 8 LLM population runner, preserving raw responses, parsed actions, information conditions, seeds, and provenance. Then the profile can be computed from actual LLM campaigns rather than supplied rates.
