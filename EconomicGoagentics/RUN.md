# Run CoAgentics

For the local workbench (start a campaign, watch progress, inspect cells), use [RUN_UI.md](RUN_UI.md):

```bash
bash start-workbench.sh 8787
```

Then open `http://127.0.0.1:8787/`.

## Engine-only build

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Section 16 memory layer only

This isolates the new Section 16 code from optional libcurl/network linkage:

```bash
./scripts/run_section16_memory_smoke.sh
```

Expected:

```text
persistent_memory_agent_smoke PASS
```
