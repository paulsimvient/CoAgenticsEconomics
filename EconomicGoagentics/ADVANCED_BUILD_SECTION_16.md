# CoAgentics Advanced Build — Section 16

## Persistent and Longitudinal Agents

Section 16 adds explicit, domain-neutral agent memory. Memory is no longer an implicit side effect of prompt construction: the policy is part of the agent configuration and the retained experience is inspectable.

### Modes
- `None`: no retained experience.
- `Rolling`: retain the most recent N experiences.
- `Episodic`: retain experience history and expose grouped episode summaries.

### Experience record
Each record contains:
- tick
- observation fingerprint
- proposed action
- validation status
- outcome

### Scientific boundary
Memory is an experimental factor. A campaign must record the memory policy so that memory-enabled and memory-disabled agents are not silently pooled. The implementation does not infer what an agent “remembers”; it records what the configured memory layer makes available to the next decision.

### Next integration
The next step is to connect `PersistentMemory` to the Section 5 population runner so every decision can write an experience record and the next observation can include the permitted memory context. Then add longitudinal interventions and adaptation probes.
