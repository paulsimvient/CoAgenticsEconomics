"""Q1 campaign recovery: process identity, interrupted state, immutable completed cells."""
from __future__ import annotations

import hashlib
import json
import os
import subprocess
import time
import uuid
from pathlib import Path

DESIGNS = ("frozen_snapshot", "sequential_interaction")
SESSION_NAME = "execution_session.json"
MANIFEST_NAME = "run_manifest.json"
AUDIT_NAME = "recovery_audit.jsonl"
GATE_NAME = "dv026_layer_a_gate.json"
PROTECTED_NOTE = "Completed cell evidence is immutable. Recovery must skip, never overwrite."
ORIGINAL_RUN_ID = "run_1792582821581_6712"
# Cells 0–83 from the interrupted 120-cell run are irreplaceable observations.

def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()

def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()

def canonical_json(obj) -> str:
    return json.dumps(obj, sort_keys=True, separators=(",", ":"), ensure_ascii=False)

def config_fingerprint(cfg: dict) -> str:
    keys = ("campaign_id", "base_seed", "n_seeds", "repeats", "designs", "population_design", "activation_design")
    slim = {k: cfg[k] for k in keys if k in cfg}
    return sha256_bytes(canonical_json(slim).encode())

def assignment_plan(base_seed: int, n_seeds: int, repeats: int, designs: list[str]) -> list[dict]:
    cells = []
    i = 0
    for si in range(n_seeds):
        seed = base_seed + si
        for design in designs:
            for rep in range(repeats):
                cells.append({"index": i, "seed": seed, "repeat": rep, "design": design})
                i += 1
    return cells

def atomic_write_bytes(path: Path, data: bytes, *, overwrite: bool = False) -> None:
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists() and not overwrite:
        raise FileExistsError(f"refusing to overwrite {path}; {PROTECTED_NOTE}")
    tmp = path.with_name(path.name + f".tmp.{os.getpid()}.{uuid.uuid4().hex[:8]}")
    fd = os.open(str(tmp), os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o644)
    try:
        os.write(fd, data)
        os.fsync(fd)
    finally:
        os.close(fd)
    os.replace(str(tmp), str(path))

def atomic_write_json(path: Path, obj, *, overwrite: bool = False) -> None:
    atomic_write_bytes(path, (canonical_json(obj) + "\n").encode(), overwrite=overwrite)

def append_audit(run_dir: Path, event: dict) -> None:
    path = Path(run_dir) / AUDIT_NAME
    row = dict(event)
    row.setdefault("ts", time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()))
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("a", encoding="utf-8") as f:
        f.write(json.dumps(row, sort_keys=True) + "\n")
        f.flush()
        os.fsync(f.fileno())

def _decode_json_bytes(raw: bytes):
    text = raw.strip()
    if text.endswith(b"\\n"):
        text = text[:-2].rstrip()
    return json.loads(text)

def _load_json(path: Path):
    if not path.is_file():
        return None
    raw = path.read_bytes()
    if not raw.strip():
        return {"_error": "empty", "_truncated": True}
    try:
        return _decode_json_bytes(raw)
    except json.JSONDecodeError as e:
        return {"_error": f"json:{e}", "_truncated": not raw.rstrip().rstrip(b"\\n").endswith(b"}")}

def inspect_pid(pid: int) -> dict | None:
    if not isinstance(pid, int) or pid <= 0:
        return None
    try:
        r = subprocess.run(
            ["ps", "-p", str(pid), "-o", "pid=", "-o", "command="],
            capture_output=True, text=True, timeout=2,
        )
    except Exception:
        return None
    line = (r.stdout or "").strip()
    if r.returncode != 0 or not line:
        return None
    parts = line.split(None, 1)
    try:
        got = int(parts[0])
    except Exception:
        return None
    if got != pid:
        return None
    cmd = parts[1] if len(parts) > 1 else ""
    return {"pid": pid, "cmdline": cmd, "alive": True}

def identity_matches_session(session: dict, identity: dict | None) -> bool:
    if not session or not identity:
        return False
    if int(session.get("pid") or 0) != int(identity.get("pid") or -1):
        return False
    cmd = str(identity.get("cmdline") or "")
    exe = str(session.get("exe") or "")
    token = str(session.get("command_token") or "hetero-campaign")
    if token not in cmd:
        return False
    if exe:
        name = Path(exe).name
        if name not in cmd and exe not in cmd:
            return False
    return True

def latest_hetero_run(root: Path) -> Path | None:
    runs = root / "results" / "dv026_heterogeneous_campaign" / "runs"
    if not runs.is_dir():
        return None
    dirs = [p for p in runs.iterdir() if p.is_dir() and p.name.startswith("run_")]
    if not dirs:
        return None
    return max(dirs, key=lambda p: p.stat().st_mtime)

def inventory_run(run_dir: Path, planned: int | None = None) -> dict:
    run_dir = Path(run_dir)
    errors = []
    ev_dir = run_dir / "cell_evidence"
    obs_dir = run_dir / "cell_observations"
    snap_dir = run_dir / "cell_snapshots"
    completed = {}
    corrupt = []
    duplicates = []
    seen = {}
    for path in sorted(ev_dir.glob("cell_*.json")) if ev_dir.is_dir() else []:
        raw = path.read_bytes()
        digest = sha256_bytes(raw)
        try:
            doc = _decode_json_bytes(raw)
        except json.JSONDecodeError:
            corrupt.append(path.name)
            continue
        idx = doc.get("index")
        if path.name != f"cell_{idx}.json":
            errors.append(f"{path.name}: filename/index mismatch")
            continue
        if idx in seen:
            duplicates.append(idx)
            continue
        seen[idx] = digest
        if doc.get("status") == "completed":
            completed[int(idx)] = {
                "index": int(idx),
                "seed": doc.get("seed"),
                "repeat": doc.get("repeat"),
                "design": doc.get("design"),
                "sha256": digest,
                "bytes": len(raw),
            }
        else:
            errors.append(f"{path.name}: non-completed status {doc.get('status')!r}")
    obs_ok = {}
    for path in sorted(obs_dir.glob("cell_*.json")) if obs_dir.is_dir() else []:
        raw = path.read_bytes()
        try:
            doc = _decode_json_bytes(raw)
        except json.JSONDecodeError:
            corrupt.append("obs:" + path.name)
            continue
        idx = doc.get("index")
        obs_ok[idx] = sha256_bytes(raw)
        if idx in completed:
            for key in ("seed", "repeat", "design"):
                if doc.get(key) != completed[idx].get(key) and not (key == "index"):
                    errors.append(f"observation {idx}: {key} mismatch")
    snap_ok = {}
    for path in sorted(snap_dir.glob("cell_*.json")) if snap_dir.is_dir() else []:
        raw = path.read_bytes()
        try:
            _decode_json_bytes(raw)
            idx = int(path.name[5:-5])
            snap_ok[idx] = sha256_bytes(raw)
        except Exception:
            corrupt.append("snap:" + path.name)
    for idx in list(completed):
        if idx not in obs_ok:
            errors.append(f"cell {idx}: completed evidence without observation archive")
        if snap_dir.exists() and idx not in snap_ok:
            errors.append(f"cell {idx}: completed evidence without snapshot")
    manifest = _load_json(run_dir / MANIFEST_NAME)
    reconstructed = False
    if not isinstance(manifest, dict) or manifest.get("_error"):
        reconstructed = True
        if completed:
            seeds = sorted({c["seed"] for c in completed.values() if isinstance(c.get("seed"), int)})
            repeats = sorted({c["repeat"] for c in completed.values() if isinstance(c.get("repeat"), int)})
            designs = []
            for d in DESIGNS:
                if any(c.get("design") == d for c in completed.values()):
                    designs.append(d)
            base = min(seeds) if seeds else 424242
            n_seeds = (max(seeds) - base + 1) if seeds else 0
            # If the run was interrupted mid-seed, n_seeds is the planned width, not max-seen.
            # Prefer 20 when geometry matches 20*len(designs)*len(repeats).
            rep_n = (max(repeats) + 1) if repeats else 3
            guessed = n_seeds * max(1, len(designs)) * max(1, rep_n)
            if planned is None:
                planned = 120 if guessed <= 120 and base == 424242 else guessed
            manifest = {
                "campaign_id": run_dir.name,
                "base_seed": base,
                "n_seeds": planned // (max(1, len(designs)) * max(1, rep_n)) if planned else n_seeds,
                "seeds": planned // (max(1, len(designs)) * max(1, rep_n)) if planned else n_seeds,
                "repeats": rep_n,
                "designs": designs or list(DESIGNS),
                "cells_planned": planned or guessed,
                "reconstructed": True,
            }
            manifest["n_seeds"] = int(manifest["cells_planned"]) // (len(manifest["designs"]) * manifest["repeats"])
            manifest["seeds"] = manifest["n_seeds"]
        else:
            manifest = {"cells_planned": planned or 0, "reconstructed": True, "base_seed": 424242, "n_seeds": 0, "repeats": 3, "designs": list(DESIGNS)}
    planned_n = int(manifest.get("cells_planned") or planned or 0)
    designs = list(manifest.get("designs") or DESIGNS)
    repeats = int(manifest.get("repeats") or 3)
    base_seed = int(manifest.get("base_seed") or 424242)
    n_seeds = int(manifest.get("n_seeds") or manifest.get("seeds") or 0)
    plan = assignment_plan(base_seed, n_seeds, repeats, designs)
    if planned_n and len(plan) != planned_n:
        errors.append(f"plan length {len(plan)} != cells_planned {planned_n}")
    for cell in completed.values():
        if cell["index"] < len(plan):
            exp = plan[cell["index"]]
            if cell["seed"] != exp["seed"] or cell["repeat"] != exp["repeat"] or cell["design"] != exp["design"]:
                errors.append(f"cell {cell['index']}: assignment disagrees with plan")
    unfinished = [c for c in plan if c["index"] not in completed]
    attempts = []
    att_dir = run_dir / "cell_attempts"
    if att_dir.is_dir():
        attempts = sorted(p.name for p in att_dir.iterdir() if p.is_file())
    tmp_orphans = [p.name for p in run_dir.rglob("*.tmp*") if p.is_file()]
    cfg = {
        "campaign_id": str(manifest.get("campaign_id") or run_dir.name),
        "base_seed": base_seed,
        "n_seeds": n_seeds,
        "repeats": repeats,
        "designs": designs,
        "population_design": "heterogeneous_multi_llm",
        "activation_design": "both" if len(designs) > 1 else (designs[0] if designs else "sequential_interaction"),
    }
    return {
        "run_dir": str(run_dir),
        "ok": not errors and not corrupt and not duplicates,
        "errors": errors,
        "corrupt_files": corrupt,
        "duplicate_indices": duplicates,
        "completed": len(completed),
        "completed_indices": sorted(completed),
        "completed_sha256": {i: completed[i]["sha256"] for i in sorted(completed)},
        "observation_sha256": {i: obs_ok[i] for i in sorted(k for k in obs_ok if isinstance(k, int))},
        "unfinished": unfinished,
        "unfinished_count": len(unfinished),
        "planned": len(plan) or planned_n,
        "last_completed_index": max(completed) if completed else None,
        "manifest": manifest,
        "manifest_reconstructed": reconstructed,
        "config": cfg,
        "config_fingerprint": config_fingerprint(cfg),
        "attempts": attempts,
        "tmp_orphans": tmp_orphans,
        "protected_completed": True,
        "original_run": is_original_interrupted_run(run_dir),
        "immutable_completed_note": PROTECTED_NOTE,
    }

def load_session(run_dir: Path) -> dict | None:
    doc = _load_json(Path(run_dir) / SESSION_NAME)
    if not isinstance(doc, dict) or doc.get("_error"):
        return None
    return doc

def write_session(run_dir: Path, session: dict, *, overwrite: bool = True) -> None:
    atomic_write_json(Path(run_dir) / SESSION_NAME, session, overwrite=overwrite)

def apply_live_session(rec: dict, session: dict | None, inspect=inspect_pid) -> dict:
    """Overlay RUNNING only when pid+exe+command_token still match a live process."""
    rec = dict(rec)
    identity = inspect(int(session["pid"])) if session and session.get("pid") else None
    live = identity_matches_session(session or {}, identity)
    if live:
        rec["process_live"] = True
        rec["execution_state"] = "running"
        rec["running"] = True
        rec["interrupted"] = False
        rec["complete"] = False
        rec["display"] = None
        rec["session"] = session
    return rec

def completed_cell_path(run_dir: Path, index: int) -> Path:
    return Path(run_dir) / "cell_evidence" / f"cell_{index}.json"

def is_original_interrupted_run(run_dir: Path) -> bool:
    return Path(run_dir).name == ORIGINAL_RUN_ID

def original_completed_range() -> range:
    return range(84)

def reconcile(run_dir: Path, inspect=inspect_pid, *, persist: bool = True) -> dict:
    run_dir = Path(run_dir)
    inv = inventory_run(run_dir)
    session = load_session(run_dir)
    identity = inspect(int(session["pid"])) if session and session.get("pid") else None
    live = identity_matches_session(session or {}, identity)
    state = "idle"
    if live:
        state = "running"
    elif inv["completed"] and inv["unfinished_count"] == 0:
        state = "complete"
    elif session and session.get("status") == "cancelled":
        state = "stopped"
    elif inv["completed"] or (session and session.get("status") in ("running", "interrupted")):
        state = "interrupted"
        if persist and session and session.get("status") != "interrupted":
            session = dict(session)
            session["status"] = "interrupted"
            session["interrupted_at"] = time.time()
            session["interrupt_reason"] = "process_not_live"
            write_session(run_dir, session)
            append_audit(run_dir, {
                "action": "campaign.interrupted",
                "reason": "process identity not live at reconcile",
                "pid": session.get("pid"),
                "completed": inv["completed"],
                "planned": inv["planned"],
            })
    elif inv["tmp_orphans"] or inv["corrupt_files"]:
        state = "interrupted"
    out = {
        "execution_state": state,
        "running": state == "running",
        "interrupted": state == "interrupted",
        "complete": state == "complete",
        "eta_sec": None if state != "running" else None,
        "done": inv["completed"],
        "total": inv["planned"],
        "remaining": inv["unfinished_count"],
        "last_completed_cell": inv["last_completed_index"],
        "last_execution": "interrupted" if state == "interrupted" else state,
        "process_live": live,
        "session": session,
        "inventory": inv,
        "display": (
            f"Interrupted — {inv['completed']}/{inv['planned']} cells completed"
            if state == "interrupted"
            else None
        ),
    }
    if state != "running":
        out["eta_sec"] = None
    return out

def required_models_from_observations(run_dir: Path) -> list[str]:
    models = []
    seen = set()
    obs_dir = Path(run_dir) / "cell_observations"
    if not obs_dir.is_dir():
        return models
    for path in sorted(obs_dir.glob("cell_*.json")):
        doc = _load_json(path)
        if not isinstance(doc, dict):
            continue
        for turn in doc.get("agent_turns") or []:
            m = turn.get("model")
            if m and m not in seen:
                seen.add(m)
                models.append(m)
    return models

def verify_recovery(run_dir: Path, ollama_models: list[str] | None = None, inspect=inspect_pid, *, persist: bool = False) -> dict:
    rec = reconcile(run_dir, inspect=inspect, persist=persist)
    inv = rec["inventory"]
    reasons = []
    evidence = "failed"
    if inv["ok"] and not inv["corrupt_files"] and not inv["duplicate_indices"]:
        evidence = "passed"
    elif inv["completed"] and (inv["errors"] or inv["corrupt_files"]):
        evidence = "failed"
        reasons.extend(inv["errors"][:12])
        reasons.extend([f"corrupt {n}" for n in inv["corrupt_files"][:8]])
    else:
        evidence = "pending"
        reasons.extend(inv["errors"][:12])
    if rec["running"]:
        reasons.append("another campaign process is active")
    if inv["duplicate_indices"]:
        reasons.append(f"duplicate completed-cell identifiers: {inv['duplicate_indices']}")
    stored_fp = (inv.get("manifest") or {}).get("config_fingerprint")
    if stored_fp and stored_fp != inv["config_fingerprint"]:
        reasons.append("configuration fingerprint mismatch")
        evidence = "failed"
    required = required_models_from_observations(run_dir)
    missing_models = []
    if ollama_models is not None:
        have = set(ollama_models)
        missing_models = [m for m in required if m not in have]
        if missing_models:
            reasons.append("missing or changed Ollama model: " + ", ".join(missing_models[:8]))
    resumable = (
        rec["interrupted"]
        and evidence == "passed"
        and not rec["running"]
        and inv["unfinished_count"] > 0
        and not missing_models
        and not inv["duplicate_indices"]
        and not inv["corrupt_files"]
    )
    if inv["unfinished_count"] == 0 and inv["completed"]:
        reasons.append("no unfinished assignments")
        resumable = False
    return {
        **rec,
        "evidence_verification": evidence,
        "resume_eligible": resumable,
        "resume_blocked_reasons": reasons,
        "required_models": required,
        "missing_models": missing_models,
        "protected_completed_indices": inv["completed_indices"],
        "note": PROTECTED_NOTE,
    }

def skip_completed(plan: list[dict], completed_indices: list[int]) -> list[dict]:
    done = set(completed_indices)
    return [c for c in plan if c["index"] not in done]

def snapshot_completed_hashes(run_dir: Path) -> dict:
    """SHA-256 of every completed evidence file. Used to prove originals were not rewritten."""
    return dict(inventory_run(run_dir)["completed_sha256"])

def execute_resume(run_dir: Path, execute_cell, inspect=inspect_pid, ollama_models=None, session=None) -> dict:
    """Run only unfinished assignments. execute_cell(assignment, run_dir) must not write completed cells.
    If resume is blocked, completed evidence is left byte-identical — originals cannot be reconstructed."""
    v = verify_recovery(run_dir, ollama_models=ollama_models, inspect=inspect)
    if not v["resume_eligible"]:
        raise ValueError("recovery is unsafe: " + "; ".join(v["resume_blocked_reasons"] or ["not eligible"]))
    if v["running"]:
        raise ValueError("resume invoked while another campaign process is active")
    before = dict(v["inventory"]["completed_sha256"])
    remaining = skip_completed(v["inventory"]["unfinished"], [])
    sid = str(uuid.uuid4())
    sess = dict(session or {})
    sess.update({
        "session_id": sid,
        "status": "running",
        "recovery_attempt": int(sess.get("recovery_attempt") or 0) + 1,
        "resumed_at": time.time(),
        "command_token": "hetero-campaign",
        "campaign_id": v["inventory"]["config"]["campaign_id"],
        "config_fingerprint": v["inventory"]["config_fingerprint"],
    })
    write_session(run_dir, sess)
    append_audit(run_dir, {"action": "campaign.resume", "session_id": sid, "remaining": len(remaining)})
    executed = []
    try:
        for cell in remaining:
            execute_cell(cell, Path(run_dir))
            executed.append(cell["index"])
        sess["status"] = "complete" if not inventory_run(run_dir)["unfinished_count"] else "interrupted"
        sess["finished_at"] = time.time()
        write_session(run_dir, sess)
        append_audit(run_dir, {"action": "campaign.resume.finish", "session_id": sid, "executed": executed})
    except Exception as e:
        sess["status"] = "interrupted"
        sess["last_error"] = str(e)
        write_session(run_dir, sess)
        append_audit(run_dir, {"action": "campaign.resume.error", "session_id": sid, "error": str(e)})
        raise
    after = inventory_run(run_dir)
    for idx, digest in before.items():
        if after["completed_sha256"].get(idx) != digest:
            raise RuntimeError(f"completed cell {idx} changed during resume; originals must remain byte-identical")
    return {"session_id": sid, "executed": executed, "inventory": after, "unchanged_completed": list(before)}

def persist_layer_a_gate(root: Path, payload: dict, *, config_fingerprint: str, code_fingerprint: str, evidence_path: str | None) -> dict:
    gate = {
        "validation_type": "layer-a",
        "validation_version": "1",
        "config_fingerprint": config_fingerprint,
        "code_fingerprint": code_fingerprint,
        "timestamp": time.time(),
        "pass": bool(payload.get("layer_a_pass") or payload.get("pass")),
        "evidence_path": evidence_path,
        "payload": payload,
    }
    atomic_write_json(Path(root) / "results" / GATE_NAME, gate, overwrite=True)
    return gate

def restore_layer_a_gate(root: Path, *, config_fingerprint: str, code_fingerprint: str) -> dict:
    path = Path(root) / "results" / GATE_NAME
    doc = _load_json(path)
    empty = {"layer_a_pass": False, "pass": False, "stale": True, "reason": "no persisted gate"}
    if not isinstance(doc, dict) or doc.get("_error"):
        return empty
    reasons = []
    if doc.get("validation_type") != "layer-a":
        reasons.append("validation type mismatch")
    if doc.get("config_fingerprint") != config_fingerprint:
        reasons.append("configuration fingerprint mismatch")
    if doc.get("code_fingerprint") != code_fingerprint:
        reasons.append("code/build fingerprint mismatch")
    ev = doc.get("evidence_path")
    if ev:
        p = Path(root) / ev if not Path(ev).is_absolute() else Path(ev)
        if not p.is_file():
            reasons.append("supporting evidence missing")
    if reasons:
        return {
            "layer_a_pass": False, "pass": False, "stale": True,
            "reason": "; ".join(reasons), "persisted_pass": bool(doc.get("pass")),
        }
    payload = doc.get("payload") if isinstance(doc.get("payload"), dict) else {}
    payload = dict(payload)
    payload["layer_a_pass"] = bool(doc.get("pass"))
    payload["pass"] = bool(doc.get("pass"))
    payload["stale"] = False
    payload["restored"] = True
    return payload

def export_recovery_report(run_dir: Path, dest: Path, inspect=inspect_pid, ollama_models=None) -> Path:
    v = verify_recovery(run_dir, ollama_models=ollama_models, inspect=inspect)
    dest = Path(dest)
    atomic_write_json(dest, v, overwrite=True)
    return dest

def write_completed_cell(run_dir: Path, evidence: dict, observation: dict | None = None, snapshot: dict | None = None) -> None:
    """Atomic completed-cell write. Refuses to replace an existing completed file."""
    idx = int(evidence["index"])
    ev_path = Path(run_dir) / "cell_evidence" / f"cell_{idx}.json"
    if ev_path.exists():
        raise FileExistsError(f"refusing to overwrite completed evidence {ev_path}")
    if evidence.get("status") != "completed":
        att = Path(run_dir) / "cell_attempts" / f"cell_{idx}_{int(time.time())}.json"
        atomic_write_json(att, evidence, overwrite=False)
        return
    if observation is not None:
        atomic_write_json(Path(run_dir) / "cell_observations" / f"cell_{idx}.json", observation)
    if snapshot is not None:
        atomic_write_json(Path(run_dir) / "cell_snapshots" / f"cell_{idx}.json", snapshot)
    atomic_write_json(ev_path, evidence)
