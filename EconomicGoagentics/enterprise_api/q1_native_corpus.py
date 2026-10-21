"""Derived native-JSON corpus. Never writes into the original campaign run."""
from __future__ import annotations

import json
from pathlib import Path

from enterprise_api import campaign_recovery as recovery

KINDS = ("cell_evidence", "cell_observations", "cell_snapshots")


def corpus_root(project_root: Path) -> Path:
    return Path(project_root) / "results" / "q1_milestone" / "native_archive"


def native_payload(raw: bytes) -> tuple[bytes, dict]:
    text = raw.strip()
    trailing = False
    if text.endswith(b"\\n"):
        text = text[:-2].rstrip()
        trailing = True
    s = text.decode("utf-8")
    s, repaired = recovery.repair_archive_json_text(s)
    json.loads(s)
    out = (s.strip() + "\n").encode("utf-8")
    json.loads(out)
    return out, {
        "repaired_price_side": repaired,
        "trailing_escaped_newline_stripped": trailing,
        "already_native": not repaired and not trailing,
    }


def emit_native_archive(run_dir: Path, dest: Path) -> dict:
    run_dir = Path(run_dir).resolve()
    dest = Path(dest).resolve()
    nested = False
    try:
        dest.relative_to(run_dir)
        nested = True
    except ValueError:
        nested = False
    if nested:
        raise ValueError("refusing to write the native corpus inside the campaign run directory")
    dest.mkdir(parents=True, exist_ok=True)
    files = []
    for kind in KINDS:
        src = run_dir / kind
        if not src.is_dir():
            continue
        (dest / kind).mkdir(parents=True, exist_ok=True)
        for path in sorted(src.glob("cell_*.json")):
            raw = path.read_bytes()
            native, meta = native_payload(raw)
            out = dest / kind / path.name
            out.write_bytes(native)
            files.append({
                "kind": kind,
                "name": path.name,
                "index": int(path.stem.split("_")[1]) if path.stem.startswith("cell_") else None,
                "original_rel": str(path.relative_to(run_dir)),
                "native_rel": str(out.relative_to(dest)),
                "original_sha256": recovery.sha256_bytes(raw),
                "native_sha256": recovery.sha256_bytes(native),
                **meta,
            })
    manifest = {
        "source_run": run_dir.name,
        "source_run_original": recovery.is_original_interrupted_run(run_dir),
        "wrote_original_run": False,
        "n_files": len(files),
        "n_repaired_price_side": sum(1 for f in files if f["repaired_price_side"]),
        "n_trailing_escaped_newline_stripped": sum(1 for f in files if f["trailing_escaped_newline_stripped"]),
        "n_already_native": sum(1 for f in files if f["already_native"]),
        "transform": "strip literal \\\\n suffix; repair price-side quote; emit UTF-8 JSON with trailing newline",
        "files": files,
    }
    (dest / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    return manifest


def native_peer(original_path: Path, run_dir: Path, dest: Path) -> Path | None:
    try:
        rel = Path(original_path).resolve().relative_to(Path(run_dir).resolve())
    except ValueError:
        return None
    peer = Path(dest) / rel
    return peer if peer.is_file() else None


def native_trusted(original_path: Path, native_path: Path, manifest: dict) -> bool:
    orig = recovery.sha256_file(original_path)
    got = recovery.sha256_file(native_path)
    name = Path(original_path).name
    kind = Path(original_path).parent.name
    for row in manifest.get("files") or []:
        if row.get("name") == name and row.get("kind") == kind:
            return row.get("original_sha256") == orig and row.get("native_sha256") == got
    return False
