# Ollama setup

1. Install and start Ollama.
2. Pull a local model, for example `ollama pull llama3.2`.
3. Start the workbench normally: `./start-workbench.sh 8787`.
4. Open **Ask CoAgentics**. The status line should report the configured model as connected.

Optional environment variables:
- `OLLAMA_URL=http://127.0.0.1:11434`
- `OLLAMA_MODEL=llama3.2`
- `OLLAMA_TIMEOUT=60`

The communication layer is grounded in the current research-object and DV026 state and is not itself a scientific evidence source.
