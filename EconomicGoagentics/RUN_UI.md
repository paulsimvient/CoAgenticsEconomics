# Run the workbench

One operator path. Do not open HTML files from disk.

```bash
bash start-workbench.sh 8787
```

(`chmod +x` is not required. iCloud often strips the execute bit, so `./start-workbench.sh` can fail with Permission denied.)

Then open **exactly**:

`http://127.0.0.1:8787/`

That page is Run → Process → Results. Plain-language HTML: `http://127.0.0.1:8787/how.html`.

## Run

1. Confirm the **Ollama** pill is on (Ollama at `http://127.0.0.1:11434`, with at least one model).
2. Click **Start smoke**. That is the first run: 2 models × 2 seeds. It skips freeze and Layer A.
3. Do **not** start another campaign while Process says RUNNING.

**Start full** stays disabled until **Freeze H01 defaults** and **Run Layer A** have both succeeded.

## Process

The bar is the Ollama campaign (`done / total`, current cell, ETA). Idle copy is **Idle — no campaign**. The C++ live-market chart on the same page is a different runner and is not this experiment.

## Results

Finished cells appear in the table as each cell completes. Click a row for the asked / replied trace. Empty while running means no cell has finished yet — not that the campaign failed.

## Advanced

`/live.html` is the Research Console (expect / qualify / evidence browser). It is not required to run, watch, or inspect a campaign.

`Failed to fetch` means the browser never reached this server. Open `http://127.0.0.1:8787/`, not `interface.html` from the filesystem.
