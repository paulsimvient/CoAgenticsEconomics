# Ollama live LLM path (local economics evidence)

Use a local [Ollama](https://ollama.com) OpenAI-compatible endpoint for Layer B / live market slices. No cloud API key required.

## Setup

```bash
# Ollama already serving models (example):
ollama list
# ensure llama3.2 (default) or set another:
# ollama pull llama3.2
```

## CLI

```bash
export COAGENTICS_LLM_PROVIDER=ollama
export COAGENTICS_LLM_BASE_URL=http://127.0.0.1:11434/v1
export COAGENTICS_LLM_MODEL=llama3.2   # or qwen3:8b, etc.

./build-dv026/dv026-workbench-runner ollama-slice 424242
./build-dv026/dv026-workbench-runner ollama-paired 424242

# opt-in CTest (defaults to Ollama if no OpenAI key):
COAGENTICS_LIVE_LLM=1 ctest --test-dir build-dv026 -R llm_live_opt_in --output-on-failure
```

## Workbench

```bash
./start-workbench.sh
```

**DV026 Evidence** → set **Ollama model** → **Run Ollama slice** or **Run Ollama paired** (Layer B ZI vs LLM).

## Claim boundary

- `live_llm: true` for these commands only  
- Still `darpa_claim_ready: false`  
- Observable outcomes / paired deltas only — not rationality or Phase II constructs  
- Captured transcript can be replayed offline via historical replay helpers  

## Env reference

| Variable | Default (Ollama) |
|---|---|
| `COAGENTICS_LLM_PROVIDER` | `ollama` |
| `COAGENTICS_LLM_BASE_URL` | `http://127.0.0.1:11434/v1` |
| `COAGENTICS_LLM_MODEL` | `llama3.2` |
| `COAGENTICS_LLM_API_KEY` | unused (`ollama` placeholder) |
