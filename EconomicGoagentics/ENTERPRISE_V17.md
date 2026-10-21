# CoAgentics Economics — Enterprise V17

## Clarity + Ollama Communication Layer

V17 adds a local Ollama-backed communication interface over authoritative CoAgentics research objects and DV026 state.

### API
- `GET /api/llm/status` — Ollama reachability and available models.
- `POST /api/llm/chat` — grounded communication request.

### Guardrails
The LLM receives compact application context and is instructed not to invent observations, evidence, statistics, causal findings, or hidden traits. Consequential mutations remain outside the communication endpoint.

### Configuration
- `OLLAMA_URL` default: `http://127.0.0.1:11434`
- `OLLAMA_MODEL` default: `llama3.2`
- `OLLAMA_TIMEOUT` default: `60` seconds

### User Guide
`USER_GUIDE.md` was updated as part of this release.
