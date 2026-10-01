# EconomicGoagentics — DV026 Research Console v3

## Start

From the repository root:

```bash
./start-workbench.sh
```

Then open:

`http://127.0.0.1:8787`

The start script **reconfigures and rebuilds the current DV026 runner every time** so an older `dv026-workbench-runner` cannot silently remain in use after source changes.

To use another port:

```bash
./start-workbench.sh 8788
```

## Live LLM runtime

The console supports live LLMs. The runtime card distinguishes:

- **READY** — runner works, Ollama responds, and at least one configured model is installed.
- **OFFLINE** — the console works but Ollama is not reachable.
- **ERROR** — the DV026 runner or runtime check returned an error.

A runtime check is bounded; the UI cannot remain indefinitely on “Checking LLM runtime…”.

## Ollama

If Ollama is installed but not running, start it using the normal Ollama service/application, then press **Refresh LLM availability**.

The configured local catalog contains 10 distinct model identities. The UI reports exactly how many are installed; it does not infer availability.

## Experimental controls

The live campaign is intentionally gated by:

1. live LLM runtime availability;
2. installed model availability;
3. successful 5-trial market qualification.

The progress meter reports completed cells, total expected cells, current model, current seed, and ETA when enough information exists.

## Scientific UI boundary

The interface distinguishes:

- agent-visible information;
- experiment/audit metadata;
- live LLM execution;
- archival/literature human reference data.

It does not imply new human-subject data collection.
