# CoAgentics Communication Protocol — V18

## Purpose

The CoAgentics LLM is a **communications and navigation mechanism over authoritative research state**. It is not the source of truth, a claim generator, or an autonomous scientific authority.

The protocol is designed to make model behavior understandable, testable, and bounded while allowing Ollama to provide natural-language interaction.

## 1. Canonical interaction path

`User -> Communication Layer -> Context Resolver -> Authoritative Objects/Evidence -> Ollama -> Response Validator -> UI`

For consequential actions:

`User -> Communication Layer -> Context + Policy Check -> Proposal -> Explicit Confirmation -> Authorized Action -> Audit`

The LLM never bypasses the application service boundary.

## 2. Canonical ontology

The communication layer must understand these first-class objects:

- Workspace
- Experiment
- Experiment Version
- Model
- Prompt
- Dataset
- Run
- Observation
- Evidence Package
- Analysis
- Review
- Claim
- Audit Event

Relationships are represented explicitly rather than inferred from prose.

## 3. Epistemic classes

Every substantive response must distinguish, when applicable:

1. **OBSERVATION** — directly recorded/measured.
2. **INTERPRETATION** — analysis of observations.
3. **HYPOTHESIS** — proposed explanation under test.
4. **CLAIM** — bounded assertion subject to the experiment's validation rules.
5. **LIMITATION** — what the available evidence does not establish.

The model must not silently promote one class into another.

## 4. Grounding contract

For questions about research state, the response must be generated from current application context and retrieved authoritative objects.

For evidence-sensitive answers, the response should include machine-resolvable references to the relevant experiment, version, run, evidence, review, or claim.

If the required object cannot be retrieved, the response must say that the information is unavailable rather than infer it.

## 5. Allowed capabilities

The communication layer may:

- explain experiment state and terminology;
- summarize recorded observations;
- locate and explain evidence;
- explain lifecycle gates;
- compare recorded research objects/runs;
- identify limitations;
- navigate the User Guide;
- propose a next governed action;
- prepare a non-executing action proposal.

## 6. Prohibited capabilities

The communication layer must not:

- invent observations, evidence, statistics, or provenance;
- invent causal findings;
- infer hidden beliefs or psychological traits from behavior without an identifying experimental basis;
- declare a claim validated when the governance state does not say so;
- silently modify research objects;
- silently freeze/register/run/approve research;
- use conversational memory as scientific evidence;
- treat its own generated text as evidence.

## 7. Action safety

Actions are classified as:

- **READ** — retrieve/explain; no confirmation required.
- **PROPOSE** — prepare an action; no state mutation.
- **MUTATE** — changes a research object; explicit confirmation required.
- **CONSEQUENTIAL** — freeze, register, run, approve, or otherwise changes scientific/governance state; explicit confirmation plus server-side authorization and audit required.

The LLM can recommend or prepare actions, but authorization belongs to the application.

## 8. Response contract

The communication service should return structured metadata alongside natural language:

```json
{
  "answer": "...",
  "context": {
    "workspace_id": "...",
    "experiment_id": "...",
    "version_id": "..."
  },
  "basis": [
    {"kind": "evidence_package", "id": "..."},
    {"kind": "run", "id": "..."}
  ],
  "epistemic_class": "interpretation",
  "limitations": ["..."],
  "action": {
    "mode": "read|propose|mutate|consequential",
    "requires_confirmation": false
  },
  "grounding_status": "grounded|partial|unavailable"
}
```

The UI should render basis references as links to the underlying research objects.

## 9. Context resolution

The context resolver should assemble only the information needed for the request, prioritizing:

1. current workspace;
2. current experiment/version;
3. current lifecycle state;
4. relevant run(s);
5. relevant evidence;
6. relevant model/prompt/dataset configuration;
7. relevant reviews/claims;
8. relevant User Guide material.

The model should not receive unrestricted environment state.

## 10. Conversation memory

Conversation memory is navigation context, not scientific evidence.

A user statement such as "I think peer visibility increases competition" may be recorded as a hypothesis/research note only through an explicit application action. It must never silently become an observation or claim.

## 11. Failure behavior

The communication layer must fail closed:

- Ollama unavailable -> report unavailable.
- Required context unavailable -> report insufficient context.
- Evidence reference missing -> do not fabricate.
- Contradictory objects -> surface the contradiction.
- Unsupported inference requested -> state the claim boundary.
- Consequential action without authorization/confirmation -> do not execute.

## 12. Qualification suite

Every supported Ollama model/configuration must be tested against:

- context identification;
- object/lineage retrieval;
- evidence grounding;
- lifecycle reasoning;
- version comparison;
- uncertainty/limitation handling;
- hallucination resistance;
- unsupported psychological inference resistance;
- action safety;
- contradiction handling;
- guide navigation.

A model is not considered **qualified for CoAgentics communication** merely because it can produce fluent answers.

## 13. Release gate

A communication-layer release is complete only when:

1. protocol/design updated;
2. User Guide updated;
3. qualification suite passes or documents explicit failures;
4. no unsupported scientific claims are introduced;
5. action boundaries remain server-enforced;
6. audit behavior is verified for consequential operations.

## 14. Design principle

**Simple path, deep system.**

The LLM reduces the cognitive burden of navigating a complex research platform while preserving direct access to the underlying evidence, lineage, and governance state.
