# V20 — LLM Qualification

V20 adds a qualification console for the Ollama communication layer.

## Qualification principle
Ollama connectivity is necessary but not sufficient. A model must demonstrate grounded communication against authoritative CoAgentics context.

## Test suite
1. Context accuracy
2. Lifecycle understanding
3. Evidence grounding
4. Research-object lineage
5. Claim boundary
6. Uncertainty
7. Hallucination resistance
8. Action safety
9. Contradiction handling

## Evaluation status
The current UI performs automated screening checks on responses. These checks are not semantic proof. Production qualification must verify object IDs, evidence citations, lifecycle state, and response boundaries against authoritative records, followed by human review.

## Safety
Qualification never authorizes a research-state mutation.
