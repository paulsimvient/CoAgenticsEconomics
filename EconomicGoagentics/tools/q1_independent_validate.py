#!/usr/bin/env python3
"""CLI: independent Q1 validation. Read-only on campaign archives."""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from enterprise_api import campaign_recovery as recovery
from enterprise_api.q1_independent_validate import validate_cell, validate_run


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("run_directory")
    p.add_argument("--cell", type=int)
    p.add_argument("--output")
    p.add_argument("--featured-index", type=int)
    a = p.parse_args()
    run = Path(a.run_directory)
    if recovery.is_original_interrupted_run(run) and a.output:
        out = Path(a.output).resolve()
        if run.resolve() in out.parents or out.parent.resolve() == run.resolve():
            p.error("refusing to write derived reports inside the original campaign run directory")
    if a.cell is not None:
        result = validate_cell(run, a.cell)
    else:
        result = validate_run(run, featured_index=a.featured_index)
    rendered = json.dumps(result, indent=2, default=str) + "\n"
    if a.output:
        Path(a.output).parent.mkdir(parents=True, exist_ok=True)
        Path(a.output).write_text(rendered)
    else:
        sys.stdout.write(rendered)
    if not result.get("ok", True) and a.cell is not None:
        sys.exit(1)


if __name__ == "__main__":
    main()
