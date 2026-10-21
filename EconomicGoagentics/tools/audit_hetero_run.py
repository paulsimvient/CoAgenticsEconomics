#!/usr/bin/env python3
"""Read-only audit of heterogeneous campaign evidence; never claims scientific validation."""
import argparse, hashlib, json, pathlib, sys

def audit(folder, strict_lifecycle=False):
    root=pathlib.Path(folder)
    manifest_file=root/"run_manifest.json"
    if not manifest_file.is_file():
        return {"ok":False,"errors":["missing run_manifest.json"]}
    try:
        raw=manifest_file.read_bytes()
        manifest=json.loads(raw)
        total=manifest["cells_planned"]
        if not isinstance(total,int) or total<0 or total>1000000: raise ValueError("invalid cells_planned")
        errors=[]
        events=[]
        progress=root/"hetero_progress.jsonl"
        if progress.exists():
            for n,line in enumerate(progress.read_text().splitlines(),1):
                if not line.strip(): continue
                try: events.append(json.loads(line))
                except json.JSONDecodeError: errors.append(f"malformed progress event line {n}")
        # Validate the planned campaign geometry, not only the count.
        seeds=manifest.get("seeds")
        repeats=manifest.get("repeats")
        if not isinstance(seeds,int) or seeds<1 or not isinstance(repeats,int) or repeats<1:
            raise ValueError("invalid seeds/repeats in manifest")
        if total % (seeds*repeats):
            raise ValueError("cells_planned inconsistent with seeds and repeats")
        designs_per_seed=total//(seeds*repeats)
        if designs_per_seed not in (1,2):
            raise ValueError("unsupported experimental design count")
        # The runner writes frozen_snapshot first, then sequential_interaction.
        # Older manifests do not encode the selected design flags, so verify
        # the index/design relationship only when both designs are present.
        expected_designs=("frozen_snapshot", "sequential_interaction")
        declared_designs=manifest.get("designs")
        if declared_designs is not None:
            if (not isinstance(declared_designs,list) or
                declared_designs not in (["frozen_snapshot"],["sequential_interaction"],
                                          ["frozen_snapshot","sequential_interaction"])):
                raise ValueError("invalid manifest designs")
            if len(declared_designs)!=designs_per_seed:
                raise ValueError("manifest designs disagree with campaign geometry")
            expected_designs=tuple(declared_designs)
        latest={}
        event_counts={}
        terminal_counts={}
        running_counts={}

        for event in events:
            i=event.get("index")
            if not isinstance(i,int) or not 0<=i<total:
                errors.append(f"invalid event index: {i!r}");continue
            status=event.get("status")
            if status not in ("running","completed","error"):
                errors.append(f"cell {i}: unrecognized progress status {status!r}")
                continue
            event_counts[i]=event_counts.get(i,0)+1
            if status=="running":
                running_counts[i]=running_counts.get(i,0)+1
                if strict_lifecycle and running_counts[i]>1:
                    errors.append(f"cell {i}: duplicate running events")
            elif strict_lifecycle and running_counts.get(i,0)==0:
                errors.append(f"cell {i}: terminal event without preceding running event")
            if status in ("completed","error"):
                terminal_counts[i]=terminal_counts.get(i,0)+1
                if terminal_counts[i]>1:
                    errors.append(f"cell {i}: duplicate terminal progress events")
            if i in latest and latest[i].get("status") in ("completed","error"):
                errors.append(f"cell {i}: progress event occurs after terminal status")
            latest[i]=event
        finished={}
        evidence_digests={}

        evidence=root/"cell_evidence"
        for path in sorted(evidence.glob("cell_*.json")) if evidence.exists() else []:
            try:
                cell_bytes=path.read_bytes()
                cell=json.loads(cell_bytes)
                i=cell["index"]
                if path.name!=f"cell_{i}.json" or not isinstance(i,int) or not 0<=i<total:
                    raise ValueError("filename/index mismatch")
                if cell.get("status") not in ("completed","error"): raise ValueError("invalid terminal status")
                if i in finished: raise ValueError("duplicate cell index")
                # The run plan is ordered seed, design, repeat.
                expected_seed=manifest.get("base_seed",0)+i//(repeats*designs_per_seed)
                expected_repeat=i%repeats
                if cell.get("seed")!=expected_seed or cell.get("repeat")!=expected_repeat:
                    errors.append(f"cell {i}: seed/repeat disagrees with campaign plan")
                if declared_designs is not None or designs_per_seed==2:
                    expected_design=expected_designs[(i//repeats)%designs_per_seed]
                    if cell.get("design")!=expected_design:
                        errors.append(f"cell {i}: design disagrees with campaign plan ({expected_design})")
                finished[i]=cell
                evidence_digests[path.name]=hashlib.sha256(cell_bytes).hexdigest()
                event=latest.get(i)
                if not event or event.get("status")!=cell["status"]:
                    errors.append(f"cell {i}: missing/mismatched terminal progress event")
                for key in ("seed","repeat","design"):
                    if event and event.get(key)!=cell.get(key):
                        errors.append(f"cell {i}: {key} differs from terminal event")
            except (OSError,ValueError,KeyError,TypeError) as exc:
                errors.append(f"{path.name}: {exc}")
        for i,event in latest.items():
            if event.get("status")=="running" and i in finished:
                errors.append(f"cell {i}: evidence exists but latest event is running")
            if event.get("status") in ("completed","error") and i not in finished:
                errors.append(f"cell {i}: terminal event lacks evidence")
        # v20: cross-check Wave 3 population summaries against cell evidence.
        # The serializer stores aggregates/fingerprints, not complete decisions.
        snapshot_hashes={}
        snapshot_dir=root/"cell_snapshots"
        for path in sorted(snapshot_dir.glob("cell_*.json")) if snapshot_dir.exists() else []:
            try:
                name=path.name
                i=int(name.removeprefix("cell_").removesuffix(".json"))
                if name!=f"cell_{i}.json" or not 0<=i<total:
                    raise ValueError("invalid snapshot index or filename")
                raw_snapshot=path.read_bytes()
                snapshot=json.loads(raw_snapshot)
                if not isinstance(snapshot,dict):
                    raise ValueError("snapshot is not a JSON object")
                for key in ("agents","trades","efficiency","fingerprints"):
                    if key not in snapshot: raise ValueError(f"missing {key}")
                if (not isinstance(snapshot["agents"],int) or snapshot["agents"]<0
                    or not isinstance(snapshot["trades"],int) or snapshot["trades"]<0
                    or not isinstance(snapshot["fingerprints"],list)):
                    raise ValueError("invalid population summary shape")
                cell=finished.get(i)
                if not cell or cell.get("status")!="completed":
                    errors.append(f"snapshot {i}: no matching completed cell evidence")
                else:
                    if snapshot["agents"]!=cell.get("agents"):
                        errors.append(f"snapshot {i}: agent count mismatch")
                    if snapshot["trades"]!=cell.get("trades"):
                        errors.append(f"snapshot {i}: trade count mismatch")
                    # C++ population_run_json rounds efficiency to four decimals.
                    efficiency=snapshot["efficiency"]
                    if (not isinstance(efficiency,(int,float)) or isinstance(efficiency,bool)
                        or abs(efficiency-cell.get("efficiency",float("inf")))>0.000051):
                        errors.append(f"snapshot {i}: efficiency mismatch")
                snapshot_hashes[name]=hashlib.sha256(raw_snapshot).hexdigest()
            except (OSError,ValueError,TypeError,KeyError,OverflowError) as exc:
                errors.append(f"{path.name}: invalid snapshot: {exc}")
        # v19 and later runs write one snapshot for each completed cell.
        # Legacy runs without the directory are not subject to this requirement.
        if snapshot_dir.exists():
            for i,cell in finished.items():
                if cell["status"]=="completed" and f"cell_{i}.json" not in snapshot_hashes:
                    errors.append(f"cell {i}: completed but population snapshot missing")
        # v23: validate the observation archive introduced in v22.
        observation_hashes={}
        observation_dir=root/"cell_observations"
        for path in sorted(observation_dir.glob("cell_*.json")) if observation_dir.exists() else []:
            try:
                name=path.name
                i=int(name.removeprefix("cell_").removesuffix(".json"))
                if name!=f"cell_{i}.json" or not 0<=i<total:
                    raise ValueError("invalid observation index or filename")
                raw_observation=path.read_bytes()
                observation=json.loads(raw_observation)
                if not isinstance(observation,dict) or observation.get("schema_version")!=1:
                    raise ValueError("unsupported observation schema")
                cell=finished.get(i)
                if cell is None or cell.get("status")!="completed":
                    errors.append(f"observation {i}: no matching completed cell")
                else:
                    for key in ("index","seed","repeat","design"):
                        if observation.get(key)!=cell.get(key) if key!="index" else observation.get(key)!=i:
                            errors.append(f"observation {i}: {key} mismatch")
                for key in ("trades","bids","agent_turns"):
                    if not isinstance(observation.get(key),list):
                        raise ValueError(f"{key} must be an array")
                if cell is not None and cell.get("status")=="completed":
                    if len(observation["trades"])!=cell.get("trades"):
                        errors.append(f"observation {i}: trade count mismatch")
                for j,trade in enumerate(observation["trades"]):
                    if not isinstance(trade,dict) or not all(k in trade for k in ("time","asset","quantity","price","buyer","seller")):
                        raise ValueError(f"invalid trade at position {j}")
                for j,bid in enumerate(observation["bids"]):
                    if not isinstance(bid,dict) or not all(k in bid for k in ("time","agent_id","asset","quantity","price","side")) or bid["side"] not in ("buy","sell"):
                        raise ValueError(f"invalid bid at position {j}")
                for j,turn in enumerate(observation["agent_turns"]):
                    if not isinstance(turn,dict) or not all(k in turn for k in ("agent_id","provider","model","version","request_id","time","seed","canonical_request","raw_provider_response","latency_ms","prompt_tokens","completion_tokens")):
                        raise ValueError(f"invalid model turn at position {j}")
                observation_hashes[name]=hashlib.sha256(raw_observation).hexdigest()
            except (OSError,ValueError,TypeError,KeyError,OverflowError) as exc:
                errors.append(f"{path.name}: invalid observation archive: {exc}")
        if observation_dir.exists():
            for i,cell in finished.items():
                if cell["status"]=="completed" and f"cell_{i}.json" not in observation_hashes:
                    errors.append(f"cell {i}: completed but observation archive missing")
        # Explicit recovery inventory: a complete execution summary is not a
        # recoverable scientific checkpoint, even if its audit checks pass.
        unfinished_indices=[i for i in range(total) if i not in finished]
        errored_indices=sorted(i for i,c in finished.items() if c["status"]=="error")
        return {"ok":not errors,"manifest_sha256":hashlib.sha256(raw).hexdigest(),
                "progress_sha256":hashlib.sha256(progress.read_bytes()).hexdigest() if progress.exists() else None,
                "evidence_sha256":dict(sorted(evidence_digests.items())),
                "snapshot_sha256":dict(sorted(snapshot_hashes.items())),
                "snapshot_count":len(snapshot_hashes),
                "observation_sha256":dict(sorted(observation_hashes.items())),
                "observation_count":len(observation_hashes),
                "planned":total,"terminal_evidence":len(finished),
                "completed_execution":sum(c["status"]=="completed" for c in finished.values()),
                "execution_errors":sum(c["status"]=="error" for c in finished.values()),
                "unfinished":total-len(finished),
                "unfinished_cell_indices":unfinished_indices,
                "execution_error_cell_indices":errored_indices,
                "errors":errors,
                "duplicate_terminal_cell_indices":sorted(i for i,n in terminal_counts.items() if n>1),
                "duplicate_running_cell_indices":sorted(i for i,n in running_counts.items() if n>1),
                "terminal_without_start_indices":sorted(i for i in terminal_counts if running_counts.get(i,0)==0),
                "scientific_validation":"NOT ASSESSED",
                "resume_safe":False,
                "audit_scope":"execution evidence consistency only; no full-state recovery"}
    except (OSError,ValueError,KeyError,TypeError) as exc:
        return {"ok":False,"errors":[f"invalid manifest: {exc}"]}

if __name__=="__main__":
    p=argparse.ArgumentParser()
    p.add_argument("run_directory")
    p.add_argument("--report",help="Write a JSON audit report (will not overwrite existing file)")
    p.add_argument("--strict-lifecycle",action="store_true",help="Require running event before terminal event and reject duplicate starts")
    p.add_argument("--baseline",help="Compare current evidence hashes against an earlier audit JSON report")
    a=p.parse_args()
    result=audit(a.run_directory, strict_lifecycle=a.strict_lifecycle)
    if a.baseline:
        try:
            baseline=json.loads(pathlib.Path(a.baseline).read_text(encoding="utf-8"))
            changed=[k for k in ("manifest_sha256","progress_sha256") if baseline.get(k)!=result.get(k)]
            old=baseline.get("evidence_sha256")
            new=result.get("evidence_sha256")
            if not isinstance(old,dict) or not isinstance(new,dict):
                raise ValueError("missing evidence digests")
            added=sorted(set(new)-set(old))
            removed=sorted(set(old)-set(new))
            modified=sorted(k for k in set(old)&set(new) if old[k]!=new[k])
            result["baseline_comparison"]={"changed_top_level":changed,
                "added_cell_evidence":added,"removed_cell_evidence":removed,
                "modified_cell_evidence":modified}
            # Snapshot hashes were introduced after evidence baseline comparisons.
            # Missing snapshot digests in an older baseline are reported as
            # unavailable, never silently interpreted as a clean comparison.
            old_snap=baseline.get("snapshot_sha256")
            new_snap=result.get("snapshot_sha256")
            if isinstance(old_snap,dict) and isinstance(new_snap,dict):
                snap_added=sorted(set(new_snap)-set(old_snap))
                snap_removed=sorted(set(old_snap)-set(new_snap))
                snap_modified=sorted(k for k in set(old_snap)&set(new_snap) if old_snap[k]!=new_snap[k])
                result["baseline_comparison"].update({
                    "snapshot_comparison_available":True,
                    "added_snapshots":snap_added,
                    "removed_snapshots":snap_removed,
                    "modified_snapshots":snap_modified})
                if snap_removed or snap_modified:
                    result["ok"]=False
                    result.setdefault("errors",[]).append("baseline population snapshots changed or disappeared")
            else:
                result["baseline_comparison"]["snapshot_comparison_available"]=False
            old_obs=baseline.get("observation_sha256")
            new_obs=result.get("observation_sha256")
            if isinstance(old_obs,dict) and isinstance(new_obs,dict):
                obs_added=sorted(set(new_obs)-set(old_obs))
                obs_removed=sorted(set(old_obs)-set(new_obs))
                obs_modified=sorted(k for k in set(old_obs)&set(new_obs) if old_obs[k]!=new_obs[k])
                result["baseline_comparison"].update({
                    "observation_comparison_available":True,
                    "added_observations":obs_added,
                    "removed_observations":obs_removed,
                    "modified_observations":obs_modified})
                if obs_removed or obs_modified:
                    result["ok"]=False
                    result.setdefault("errors",[]).append("baseline observation archives changed or disappeared")
            else:
                result["baseline_comparison"]["observation_comparison_available"]=False
            if "manifest_sha256" in changed or removed or modified:
                result["ok"]=False
                result.setdefault("errors",[]).append("baseline evidence changed or disappeared")
        except (OSError,ValueError,TypeError) as exc:
            result["ok"]=False
            result.setdefault("errors",[]).append(f"baseline comparison failed: {exc}")
    rendered=json.dumps(result,indent=2,sort_keys=True)+"\n"
    print(rendered,end="")
    if a.report:
        report=pathlib.Path(a.report)
        try:
            # Exclusive creation prevents accidental replacement of prior audit evidence.
            with report.open("x",encoding="utf-8") as handle:
                handle.write(rendered)
        except OSError as exc:
            print(f"Could not write audit report: {exc}",file=sys.stderr)
            sys.exit(2)
    sys.exit(0 if result["ok"] else 1)
