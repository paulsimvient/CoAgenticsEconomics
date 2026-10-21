# CoAgentics Economic Intelligence Lab — User Guide

**Version 15.0**

This guide is part of the product. **Every release that changes a user-visible workflow must update the guide in the same release.**

## Core workflow
Workspace → Experiment → Version → Run → Evidence → Review → Claim

## Start here
Confirm workspace, open the experiment, read the question, inspect lifecycle state, then follow the next governed action.

## Portfolio & workspace
Use Portfolio to find active research and attention items. Workspace provides organizational context. Enterprise metadata is not scientific evidence.

## Experiments
Define question and conditions, register model/prompt/dataset, version the design, pass required gates, and freeze before controlled execution.

## Running research
Observe phase, conditions, agent actions, market events, and execution state. The DV026 execution stream is the source of observed behavior.

## Evidence
Inspect source experiment/version, run provenance, observations, analysis configuration, and limitations.

## Reproducibility
The V15 graph is dynamically populated from the enterprise research-object registry and DV026 execution state. Refresh retrieves current relationships; Fit redraws the graph; clicking a node opens its object.

## Review & claims
Review determines whether evidence supports a bounded claim under registered conditions. Keep observation, interpretation, claim, and limitation distinct.

## Enterprise administration
Persistent objects include workspaces, experiments, versions, models, prompts, datasets, evidence, reviews, claims, and audit events.

## Troubleshooting
If lineage is unavailable, do not treat fallback UI as evidence. If a value looks wrong, identify whether it is observed data, enterprise metadata, or presentation information.

## V15 changes
- Dynamic reproducibility graph
- Dynamic experiment/model/prompt/dataset/evidence/review/claim relationships
- Click-to-inspect lineage nodes
- Built-in contextual User Guide
- F1 guide shortcut
- Release rule: guide must change with every user-visible workflow change


## V17 — Ollama Research Communication

### What it is
The **Ask CoAgentics** panel is a local-language communication layer over the research application. It uses Ollama when Ollama is running locally.

### What it can do
- Explain the current experiment and lifecycle.
- Point you toward available evidence and research objects.
- Explain what the application currently knows and does not know.
- Suggest the next governed workflow step.
- Help navigate complex terminology and the User Guide.

### What it cannot do
The communication model is **not the source of truth**. It must not invent experimental observations, statistics, evidence, causal findings, or hidden psychological states. It does not approve claims or perform consequential mutations.

### Ollama setup
The workbench expects Ollama at `http://127.0.0.1:11434` by default. Configure `OLLAMA_URL` to change the endpoint and `OLLAMA_MODEL` to select the model. The UI reports whether Ollama is reachable.

Example:

```bash
OLLAMA_MODEL=llama3.2 ./start-workbench.sh 8787
```

### Recommended use
Ask questions such as:
- “What should I do next?”
- “What evidence is available for this experiment?”
- “Explain the current lifecycle state.”
- “What changed between the current and previous research objects?”

For scientific decisions, inspect the linked evidence and research objects yourself.

### Release requirement
Any change to the communication layer, prompt policy, supported Ollama configuration, or user-facing behavior must update this guide in the same release.

## V18 — Communication Protocol and LLM Qualification

### The LLM's role
Ask CoAgentics is a communication and navigation layer over authoritative research objects. It is not the source of scientific truth.

### How an answer is produced
`Question → Context → Research Objects/Evidence → Ollama → Response Validation → Answer`

For consequential actions:
`Question → Proposal → Explicit Confirmation → Authorized Action → Audit`

### What the LLM knows
The communication layer works from the current workspace, experiment/version, lifecycle, runs, evidence, registered model/prompt/dataset objects, reviews/claims, and relevant User Guide content.

### What the LLM must distinguish
- Observation: what was recorded.
- Interpretation: what analysis suggests.
- Hypothesis: what is being tested.
- Claim: what is asserted within the experiment's boundary.
- Limitation: what the evidence does not establish.

### When not to trust an answer as evidence
If the answer lacks a linked research object/evidence basis, or the application reports insufficient context, inspect the underlying objects directly. Conversational memory and generated language are not scientific evidence.

### Consequential actions
The LLM may explain or propose an action, but it cannot silently register, freeze, run, approve, or otherwise mutate research state. Those operations require application authorization, explicit confirmation where required, and audit logging.

### LLM qualification
Every supported Ollama model/configuration is evaluated for context, lineage, evidence grounding, lifecycle reasoning, uncertainty, hallucination resistance, unsupported-inference resistance, contradiction handling, action safety, and guide navigation. Fluency alone is not qualification.

### V18 release rule
Communication behavior, ontology, context rules, action policy, Ollama configuration, qualification criteria, or user-facing LLM workflows must be updated in the design documentation and this User Guide in the same release.


## V19 — Research Navigator

The Research Navigator provides natural-language communication over authoritative research objects.

### Protocol
**ASK → RETRIEVE → EXPLAIN → CITE → VERIFY**

Ask about lifecycle, evidence, lineage, model/prompt identity, or changes between runs. The Navigator must not invent research findings and must state when authoritative context is unavailable. Consequential mutations remain outside the conversational path unless explicitly confirmed through a governed action workflow.

## V20 — LLM Qualification

Open **LLM QA** to test the configured Ollama communication layer. The suite checks context, lifecycle, evidence, lineage, claim boundaries, uncertainty, hallucination resistance, action safety, and contradiction handling.

A passing screening result does **not** mean the model is scientifically validated. Production qualification requires verification against authoritative research objects and human review. Qualification never authorizes experiment mutations.


## V24 — Evidence-Backed Agent Trace

The Agent Trace is now populated from the selected archived market cell. Choose a completed cell, then select a trader to inspect private value, recorded decision, order, book status, and trade. The trace is read-only and fails closed when authoritative archived evidence is unavailable; it never substitutes demonstration values.
