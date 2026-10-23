#!/usr/bin/env python3
"""Independently prove LLM-to-LLM trades from a heterogeneous campaign cell.

Read-only. Parses raw Ollama responses, links them to submitted bids and fills,
recomputes surplus and conservation. Does not write into the source run.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from enterprise_api.campaign_recovery import decode_json_document  # noqa: E402


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def extract_action(raw: str) -> dict | None:
    if not raw or not isinstance(raw, str):
        return None
    text = raw.strip()
    fence = re.search(r"```(?:json)?\s*(\{.*?\})\s*```", text, re.S | re.I)
    if fence:
        text = fence.group(1)
    start = text.find("{")
    end = text.rfind("}")
    if start < 0 or end <= start:
        return None
    try:
        obj = json.loads(text[start : end + 1])
    except json.JSONDecodeError:
        return None
    if not isinstance(obj, dict):
        return None
    action = str(obj.get("action") or "").upper()
    if action not in {"BUY", "SELL", "HOLD"}:
        return None
    qty = obj.get("quantity", 0)
    price = obj.get("price")
    return {
        "action": action,
        "quantity": qty,
        "price": price,
        "asset": obj.get("asset"),
        "time": obj.get("time"),
    }


def load_cell(run_dir: Path, index: int) -> dict:
    ev = run_dir / "cell_evidence" / f"cell_{index}.json"
    obs = run_dir / "cell_observations" / f"cell_{index}.json"
    snap = run_dir / "cell_snapshots" / f"cell_{index}.json"
    evidence, ev_meta = decode_json_document(ev.read_bytes())
    observation, obs_meta = decode_json_document(obs.read_bytes())
    snapshot, snap_meta = (decode_json_document(snap.read_bytes()) if snap.is_file() else ({}, {}))
    return {
        "evidence": evidence,
        "observation": observation,
        "snapshot": snapshot,
        "paths": {"evidence": str(ev), "observation": str(obs), "snapshot": str(snap) if snap.is_file() else None},
        "hashes": {
            "evidence": sha256(ev),
            "observation": sha256(obs),
            "snapshot": sha256(snap) if snap.is_file() else None,
        },
        "read_meta": {"evidence": ev_meta, "observation": obs_meta, "snapshot": snap_meta},
    }


def prove(run_dir: Path, index: int = 0) -> dict:
    packed = load_cell(run_dir, index)
    ev = packed["evidence"]
    obs = packed["observation"]
    assignment = {a["seat"]: a for a in ev.get("assignment") or []}
    if not assignment:
        for t in obs.get("agent_turns") or []:
            aid = t.get("agent_id")
            if not aid or aid in assignment:
                continue
            try:
                req = json.loads(t.get("canonical_request") or "{}")
            except json.JSONDecodeError:
                req = {}
            agent = req.get("agent") or {}
            role_raw = str(agent.get("role") or "").upper()
            assignment[aid] = {
                "seat": aid,
                "model": t.get("model"),
                "provider": t.get("provider"),
                "version": t.get("version"),
                "role": "buyer" if role_raw == "BUYER" else "seller",
                "private_value_or_cost": agent.get("private_value"),
            }
    turns = obs.get("agent_turns") or []
    bids = obs.get("bids") or []
    trades = obs.get("trades") or []
    errors = []

    seats = set(assignment)
    if len(assignment) != 10:
        errors.append(f"expected 10 seats, found {len(assignment)}")
    roles = [a["role"] for a in assignment.values()]
    if roles.count("buyer") != 5 or roles.count("seller") != 5:
        errors.append(f"role split {roles.count('buyer')} buy / {roles.count('seller')} sell")
    models = {a["model"] for a in assignment.values()}
    providers = {a.get("provider") for a in assignment.values()}
    if any(p != "ollama" for p in providers):
        errors.append(f"non-ollama providers: {sorted(providers)}")
    if "mock" in {m.split(":")[0] for m in models}:
        errors.append("mock model identity present")

    parsed_turns = []
    empty_raw = 0
    for t in turns:
        raw = t.get("raw_provider_response") or ""
        if not raw.strip():
            empty_raw += 1
        parsed = extract_action(raw)
        parsed_turns.append({**t, "parsed": parsed})
        if t.get("provider") != "ollama":
            errors.append(f"{t.get('agent_id')}: provider {t.get('provider')}")
        if t.get("agent_id") not in seats:
            errors.append(f"turn for unknown seat {t.get('agent_id')}")
    if empty_raw:
        errors.append(f"{empty_raw} turns with empty raw_provider_response")
    if len({t.get("agent_id") for t in turns}) < 10:
        errors.append(f"inference seats {len({t.get('agent_id') for t in turns})} < 10")

    def last_order(agent_id: str, side: str, t: int):
        cands = [b for b in bids if b.get("agent_id") == agent_id and b.get("side") == side and b.get("time") <= t]
        return cands[-1] if cands else None

    def last_turn(agent_id: str, action: str, t: int):
        cands = [
            x
            for x in parsed_turns
            if x.get("agent_id") == agent_id
            and x.get("parsed")
            and x["parsed"]["action"] == action
            and (x.get("time") or 0) <= t
        ]
        return cands[-1] if cands else None

    proven = []
    for i, trade in enumerate(trades):
        buyer_id = trade.get("buyer")
        seller_id = trade.get("seller")
        qty = trade.get("quantity")
        price = trade.get("price")
        t = trade.get("time") or 0
        rec = {
            "index": i,
            "time": t,
            "quantity": qty,
            "price": price,
            "buyer": buyer_id,
            "seller": seller_id,
            "llm_to_llm": False,
        }
        b = assignment.get(buyer_id)
        s = assignment.get(seller_id)
        if not b or b.get("role") != "buyer":
            rec["error"] = "buyer not in assignment"
            errors.append(f"trade {i}: {rec['error']}")
            proven.append(rec)
            continue
        if not s or s.get("role") != "seller":
            rec["error"] = "seller not in assignment"
            errors.append(f"trade {i}: {rec['error']}")
            proven.append(rec)
            continue
        rec["buyer_model"] = b["model"]
        rec["seller_model"] = s["model"]
        rec["buyer_private_value"] = b["private_value_or_cost"]
        rec["seller_private_cost"] = s["private_value_or_cost"]
        rec["buyer_surplus"] = qty * (b["private_value_or_cost"] - price) if isinstance(qty, int) else None
        rec["seller_surplus"] = qty * (price - s["private_value_or_cost"]) if isinstance(qty, int) else None
        rec["total_surplus"] = (
            rec["buyer_surplus"] + rec["seller_surplus"]
            if rec["buyer_surplus"] is not None and rec["seller_surplus"] is not None
            else None
        )
        bt = last_turn(buyer_id, "BUY", t)
        st = last_turn(seller_id, "SELL", t)
        rec["buyer_order"] = last_order(buyer_id, "buy", t)
        rec["seller_order"] = last_order(seller_id, "sell", t)
        rec["buyer_raw"] = (bt or {}).get("raw_provider_response")
        rec["seller_raw"] = (st or {}).get("raw_provider_response")
        rec["buyer_parsed"] = (bt or {}).get("parsed")
        rec["seller_parsed"] = (st or {}).get("parsed")
        rec["buyer_request_id"] = (bt or {}).get("request_id")
        rec["seller_request_id"] = (st or {}).get("request_id")
        rec["buyer_latency_ms"] = (bt or {}).get("latency_ms")
        rec["seller_latency_ms"] = (st or {}).get("latency_ms")
        ok = bool(
            rec["buyer_raw"]
            and rec["seller_raw"]
            and rec["buyer_parsed"]
            and rec["seller_parsed"]
            and rec["buyer_order"]
            and rec["seller_order"]
            and b.get("provider") == "ollama"
            and s.get("provider") == "ollama"
        )
        rec["llm_to_llm"] = ok
        if not ok:
            rec["error"] = "missing independently verifiable model-generated orders on one or both sides"
            errors.append(f"trade {i}: {rec['error']}")
        proven.append(rec)

    cash = {seat: 10000.0 for seat in assignment}
    inv = {seat: (1 if assignment[seat]["role"] == "seller" else 0) for seat in assignment}
    for trade in trades:
        q, p = trade["quantity"], trade["price"]
        cash[trade["buyer"]] -= q * p
        cash[trade["seller"]] += q * p
        inv[trade["buyer"]] += q
        inv[trade["seller"]] -= q
    conservation = {
        "cash_sum": sum(cash.values()),
        "cash_sum_expected": 10000.0 * len(assignment),
        "inventory_sum": sum(inv.values()),
        "inventory_sum_expected": 5,
        "negative_inventory": [seat for seat, v in inv.items() if v < 0],
        "negative_cash": [seat for seat, v in cash.items() if v < -1e-9],
    }
    if abs(conservation["cash_sum"] - conservation["cash_sum_expected"]) > 1e-6:
        errors.append(f"cash not conserved: {conservation['cash_sum']}")
    if conservation["inventory_sum"] != conservation["inventory_sum_expected"]:
        errors.append(f"inventory not conserved: {conservation['inventory_sum']}")
    if conservation["negative_inventory"] or conservation["negative_cash"]:
        errors.append("negative cash or inventory after replay")

    competitive = 0.0
    buyers = sorted(
        [a for a in assignment.values() if a["role"] == "buyer"],
        key=lambda a: -a["private_value_or_cost"],
    )
    sellers = sorted(
        [a for a in assignment.values() if a["role"] == "seller"],
        key=lambda a: a["private_value_or_cost"],
    )
    for b, s in zip(buyers, sellers):
        gap = b["private_value_or_cost"] - s["private_value_or_cost"]
        if gap > 0:
            competitive += gap
    realized = sum(t["total_surplus"] or 0 for t in proven)
    eta = (100.0 * realized / competitive) if competitive > 0 else None
    reported = ev.get("efficiency")
    eta_match = (
        eta is not None
        and reported is not None
        and math.isfinite(float(reported))
        and abs(eta - float(reported)) < 0.51
    )

    llm_to_llm = sum(1 for t in proven if t.get("llm_to_llm"))
    return {
        "run_dir": str(run_dir),
        "index": index,
        "seed": ev.get("seed"),
        "design": ev.get("design"),
        "status": ev.get("status"),
        "agents": len(assignment),
        "distinct_models": sorted(models),
        "inference_calls": len(turns),
        "submitted_orders": len(bids),
        "executed_trades": len(trades),
        "verified_llm_to_llm_trades": llm_to_llm,
        "reported_efficiency": reported,
        "independent_eta_percent": eta,
        "competitive_surplus": competitive,
        "realized_surplus": realized,
        "eta_matches_reported": eta_match,
        "conservation": conservation,
        "ending_cash": cash,
        "ending_inventory": inv,
        "trades": proven,
        "files": packed["paths"],
        "sha256": packed["hashes"],
        "observation_repaired_on_read": bool((packed.get("read_meta") or {}).get("observation", {}).get("repaired_on_read")),
        "wrote_archive": False,
        "errors": errors,
        "qualified_llm_to_llm": llm_to_llm >= 1 and not any("mock" in e for e in errors),
    }


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("run_dir")
    p.add_argument("--index", type=int, default=0)
    p.add_argument("--output")
    args = p.parse_args()
    result = prove(Path(args.run_dir), args.index)
    text = json.dumps(result, indent=2) + "\n"
    if args.output:
        Path(args.output).write_text(text)
    else:
        print(text)


if __name__ == "__main__":
    main()
