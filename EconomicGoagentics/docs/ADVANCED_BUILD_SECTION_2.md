# CoAgentics Advanced Build — Section 2

## Explicit information environment

Section 2 turns the Section 1 information contract into an executable visibility boundary. An agent receives information through an explicit observation path rather than receiving the complete information ledger and filtering it itself.

### Implemented

- `include/coagentics/domain/InformationEnvironment.hpp`
- `src/InformationEnvironment.cpp`
- `tests/information_environment_smoke.cpp`
- CMake registration

### Visibility controls

The environment now enforces:

- public-news access
- price-history access
- peer-action access
- private-state access
- agent/role-specific visibility
- explicit publication/visibility delay
- event expiration
- observation history windows
- event validation before publication

### Architectural rule

**No information leakage through the observation interface.**

The complete ledger remains owned by the information environment. The observation builder asks the environment what the specified agent can see at the current tick under the specified information policy.

### Verification

`information_environment_smoke` exercises:

1. public information visibility
2. blocked peer actions
3. blocked private state
4. delayed information
5. expired information
6. history-window filtering
7. agent-specific information
8. invalid-event rejection

The test passes when the visibility boundary prevents an agent from observing information outside its declared information regime.

## Next section

Section 3 will put the **agent adapter and action boundary** behind the same contracts: an agent receives only an `Observation`, returns an `Action`, and the environment validates the action before execution. This is where model/provider differences can be introduced without changing the experiment engine.
