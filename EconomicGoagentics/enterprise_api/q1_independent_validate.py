"""Independent economic and LLM-trace checks for Q1 heterogeneous cells.

Read-only. Never writes into a campaign run directory. Distinguishes mock
scripted transport from actual Ollama inference. Does not claim a scientific
qualification gate has passed; it reports what the archive actually contains.
"""
from __future__ import annotations

import json
import math
from collections import defaultdict
from pathlib import Path

from enterprise_api import campaign_recovery as recovery
from enterprise_api.q1_inference import infer_matched_eta

STARTING_CASH = 10000.0
ASSET = "ASSET"
CLAIM_BOUNDARY = (
    "Descriptive recomputation from archived files only. "
    "Mock transport is not Ollama inference. "
    "On-read JSON repair never rewrites archive bytes. "
    "Induced-value surplus counts only the first unit per seat, matching Market::metrics. "
    "Extra cash-financed units are recorded as trades but do not add redemption value. "
    "Paired frozen-vs-sequential differences are matched observations, "
    "not a causal ranking of models."
)


def _obj(text):
    if isinstance(text, dict):
        return text
    if not text or not isinstance(text, str):
        return {}
    try:
        o = json.loads(text)
        return o if isinstance(o, dict) else {}
    except json.JSONDecodeError:
        return {}


def inference_kind(turn: dict) -> str:
    provider = str(turn.get("provider") or "").lower()
    model = str(turn.get("model") or "")
    raw = str(turn.get("raw_provider_response") or "")
    if provider == "mock" or model.startswith("hetero-model-") or model.startswith("mock"):
        return "mock"
    if provider == "ollama" or "ollama" in provider:
        return "ollama"
    if "mock" in provider:
        return "mock"
    if raw and "ollama" in raw.lower() and provider:
        return "ollama"
    if raw:
        return "unknown_nonempty"
    return "empty"


def load_cell_files(run_dir: Path, index: int) -> dict:
    run_dir = Path(run_dir)
    name = f"cell_{index}.json"
    ev_path = run_dir / "cell_evidence" / name
    obs_path = run_dir / "cell_observations" / name
    snap_path = run_dir / "cell_snapshots" / name
    out = {
        "index": index,
        "evidence": None,
        "observation": None,
        "snapshot": None,
        "evidence_meta": {"ok": False, "path": str(ev_path), "wrote_archive": False},
        "observation_meta": {"ok": False, "path": str(obs_path), "wrote_archive": False},
        "snapshot_meta": {"ok": False, "path": str(snap_path), "wrote_archive": False},
        "evidence_sha256": recovery.sha256_file(ev_path) if ev_path.is_file() else None,
        "observation_sha256": recovery.sha256_file(obs_path) if obs_path.is_file() else None,
    }
    for key, path in (("evidence", ev_path), ("observation", obs_path), ("snapshot", snap_path)):
        if not path.is_file():
            continue
        try:
            obj, meta = recovery.decode_json_document(path.read_bytes())
            meta = dict(meta)
            meta["path"] = str(path)
            meta["wrote_archive"] = False
            out[key] = obj if isinstance(obj, dict) else None
            out[f"{key}_meta"] = meta
        except json.JSONDecodeError as e:
            out[f"{key}_meta"] = {"ok": False, "reason": str(e), "path": str(path), "wrote_archive": False}
    return out


class IndependentCDA:
    """Replay of Market.cpp: price-time via first-max, midpoint trade, cash/inventory checks."""

    def __init__(self, accounts: dict):
        self.accounts = {
            aid: {"cash": float(a["cash"]), "inventory": dict(a.get("inventory") or {})}
            for aid, a in accounts.items()
        }
        self.buys = []
        self.sells = []
        self.trades = []

    def submit(self, bid: dict) -> dict:
        aid = bid["agent_id"]
        qty = int(bid.get("quantity") or 0)
        price = float(bid.get("price"))
        side = str(bid.get("side") or "").lower()
        asset = str(bid.get("asset") or ASSET)
        if qty <= 0 or price < 0 or not math.isfinite(price) or aid not in self.accounts:
            return {"accepted": False, "reason": "invalid_order"}
        acct = self.accounts[aid]
        if side in ("buy", "bid") and acct["cash"] + 1e-9 < price * qty:
            return {"accepted": False, "reason": "insufficient_cash"}
        if side in ("sell", "ask") and acct["inventory"].get(asset, 0) < qty:
            return {"accepted": False, "reason": "insufficient_inventory"}
        order = {
            "time": bid.get("time"),
            "agent_id": aid,
            "asset": asset,
            "quantity": qty,
            "price": price,
            "side": "buy" if side in ("buy", "bid") else "sell",
        }
        (self.buys if order["side"] == "buy" else self.sells).append(order)
        self._match(asset, order["time"])
        return {"accepted": True}

    def _match(self, asset, time):
        while True:
            bi = None
            for i, b in enumerate(self.buys):
                if b["asset"] == asset and (bi is None or b["price"] > self.buys[bi]["price"]):
                    bi = i
            si = None
            for i, s in enumerate(self.sells):
                if s["asset"] == asset and (si is None or s["price"] < self.sells[si]["price"]):
                    si = i
            if bi is None or si is None:
                break
            buy, sell = self.buys[bi], self.sells[si]
            if buy["price"] < sell["price"]:
                break
            q = min(buy["quantity"], sell["quantity"])
            p = (buy["price"] + sell["price"]) / 2.0
            buyer = self.accounts[buy["agent_id"]]
            seller = self.accounts[sell["agent_id"]]
            q = min(q, int(math.floor((buyer["cash"] + 1e-9) / p)))
            q = min(q, int(seller["inventory"].get(asset, 0)))
            if q <= 0:
                break
            buyer["cash"] -= p * q
            buyer["inventory"][asset] = buyer["inventory"].get(asset, 0) + q
            seller["cash"] += p * q
            seller["inventory"][asset] = seller["inventory"].get(asset, 0) - q
            self.trades.append({
                "time": time, "asset": asset, "quantity": q, "price": p,
                "buyer": buy["agent_id"], "seller": sell["agent_id"],
            })
            buy["quantity"] -= q
            sell["quantity"] -= q
            if buy["quantity"] == 0:
                self.buys.pop(bi)
            if sell["quantity"] == 0:
                self.sells.pop(si)

    def totals(self):
        cash = sum(a["cash"] for a in self.accounts.values())
        inv = sum(sum(v for v in a["inventory"].values()) for a in self.accounts.values())
        return cash, inv


def agents_from_observation(obs: dict, ev: dict | None = None) -> dict:
    agents = {}
    assignment = (ev or {}).get("assignment") if isinstance(ev, dict) else None
    if isinstance(assignment, list):
        for row in assignment:
            if not isinstance(row, dict):
                continue
            aid = row.get("seat") or row.get("agent_id")
            if not aid:
                continue
            role = str(row.get("role") or "").lower()
            inv = int(row.get("inventory") if row.get("inventory") is not None else (1 if role == "seller" else 0))
            agents[aid] = {
                "agent_id": aid,
                "model": row.get("model"),
                "provider": row.get("provider"),
                "role": role,
                "private_value": row.get("private_value_or_cost"),
                "cash": float(row.get("cash") if row.get("cash") is not None else STARTING_CASH),
                "inventory": {ASSET: inv},
                "inference": "mock" if str(row.get("provider") or "") == "mock" else None,
            }
    for turn in obs.get("agent_turns") or []:
        if not isinstance(turn, dict):
            continue
        aid = turn.get("agent_id")
        if not aid:
            continue
        req = _obj(turn.get("canonical_request"))
        ag = req.get("agent") if isinstance(req.get("agent"), dict) else {}
        row = agents.setdefault(aid, {
            "agent_id": aid, "model": None, "provider": None, "role": None,
            "private_value": None, "cash": STARTING_CASH, "inventory": {ASSET: 0},
            "inference": None,
        })
        row["model"] = row.get("model") or turn.get("model")
        row["provider"] = row.get("provider") or turn.get("provider")
        role = str(ag.get("role") or row.get("role") or "").lower()
        if role:
            row["role"] = role
        if ag.get("private_value") is not None:
            row["private_value"] = ag.get("private_value")
        if ag.get("cash") is not None:
            row["cash"] = float(ag.get("cash"))
        if ag.get("inventory") is not None:
            try:
                row["inventory"] = {ASSET: int(ag.get("inventory"))}
            except (TypeError, ValueError):
                pass
        kind = inference_kind(turn)
        if kind == "ollama":
            row["inference"] = "ollama"
        elif row.get("inference") is None:
            row["inference"] = kind
        row.setdefault("turns", []).append(turn)
    for aid, row in agents.items():
        if row.get("inventory") is None:
            row["inventory"] = {ASSET: 1 if row.get("role") == "seller" else 0}
        if not row.get("turns"):
            row["turns"] = []
    return agents


def max_and_realized_surplus(agents: dict, trades: list) -> dict:
    buyers = sorted(
        [(a["agent_id"], float(a["private_value"])) for a in agents.values()
         if a.get("role") == "buyer" and a.get("private_value") is not None],
        key=lambda x: -x[1],
    )
    sellers = sorted(
        [(a["agent_id"], float(a["private_value"])) for a in agents.values()
         if a.get("role") == "seller" and a.get("private_value") is not None],
        key=lambda x: x[1],
    )
    max_s = 0.0
    n_eff = 0
    for i in range(min(len(buyers), len(sellers))):
        gap = buyers[i][1] - sellers[i][1]
        if gap > 0:
            max_s += gap
            n_eff += 1
        else:
            break
    realized = 0.0
    used_b = defaultdict(int)
    used_s = defaultdict(int)
    missing = []
    extra_units = 0
    induced_units = 0
    for tr in trades:
        b = agents.get(tr.get("buyer"))
        s = agents.get(tr.get("seller"))
        qty = int(tr.get("quantity") or 0)
        if not b or not s or b.get("role") != "buyer" or s.get("role") != "seller":
            missing.append({"buyer": tr.get("buyer"), "seller": tr.get("seller")})
            continue
        if b.get("private_value") is None or s.get("private_value") is None:
            missing.append({"buyer": tr.get("buyer"), "seller": tr.get("seller"), "reason": "no_private_value"})
            continue
        for _ in range(qty):
            bi = used_b[b["agent_id"]]
            si = used_s[s["agent_id"]]
            used_b[b["agent_id"]] += 1
            used_s[s["agent_id"]] += 1
            # One induced value/cost per seat, same as Market::metrics.
            if bi == 0 and si == 0:
                realized += float(b["private_value"]) - float(s["private_value"])
                induced_units += 1
            else:
                extra_units += 1
    eff = 100.0 * realized / max_s if max_s > 0 else None
    return {
        "maximum_surplus": max_s,
        "realized_surplus": realized,
        "allocative_efficiency": eff,
        "efficient_quantity": n_eff,
        "induced_units": induced_units,
        "extra_units_excluded": extra_units,
        "surplus_definition": "induced_value_first_unit_per_seat",
        "trade_party_errors": missing,
    }


def trade_llm_sides(agents: dict, trades: list) -> dict:
    rows = []
    both = one = neither = 0
    mock_involved = 0
    for tr in trades:
        b = agents.get(tr.get("buyer")) or {}
        s = agents.get(tr.get("seller")) or {}
        bk = b.get("inference")
        sk = s.get("inference")
        b_llm = bk == "ollama"
        s_llm = sk == "ollama"
        if b_llm and s_llm:
            both += 1
            status = "both_sides_ollama"
        elif b_llm or s_llm:
            one += 1
            status = "one_side_ollama"
        else:
            neither += 1
            status = "neither_ollama"
        if bk == "mock" or sk == "mock":
            mock_involved += 1
        rows.append({
            "time": tr.get("time"),
            "price": tr.get("price"),
            "quantity": tr.get("quantity"),
            "buyer": tr.get("buyer"),
            "seller": tr.get("seller"),
            "buyer_model": b.get("model"),
            "seller_model": s.get("model"),
            "buyer_inference": bk,
            "seller_inference": sk,
            "buyer_private_value": b.get("private_value"),
            "seller_private_value": s.get("private_value"),
            "status": status,
        })
    return {
        "n_trades": len(trades),
        "both_sides_ollama": both,
        "one_side_ollama": one,
        "neither_ollama": neither,
        "mock_involved": mock_involved,
        "trades": rows,
    }


def trade_transcript(agents: dict, obs: dict, trades: list) -> list:
    out = []
    turns_by_agent = {aid: a.get("turns") or [] for aid, a in agents.items()}
    for tr in trades:
        buyer, seller = tr.get("buyer"), tr.get("seller")
        bt = next((t for t in reversed(turns_by_agent.get(buyer) or []) if t.get("time", 0) <= (tr.get("time") or 0)), None)
        st = next((t for t in reversed(turns_by_agent.get(seller) or []) if t.get("time", 0) <= (tr.get("time") or 0)), None)
        if bt is None and turns_by_agent.get(buyer):
            bt = turns_by_agent[buyer][-1]
        if st is None and turns_by_agent.get(seller):
            st = turns_by_agent[seller][-1]
        out.append({
            "trade": {
                "time": tr.get("time"), "price": tr.get("price"), "quantity": tr.get("quantity"),
                "buyer": buyer, "seller": seller, "asset": tr.get("asset"),
            },
            "buyer": {
                "agent_id": buyer,
                "model": (agents.get(buyer) or {}).get("model"),
                "role": (agents.get(buyer) or {}).get("role"),
                "private_value": (agents.get(buyer) or {}).get("private_value"),
                "inference": (agents.get(buyer) or {}).get("inference"),
                "canonical_request": (bt or {}).get("canonical_request"),
                "raw_provider_response": (bt or {}).get("raw_provider_response"),
                "provider": (bt or {}).get("provider"),
                "request_id": (bt or {}).get("request_id"),
            },
            "seller": {
                "agent_id": seller,
                "model": (agents.get(seller) or {}).get("model"),
                "role": (agents.get(seller) or {}).get("role"),
                "private_value": (agents.get(seller) or {}).get("private_value"),
                "inference": (agents.get(seller) or {}).get("inference"),
                "canonical_request": (st or {}).get("canonical_request"),
                "raw_provider_response": (st or {}).get("raw_provider_response"),
                "provider": (st or {}).get("provider"),
                "request_id": (st or {}).get("request_id"),
            },
        })
    return out


def validate_cell(run_dir: Path, index: int) -> dict:
    files = load_cell_files(run_dir, index)
    ev, obs = files["evidence"], files["observation"]
    errors = []
    if not ev:
        return {**files, "ok": False, "errors": ["missing or unreadable cell_evidence"], "claim_boundary": CLAIM_BOUNDARY}
    if str(ev.get("status") or "") != "completed":
        errors.append(f"evidence status is {ev.get('status')!r}, not completed")
    if not obs:
        return {
            **files, "ok": False, "errors": errors + ["observation unreadable even after on-read repair"],
            "claim_boundary": CLAIM_BOUNDARY,
        }
    agents = agents_from_observation(obs, ev)
    buyers = [a for a in agents.values() if a.get("role") == "buyer"]
    sellers = [a for a in agents.values() if a.get("role") == "seller"]
    trades = [t for t in (obs.get("trades") or []) if isinstance(t, dict)]
    bids = [b for b in (obs.get("bids") or []) if isinstance(b, dict)]
    accounts = {
        aid: {"cash": a["cash"], "inventory": dict(a["inventory"])}
        for aid, a in agents.items()
    }
    cda = IndependentCDA(accounts)
    rejected = []
    for bid in bids:
        r = cda.submit(bid)
        if not r.get("accepted"):
            rejected.append({"bid": bid, "reason": r.get("reason")})
    replayed = cda.trades
    def _key(t):
        return (t.get("buyer"), t.get("seller"), int(t.get("quantity") or 0), round(float(t.get("price") or 0), 10))
    archived_keys = [_key(t) for t in trades]
    replay_keys = [_key(t) for t in replayed]
    matching_ok = archived_keys == replay_keys
    cash0 = sum(a["cash"] for a in accounts.values())
    inv0 = sum(sum(v for v in a["inventory"].values()) for a in accounts.values())
    cash1, inv1 = cda.totals()
    conservation = {
        "initial_cash": cash0,
        "final_cash": cash1,
        "cash_conserved": abs(cash0 - cash1) < 1e-6,
        "initial_inventory": inv0,
        "final_inventory": inv1,
        "inventory_conserved": inv0 == inv1,
    }
    surplus = max_and_realized_surplus(agents, trades)
    reported = ev.get("efficiency")
    eff_ok = False
    if surplus["allocative_efficiency"] is not None and reported is not None:
        try:
            eff_ok = abs(float(reported) - float(surplus["allocative_efficiency"])) < 1e-4
        except (TypeError, ValueError):
            eff_ok = False
    llm = trade_llm_sides(agents, trades)
    kinds = sorted({inference_kind(t) for t in (obs.get("agent_turns") or []) if isinstance(t, dict)})
    live = "ollama" in kinds and "mock" not in kinds
    mock = "mock" in kinds
    if not matching_ok:
        errors.append("independent CDA replay trades differ from archived trades")
    if not conservation["cash_conserved"] or not conservation["inventory_conserved"]:
        errors.append("cash or inventory not conserved on replay")
    if surplus["trade_party_errors"]:
        errors.append("trade parties missing buyer/seller private values or roles")
    ok = not errors and files["observation_meta"].get("ok")
    return {
        "ok": ok,
        "index": index,
        "seed": ev.get("seed"),
        "repeat": ev.get("repeat"),
        "design": ev.get("design"),
        "n_agents": len(agents),
        "n_buyers": len(buyers),
        "n_sellers": len(sellers),
        "five_plus_five": len(buyers) == 5 and len(sellers) == 5,
        "n_bids": len(bids),
        "n_trades": len(trades),
        "inference_kinds": kinds,
        "inference_label": "ollama" if live else ("mixed" if live and mock else ("mock" if mock else "unknown")),
        "matching_ok": matching_ok,
        "rejected_replay_bids": rejected,
        "conservation": conservation,
        "surplus": surplus,
        "reported_efficiency": reported,
        "efficiency_matches_recompute": eff_ok,
        "llm_sides": {k: llm[k] for k in ("n_trades", "both_sides_ollama", "one_side_ollama", "neither_ollama", "mock_involved")},
        "both_sides_ollama_all_trades": llm["n_trades"] > 0 and llm["both_sides_ollama"] == llm["n_trades"],
        "observation_repaired_on_read": bool(files["observation_meta"].get("repaired_on_read")),
        "snapshot_extra_data_ignored": bool(files["snapshot_meta"].get("extra_data_ignored")),
        "wrote_archive": False,
        "errors": errors,
        "agents": [
            {k: a.get(k) for k in ("agent_id", "model", "provider", "role", "private_value", "inference")}
            for a in agents.values()
        ],
        "transcript": trade_transcript(agents, obs, trades),
        "evidence_sha256": files["evidence_sha256"],
        "observation_sha256": files["observation_sha256"],
        "claim_boundary": CLAIM_BOUNDARY,
    }


def matched_comparison(run_dir: Path) -> dict:
    run_dir = Path(run_dir)
    ev_dir = run_dir / "cell_evidence"
    rows = []
    for path in sorted(ev_dir.glob("cell_*.json")) if ev_dir.is_dir() else []:
        try:
            doc, _ = recovery.decode_json_document(path.read_bytes())
        except json.JSONDecodeError:
            continue
        if not isinstance(doc, dict) or doc.get("status") != "completed":
            continue
        try:
            eta = float(doc.get("efficiency"))
        except (TypeError, ValueError):
            continue
        if not math.isfinite(eta):
            continue
        rows.append({
            "index": doc.get("index"),
            "seed": doc.get("seed"),
            "repeat": doc.get("repeat"),
            "design": doc.get("design"),
            "efficiency": eta,
            "trades": doc.get("trades"),
        })
    by_pair = defaultdict(dict)
    for r in rows:
        by_pair[(r["seed"], r["repeat"])][r["design"]] = r
    pairs = []
    for (seed, repeat), designs in sorted(by_pair.items()):
        frozen = designs.get("frozen_snapshot")
        seq = designs.get("sequential_interaction")
        if not frozen or not seq:
            continue
        pairs.append({
            "seed": seed,
            "repeat": repeat,
            "frozen_index": frozen["index"],
            "sequential_index": seq["index"],
            "frozen_eta": frozen["efficiency"],
            "sequential_eta": seq["efficiency"],
            "paired_difference_seq_minus_frozen": seq["efficiency"] - frozen["efficiency"],
            "frozen_trades": frozen["trades"],
            "sequential_trades": seq["trades"],
        })
    diffs = [p["paired_difference_seq_minus_frozen"] for p in pairs]
    n = len(pairs)
    mean_d = sum(diffs) / n if n else None
    n_seq_higher = sum(1 for d in diffs if d > 0)
    n_frozen_higher = sum(1 for d in diffs if d < 0)
    n_tie = sum(1 for d in diffs if d == 0)
    inference = infer_matched_eta(pairs)
    return {
        "n_completed_evidence_cells": len(rows),
        "n_paired_cells": n,
        "mean_frozen_eta": (sum(p["frozen_eta"] for p in pairs) / n if n else None),
        "mean_sequential_eta": (sum(p["sequential_eta"] for p in pairs) / n if n else None),
        "mean_paired_difference_seq_minus_frozen": mean_d,
        "n_sequential_higher": n_seq_higher,
        "n_frozen_higher": n_frozen_higher,
        "n_tie": n_tie,
        "pairs": pairs,
        "inference": inference,
        "counterbalancing": "Same seed and repeat; designs frozen_snapshot vs sequential_interaction; seat rotation by seed in engine.",
        "claim_boundary": CLAIM_BOUNDARY,
        "scientific_gate": "not_passed",
        "scientific_gate_reason": inference["claim_boundary"],
    }


def validate_run(run_dir: Path, *, featured_index: int | None = None) -> dict:
    run_dir = Path(run_dir)
    ev_dir = run_dir / "cell_evidence"
    indices = []
    for path in sorted(ev_dir.glob("cell_*.json")) if ev_dir.is_dir() else []:
        try:
            doc, _ = recovery.decode_json_document(path.read_bytes())
        except json.JSONDecodeError:
            continue
        if isinstance(doc, dict) and doc.get("status") == "completed" and isinstance(doc.get("index"), int):
            indices.append(doc["index"])
    cells = []
    for i in indices:
        cells.append(validate_cell(run_dir, i))
    readable = [c for c in cells if c.get("n_agents") is not None and "observation unreadable" not in " ".join(c.get("errors") or [])]
    ollama_cells = [c for c in readable if c.get("inference_label") == "ollama"]
    five = [c for c in readable if c.get("five_plus_five")]
    both = [c for c in ollama_cells if c.get("both_sides_ollama_all_trades")]
    featured = None
    if featured_index is not None:
        featured = next((c for c in cells if c.get("index") == featured_index), None)
    if featured is None:
        candidates = [c for c in both if c.get("five_plus_five")]
        featured = max(candidates, key=lambda c: (c.get("reported_efficiency") or -1, -(c.get("index") or 0)), default=None)
    return {
        "run_dir": str(run_dir),
        "original_run": recovery.is_original_interrupted_run(run_dir),
        "n_completed_evidence": len(indices),
        "n_cells_validated": len(cells),
        "n_observation_readable": len(readable),
        "n_observation_repaired_on_read": sum(1 for c in cells if c.get("observation_repaired_on_read")),
        "n_five_plus_five": len(five),
        "n_ollama_cells": len(ollama_cells),
        "n_both_sides_ollama_all_trades": len(both),
        "n_matching_ok": sum(1 for c in readable if c.get("matching_ok")),
        "n_conservation_ok": sum(1 for c in readable if c.get("conservation", {}).get("cash_conserved") and c.get("conservation", {}).get("inventory_conserved")),
        "n_efficiency_match": sum(1 for c in readable if c.get("efficiency_matches_recompute")),
        "n_cells_with_extra_units": sum(1 for c in readable if (c.get("surplus") or {}).get("extra_units_excluded")),
        "wrote_archive": False,
        "featured_cell": featured,
        "matched_comparison": matched_comparison(run_dir),
        "cells": [{k: c.get(k) for k in (
            "ok", "index", "seed", "repeat", "design", "n_buyers", "n_sellers",
            "five_plus_five", "n_trades", "inference_label", "matching_ok",
            "efficiency_matches_recompute", "both_sides_ollama_all_trades",
            "observation_repaired_on_read", "reported_efficiency", "errors",
            "llm_sides", "conservation", "surplus",
        )} for c in cells],
        "claim_boundary": CLAIM_BOUNDARY,
        "engineering_gate": "passed" if (
            len(five) == len(indices) and len(indices) > 0
            and len(both) == len(ollama_cells) == len(readable) == len(indices)
            and all(c.get("matching_ok") and c.get("efficiency_matches_recompute") for c in readable)
            and all((c.get("conservation") or {}).get("cash_conserved") and (c.get("conservation") or {}).get("inventory_conserved") for c in readable)
        ) else "not_passed",
        "scientific_gate": "not_passed",
        "scientific_gate_reason": (
            "Wilcoxon on these 60 pairs is post-hoc because descriptive means were "
            "inspected before the analysis plan was locked. Native JSON for cells "
            "84–119 is a derived corpus; original archive bytes are preserved."
        ),
    }
