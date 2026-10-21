#!/usr/bin/env python3
"""Localhost workbench controller: v27 paced CDA + DV026 evidence runner + batch validation."""
import hashlib, json, os, re, subprocess, threading, time, uuid, urllib.parse, urllib.request, urllib.error
from http.server import ThreadingHTTPServer, BaseHTTPRequestHandler
from pathlib import Path
from enterprise_api import campaign_recovery as recovery
ROOT=Path(__file__).resolve().parent
CAMPAIGN_DIR=ROOT/'results'/'dv026_ollama_campaign'
HETERO_ROOT=ROOT/'results'/'dv026_heterogeneous_campaign'
IMPLEMENTATION_REVISION='2026-10-04-pass3-step17-ux-refinement'

# ===== V17 Ollama research communication layer =====
OLLAMA_URL=os.environ.get('OLLAMA_URL','http://127.0.0.1:11434').rstrip('/')
OLLAMA_MODEL=os.environ.get('OLLAMA_MODEL','llama3.2')
OLLAMA_TIMEOUT=float(os.environ.get('OLLAMA_TIMEOUT','60'))

def _ollama_status():
    try:
        req=urllib.request.Request(OLLAMA_URL+'/api/tags',headers={'Accept':'application/json'})
        with urllib.request.urlopen(req,timeout=4) as r:
            data=json.loads(r.read().decode('utf-8'))
        models=[m.get('name') for m in data.get('models',[]) if isinstance(m,dict)]
        return {'reachable':True,'url':OLLAMA_URL,'configured_model':OLLAMA_MODEL,'models':models}
    except Exception as e:
        return {'reachable':False,'url':OLLAMA_URL,'configured_model':OLLAMA_MODEL,'models':[],'error':str(e)}

def _research_context():
    # Only expose compact, authoritative application state to the LLM.
    try: campaign=S.campaign_status()
    except Exception: campaign={}
    try: live=S.snapshot(0)
    except Exception: live={}
    experiments=_ent_read('experiment')[-20:]
    versions=_ent_read('experiment_version')[-20:]
    models=_ent_read('model')[-20:]
    prompts=_ent_read('prompt')[-20:]
    datasets=_ent_read('dataset')[-20:]
    evidence=_ent_read('evidence_package')[-20:]
    reviews=_ent_read('review')[-20:]
    claims=_ent_read('claim')[-20:]
    return {
      'experiments':experiments,'versions':versions,'models':models,'prompts':prompts,
      'datasets':datasets,'evidence':evidence,'reviews':reviews,'claims':claims,
      'campaign_status':campaign,
      'live_state':{k:live.get(k) for k in ('status','running','complete','events','trace') if k in live},
      'note':'Research objects and DV026 execution state are authoritative. Missing values mean missing data.'
    }

def _ollama_chat(user_message):
    status=_ollama_status()
    if not status.get('reachable'):
        return {'ok':False,'error':'Ollama is not reachable. Start Ollama and retry.','status':status}
    context=_research_context()
    system="""You are the CoAgentics Research Communication Layer. You are a communication and navigation mechanism over an authoritative experimental system, not the source of scientific truth. Use only the supplied application context. Never invent observations, statistics, experiment results, object relationships, or evidence. Clearly distinguish observed data, enterprise metadata, interpretation, and claim. If the context is insufficient, say that it is insufficient. When useful, tell the user which object or UI area to inspect. Explain the next governed action based on the experiment lifecycle when possible. Do not execute mutations. Do not approve claims. Do not infer hidden beliefs or psychological traits from observable behavior. Be concise and practical."""
    payload={'model':OLLAMA_MODEL,'stream':False,'messages':[
      {'role':'system','content':system},
      {'role':'user','content':'AUTHORITATIVE APPLICATION CONTEXT:\n'+json.dumps(context,separators=(',',':'))+'\n\nUSER QUESTION:\n'+str(user_message)}
    ],'options':{'temperature':0.1}}
    try:
        req=urllib.request.Request(OLLAMA_URL+'/api/chat',data=json.dumps(payload).encode('utf-8'),headers={'Content-Type':'application/json','Accept':'application/json'},method='POST')
        with urllib.request.urlopen(req,timeout=OLLAMA_TIMEOUT) as r:
            data=json.loads(r.read().decode('utf-8'))
        answer=((data.get('message') or {}).get('content') or '').strip()
        if not answer: raise ValueError('Ollama returned an empty response')
        return {'ok':True,'answer':answer,'model':data.get('model',OLLAMA_MODEL),'status':status}
    except Exception as e:
        return {'ok':False,'error':str(e),'model':OLLAMA_MODEL,'status':status}

# ===== Enterprise V13 persistent research objects =====
ENTERPRISE_STORE = ROOT/'data'/'enterprise'
ENTERPRISE_STORE.mkdir(parents=True, exist_ok=True)
ENTERPRISE_KINDS = {
 'workspaces':'workspace','experiments':'experiment','versions':'experiment_version',
 'models':'model','prompts':'prompt','datasets':'dataset','evidence':'evidence_package',
 'reviews':'review','claims':'claim','audit-events':'audit_event'
}
def _ent_path(kind): return ENTERPRISE_STORE/(kind+'.jsonl')
def _ent_read(kind):
 p=_ent_path(kind)
 if not p.is_file(): return []
 rows=[]
 for line in p.read_text(encoding='utf-8').splitlines():
  if line.strip():
   try: rows.append(json.loads(line))
   except Exception: pass
 return rows
def _ent_write(kind,rows):
 _ent_path(kind).write_text(''.join(json.dumps(x,separators=(',',':'))+'\n' for x in rows),encoding='utf-8')
def _ent_upsert(kind,obj):
 import uuid
 rows=_ent_read(kind); now=time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime())
 if not obj.get('id'): obj['id']=kind[:3]+'_'+uuid.uuid4().hex[:12]; obj['created_at']=now
 obj['updated_at']=now
 found=False
 for i,r in enumerate(rows):
  if r.get('id')==obj['id']: rows[i]=obj; found=True; break
 if not found: rows.append(obj)
 _ent_write(kind,rows); return obj
def _ent_get(kind,oid): return next((x for x in _ent_read(kind) if x.get('id')==oid),None)
def _ent_delete(kind,oid): _ent_write(kind,[x for x in _ent_read(kind) if x.get('id')!=oid]); return True
def _ent_seed():
 if not _ent_read('workspace'): _ent_upsert('workspace',{'id':'ws_dv026','name':'DV026 Economic Behavior','description':'Controlled experimental research on AI-agent economic behavior.','status':'active','environment':'development'})
 if not _ent_read('experiment'): _ent_upsert('experiment',{'id':'exp_dv026_026','workspace_id':'ws_dv026','name':'Peer Visibility','question':'Does peer visibility alter AI-agent economic behavior?','status':'draft','current_version':'0.1'})
_ent_seed()

EXPECTATIONS_DIR=ROOT/'results'/'dv026_research_expectations'
EXPECTATIONS_PATH=EXPECTATIONS_DIR/'research_expectations.json'
DESIGN_DIR=ROOT/'results'/'dv026_research_design'
DESIGN_PATH=DESIGN_DIR/'experiment_design.json'
LIFECYCLE_PATH=ROOT/'workbench'/'experiment_lifecycle.json'
LIFECYCLE_SCHEMA='coagentics-experiment-lifecycle-v1'

# Read-only formula provenance for Research Console Method drawers (no editable math).
CHART_METHODS={
 'models_x_seeds':{
  'title':'Models × seeds',
  'formula':'Per model: cell_ok count and attempted count from cells.jsonl; bar scale = count / n_seeds.',
  'math':'ok_m / N , attempted_m / N',
  'source':'results/dv026_ollama_campaign/cells.jsonl',
  'fields':['model','attempted','cell_ok','n_seeds'],
 },
 'literature_vs_llm':{
  'title':'η · literature vs LLM',
  'formula':'Literature band = archival Table 2 means. LLM point = mean treatment_efficiency over rows with cell_ok (never Layer A CDA).',
  'math':'live_mean = mean(treatment_efficiency | cell_ok)',
  'source':'docs/GODE_SUNDER_1993_TABLE2.json + cells.jsonl',
  'fields':['treatment_efficiency','cell_ok','literature_means'],
 },
 'pipeline':{
  'title':'Pipeline',
  'formula':'Each stage counts matching boolean flags among non-skipped cells; % = count / attempted.',
  'math':'stage_count / attempted',
  'source':'results/dv026_ollama_campaign/cells.jsonl',
  'fields':['attempted','parse_ok','action_valid','market_action_ok','market_accepted','replay_ok|cross_event_valid','cell_ok'],
 },
}
ANALYSIS_METHODS={
 'campaign_analysis':{
  'title':'Campaign analysis · model × version',
  'formula':'Per model, mean and sample sd of treatment_efficiency across seed cells; 95% CI = mean ± 1.96·sd/√n (descriptive).',
  'math':'CI = mean ± 1.96·sd/√n',
  'source':'results/dv026_ollama_campaign/cells.jsonl',
  'fields':['treatment_efficiency','parse_ok','action_valid','market_accepted','execution_observed'],
 },
 'matched_effects':{
  'title':'Matched treatment effects',
  'formula':'Mean/CI of persisted paired deltas already written on each cell row (not recomputed in the UI).',
  'math':'mean(delta_*), CI as above',
  'source':'results/dv026_ollama_campaign/cells.jsonl',
  'fields':['delta_efficiency','delta_surplus','delta_mean_price'],
 },
 'fingerprints':{
  'title':'Behavioral fingerprints',
  'formula':'Aggregated only from persisted cell_*.jsonl decision turns (aggressiveness, reservation violations, fills, payoff).',
  'math':'rates over observed turns / quotes',
  'source':'results/dv026_ollama_campaign/cell_*.jsonl',
  'fields':['parsed_action','submission.filled_quantity','agent_state_before.private_value'],
 },
}

def _unfreeze_expectations_to_draft():
 """Keep hypothesis content; unlock for a new experiment (workflow beginning)."""
 doc=_load_expectations()
 doc['status']='draft'
 doc.pop('manifest_sha256',None)
 doc.pop('frozen_at',None)
 doc.pop('frozen_before_observation',None)
 EXPECTATIONS_DIR.mkdir(parents=True,exist_ok=True)
 EXPECTATIONS_PATH.write_text(json.dumps(doc,indent=2,ensure_ascii=False)+'\n')
 return doc

def _canonical_json(obj):
 return json.dumps(obj,sort_keys=True,separators=(',',':'),ensure_ascii=False)

def _expectations_hash(doc):
 payload=dict(doc)
 payload.pop('manifest_sha256',None)
 return hashlib.sha256(_canonical_json(payload).encode('utf-8')).hexdigest()

def _normalize_expectations(doc):
 # Backward-compatible migration for the earlier metric schema.
 # The scientific schema requires the explicit operational_definition field.
 if not isinstance(doc,dict): return doc
 for e in doc.get('expectations',[]) if isinstance(doc.get('expectations'),list) else []:
  m=e.get('metric')
  if isinstance(m,dict) and not str(m.get('operational_definition') or '').strip():
   legacy=m.get('definition')
   if legacy not in (None,''):
    m['operational_definition']=legacy
  # Keep the canonical field only; do not silently preserve a conflicting legacy alias.
  if isinstance(m,dict): m.pop('definition',None)
 return doc

def _empty_expectations(integrity_error=None):
 out={'schema_version':'dv026-research-expectations-v1','status':'draft','expectations':[],
      'sample_size':{'planned_seeds':20,'minimum_complete':18,'stopping_rule':'fixed_number_of_seeds'},
      'integrity_ok':integrity_error is None}
 if integrity_error:
  out['integrity_ok']=False
  out['integrity_error']=str(integrity_error)
 return out

def _load_expectations():
 if not EXPECTATIONS_PATH.is_file():
  return _empty_expectations()
 try:
  d=_normalize_expectations(json.loads(EXPECTATIONS_PATH.read_text()))
  if not isinstance(d,dict):
   return _empty_expectations('research_expectations.json is not a JSON object')
  if d.get('status')=='frozen':
   stored=str(d.get('manifest_sha256') or '')
   actual=_expectations_hash(d)
   if not stored or stored!=actual:
    err=(
     'Frozen research expectations failed SHA-256 integrity verification '
     f'(stored={stored or "(missing)"}; actual={actual}). '
     'Re-save draft fields and Freeze again before a full campaign.'
    )
    blocked=_empty_expectations(err)
    # Keep hypothesis content visible for repair, but do not treat as frozen.
    blocked['expectations']=d.get('expectations') if isinstance(d.get('expectations'),list) else []
    if isinstance(d.get('sample_size'),dict): blocked['sample_size']=d['sample_size']
    blocked['integrity_blocked']=True
    blocked['integrity_stored_sha256']=stored or None
    blocked['integrity_actual_sha256']=actual
    return blocked
  d['integrity_ok']=True
  d.pop('integrity_error',None)
  d.pop('integrity_blocked',None)
  return d
 except Exception as e:
  return _empty_expectations(f'Failed to load research expectations: {e}')

def _validate_expectations(doc):
 if not isinstance(doc,dict): raise ValueError('expectations document must be an object')
 if doc.get('schema_version')!='dv026-research-expectations-v1': raise ValueError('unsupported expectations schema')
 exps=doc.get('expectations')
 if not isinstance(exps,list) or not exps: raise ValueError('at least one hypothesis is required before freezing')
 seen=set()
 for i,e in enumerate(exps,1):
  if not isinstance(e,dict): raise ValueError(f'hypothesis {i} must be an object')
  hid=str(e.get('id') or '').strip()
  if not re.fullmatch(r'H\d{2}',hid): raise ValueError(f'hypothesis {i} requires an ID such as H01')
  if hid in seen: raise ValueError(f'duplicate hypothesis ID {hid}')
  seen.add(hid)
  for key in ('research_question','hypothesis','comparison','metric','unit_of_analysis','estimand','expected','analysis','rationale'):
   if not e.get(key): raise ValueError(f'{hid}: missing required field {key}')
  exp=e['expected']
  if not isinstance(exp,dict) or not exp.get('type'): raise ValueError(f'{hid}: expected result type is required')
  if exp.get('type') in ('directional','numeric','absolute','distribution','equivalence') and not exp.get('direction') and exp.get('type')=='directional':
   raise ValueError(f'{hid}: expected direction is required')
  analysis=e['analysis']
  if not isinstance(analysis,dict) or not analysis.get('method') or not analysis.get('uncertainty'):
   raise ValueError(f'{hid}: analysis method and uncertainty are required')
  metric=e.get('metric')
  if isinstance(metric,dict):
   for mk in ('name','operational_definition','unit','population','aggregation'):
    if not str(metric.get(mk) or '').strip(): raise ValueError(f'{hid}: metric.{mk} is required')
  elif not str(metric or '').strip(): raise ValueError(f'{hid}: metric is required')
  if exp.get('type') in ('numeric','absolute','distribution','equivalence'):
   vals=[exp.get(k) for k in ('value','effect','mean','margin') if exp.get(k) not in (None,'')]
   if not vals: raise ValueError(f'{hid}: numeric expectation requires a numeric value/effect/mean/margin')
   for v in vals:
    try: float(v)
    except (TypeError,ValueError): raise ValueError(f'{hid}: expected numeric values must be numbers')
  lo,hi=exp.get('range_low'),exp.get('range_high')
  if lo not in (None,'') and hi not in (None,''):
   try:
    if float(lo)>float(hi): raise ValueError(f'{hid}: expected range lower bound exceeds upper bound')
   except (TypeError,ValueError): raise ValueError(f'{hid}: expected range bounds must be numeric')
 ss=doc.get('sample_size') or {}
 try: planned=int(ss.get('planned_seeds')); minimum=int(ss.get('minimum_complete'))
 except Exception: raise ValueError('sample_size planned_seeds and minimum_complete are required integers')
 if planned<1 or minimum<1 or minimum>planned: raise ValueError('sample_size requires 1 <= minimum_complete <= planned_seeds')
 if ss.get('stopping_rule')!='fixed_number_of_seeds': raise ValueError('current implementation requires fixed_number_of_seeds stopping rule')
 return True

def _freeze_expectations(doc):
 _validate_expectations(doc)
 out=json.loads(json.dumps(doc))
 out['status']='frozen'
 out['frozen_at']=time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime())
 out['frozen_before_observation']=True
 out['manifest_sha256']=_expectations_hash(out)
 EXPECTATIONS_DIR.mkdir(parents=True,exist_ok=True)
 EXPECTATIONS_PATH.write_text(json.dumps(out,indent=2,ensure_ascii=False)+'\n')
 return out


def _current_campaign_summary(doc):
 return isinstance(doc,dict) and doc.get('implementation_revision')==IMPLEMENTATION_REVISION

def _layer_a_payload_from_doc(doc):
 """Extract Layer A qualification fields from a runner/summary document."""
 if not isinstance(doc,dict): return None
 la=doc.get('layer_a') if isinstance(doc.get('layer_a'),dict) else None
 src=la if isinstance(la,dict) else doc
 pass_flag=bool(src.get('layer_a_pass') or src.get('pass'))
 cda=src.get('mean_efficiency_cda')
 sealed=src.get('mean_efficiency_sealed')
 if not pass_flag and cda is None and sealed is None and not la:
  return None
 if not pass_flag and cda is None:
  return None
 return {
  'layer_a_pass':pass_flag,
  'pass':pass_flag,
  'mean_efficiency_cda':cda,
  'mean_efficiency_sealed':sealed,
  'trials':src.get('trials') or src.get('layer_a_trials') or [],
  'source':('layer_a' if la else 'summary'),
 }

def _model_cell_slug(model:str)->str:
 """Match runner filenames: cell_{model_with_colon_as_underscore}_{seed}.jsonl"""
 return str(model or '').replace(':', '_')

def _parse_cell_filename(name:str):
 """Return (slug, seed) from cell_*.jsonl basename, or None."""
 if not name.startswith('cell_') or not name.endswith('.jsonl'):
  return None
 stem=name[5:-6]
 m=re.match(r'^(.*)_(\d+)$', stem)
 if not m: return None
 return m.group(1), int(m.group(2))

def _iter_jsonl_records(text:str):
 """Yield JSON objects from JSONL, rejoining lines broken by unescaped newlines in strings.

 Older cell writers escaped quotes/backslashes but not \\n inside raw_provider_response,
 so one logical turn became multiple physical lines. Reassemble until json.loads succeeds.
 """
 buf=''
 for line in (text or '').splitlines():
  piece=line.rstrip('\n')
  if not piece.strip() and not buf:
   continue
  # Insert escaped newline — a literal \\n inside the JSON string — not a raw break.
  buf=(buf+'\\n'+piece) if buf else piece
  try:
   obj=json.loads(buf)
  except Exception:
   # Cap runaway buffers (corrupt files) so we don't hold megabytes.
   if len(buf)>2_000_000:
    buf=''
   continue
  if isinstance(obj,dict):
   yield obj
  buf=''

def _safe_campaign_cell_path(model:str, seed:int)->Path:
 """Resolve cell file under CAMPAIGN_DIR only (no path traversal)."""
 slug=_model_cell_slug(model)
 if not slug or '/' in slug or '\\' in slug or '..' in slug:
  raise ValueError('invalid model')
 seed=int(seed)
 path=(CAMPAIGN_DIR/f'cell_{slug}_{seed}.jsonl').resolve()
 root=CAMPAIGN_DIR.resolve()
 if root not in path.parents and path!=root:
  raise ValueError('path escape')
 if not str(path).startswith(str(root)+os.sep):
  raise ValueError('path escape')
 return path

def _latest_hetero_run_dir():
 runs=HETERO_ROOT/'runs'
 if not runs.is_dir():
  return None
 dirs=[p for p in runs.iterdir() if p.is_dir() and p.name.startswith('run_')]
 if not dirs:
  return None
 return max(dirs, key=lambda p:(p.stat().st_mtime, p.name))

def _layer_a_config_fp():
 return recovery.sha256_bytes(b'layer-a-cda-sealed-v1|'+IMPLEMENTATION_REVISION.encode())

def _code_fingerprint():
 parts=[IMPLEMENTATION_REVISION]
 try:
  exe=find_exe('dv026-workbench-runner')
  parts.append(recovery.sha256_file(Path(exe)))
 except Exception:
  parts.append('exe-missing')
 return recovery.sha256_bytes('|'.join(parts).encode())

def _persist_layer_a_if_passed(la:dict):
 if not isinstance(la,dict) or not (la.get('layer_a_pass') or la.get('pass')):
  return
 ev='results/dv026_ollama_campaign/summary.json'
 try:
  recovery.persist_layer_a_gate(ROOT,la,config_fingerprint=_layer_a_config_fp(),code_fingerprint=_code_fingerprint(),evidence_path=ev if (ROOT/ev).is_file() else None)
 except Exception:
  pass

def _write_campaign_session(proc, exe, args, run_dir=None, extra=None):
 sess={
  'session_id':str(uuid.uuid4()),
  'pid':int(proc.pid) if proc is not None else None,
  'exe':str(exe),
  'cmdline':' '.join(str(a) for a in args),
  'command_token':'hetero-campaign',
  'status':'running',
  'started_at':time.time(),
 }
 if extra:
  sess.update(extra)
 recovery.atomic_write_json(HETERO_ROOT/'execution_session.json',sess,overwrite=True)
 dest=run_dir or _latest_hetero_run_dir()
 if dest:
  recovery.write_session(dest,sess)
  recovery.append_audit(dest,{'action':'campaign.session.start','pid':sess.get('pid'),'session_id':sess.get('session_id')})
 return sess

def _finish_campaign_session(status, error=None, run_dir=None):
 dest=run_dir or _latest_hetero_run_dir()
 for folder in [HETERO_ROOT, dest]:
  if not folder:
   continue
  sess=recovery.load_session(folder) or {}
  if not sess and folder is HETERO_ROOT:
   continue
  sess=dict(sess)
  sess['status']=status
  sess['finished_at']=time.time()
  if error:
   sess['last_error']=str(error)
  try:
   recovery.write_session(folder,sess)
  except Exception:
   pass
 if dest:
  try:
   recovery.append_audit(dest,{'action':'campaign.session.finish','status':status,'error':error})
  except Exception:
   pass

def _read_json_file(path:Path):
 if not path or not path.is_file():
  return None
 try:
  d=recovery._decode_json_bytes(path.read_bytes())
  return d if isinstance(d,dict) else None
 except Exception:
  return None

def _hetero_model_label(design, repeat, index=None):
 design=str(design or 'cell')
 short='frozen' if 'frozen' in design else 'sequential'
 label=f'{short} · r{int(repeat or 0)}'
 if index is not None:
  label=f'{label} · c{int(index)}'
 return label

def _extract_json_obj(text):
 if not text:
  return {}
 s=str(text).strip()
 try:
  o=json.loads(s)
  return o if isinstance(o,dict) else {}
 except Exception:
  pass
 m=re.search(r'\{.*\}', s, re.S)
 if not m:
  return {}
 try:
  o=json.loads(m.group(0))
  return o if isinstance(o,dict) else {}
 except Exception:
  return {}

def _hetero_runner_alive():
 """True only if a C++ hetero-campaign process exists. File mtime is not enough (reboot + iCloud clocks)."""
 try:
  r=subprocess.run(['pgrep','-f','dv026-workbench-runner.*hetero-campaign'],capture_output=True,text=True)
  return any(p.isdigit() for p in (r.stdout or '').split())
 except Exception:
  return False

def _hetero_progress(run_dir=None):
 run_dir=run_dir or _lab_run_dir()
 if not run_dir:
  return None
 path=run_dir/'hetero_progress.jsonl'
 completed={}
 running=None
 if path.is_file():
  for line in path.read_text(errors='replace').splitlines():
   line=line.strip()
   if not line:
    continue
   try:
    row=json.loads(line)
   except Exception:
    continue
   if not isinstance(row,dict):
    continue
   st=str(row.get('status') or '')
   idx=row.get('index')
   if st=='completed' and idx is not None:
    completed[int(idx)]=row
   elif st=='running':
    running=row
 live=_hetero_runner_alive()
 if not live:
  running=None
 repeats={int(r.get('repeat') or 0) for r in completed.values()}
 designs={str(r.get('design') or '') for r in completed.values()}
 total=None
 if 2 in repeats and 'sequential_interaction' in designs and 'frozen_snapshot' in designs:
  total=20*3*2
 elif completed:
  total=max(completed)+1
  if running and running.get('index') is not None:
   total=max(total, int(running['index'])+1)
 return {
  'run_dir':run_dir,
  'completed':completed,
  'running':running,
  'done':len(completed),
  'total':total,
  'live':live,
 }

def _hetero_cells_index():
 prog=_hetero_progress()
 if not prog:
  return []
 run_dir=prog['run_dir']
 ev_dir=run_dir/'cell_evidence'
 items=[]
 seen={}
 files=sorted(ev_dir.glob('cell_*.json'), key=lambda p:p.stat().st_mtime) if ev_dir.is_dir() else []
 for path in files:
  ev=_read_json_file(path)
  if not ev:
   continue
  idx=ev.get('index')
  seed=int(ev.get('seed') or 0)
  design=str(ev.get('design') or '')
  repeat=int(ev.get('repeat') or 0)
  key=(_hetero_model_label(design, repeat), seed)
  model=_hetero_model_label(design, repeat, idx if key in seen else None)
  seen[key]=True
  obs=_read_json_file(run_dir/'cell_observations'/path.name)
  turns=int(ev.get('observed_llm_turns') or 0)
  if not turns and isinstance(obs,dict):
   turns=len(obs.get('agent_turns') or [])
  err=str(ev.get('error') or '')
  items.append({
   'model':model,
   'seed':seed,
   'slug':path.stem,
   'file':str(path.relative_to(ROOT)) if str(path).startswith(str(ROOT)) else path.name,
   'turns':turns,
   'cell_ok':(str(ev.get('status') or '')=='completed' and not err),
   'control_efficiency':None,
   'treatment_efficiency':ev.get('efficiency'),
   'delta_efficiency':None,
   'control_surplus':None,
   'treatment_surplus':None,
   'delta_surplus':None,
   'control_mean_price':None,
   'treatment_mean_price':None,
   'delta_mean_price':None,
   'latency_ms':None,
   'units_filled':ev.get('trades'),
   'error':err,
   'indexed':True,
   'kind':'hetero',
   'cell_index':idx,
   'design':design,
   'repeat':repeat,
   'agents':ev.get('agents'),
  })
 items.sort(key=lambda c:(c.get('cell_index') is None, c.get('cell_index') or 0))
 return items

def _overlay_hetero_progress(snap:dict):
 run_dir=_lab_run_dir()
 if not run_dir:
  return snap
 rec=recovery.reconcile(run_dir, persist=False)
 rec=recovery.apply_live_session(rec, recovery.load_session(HETERO_ROOT))
 snap['hetero_run_dir']=str(run_dir)
 snap['command']='hetero-campaign'
 snap['population_design']='heterogeneous_multi_llm'
 snap['execution_state']=rec['execution_state']
 snap['interrupted']=bool(rec['interrupted'])
 snap['remaining']=rec['remaining']
 snap['last_completed_cell']=rec['last_completed_cell']
 snap['recovery_display']=rec.get('display')
 snap['protected_completed']=int(rec['done'] or 0)
 snap['original_run']=bool((rec.get('inventory') or {}).get('original_run'))
 snap['immutable_note']=recovery.PROTECTED_NOTE
 if rec['done']:
  snap['done']=rec['done']
  snap['cells_ok']=rec['done']
  snap['pass_count']=rec['done']
 if rec['total']:
  snap['total']=max(int(snap.get('total') or 0), int(rec['total']))
 live=bool(rec['process_live'])
 inmem=bool(snap.get('running'))
 if inmem and not live:
  snap['running']=False
  inmem=False
 sess=rec.get('session') or {}
 if snap.get('resume_started_at') is None and sess.get('resume_started_at') is not None:
  snap['resume_started_at']=sess.get('resume_started_at')
 if snap.get('resume_baseline_done') is None and sess.get('resume_baseline_done') is not None:
  snap['resume_baseline_done']=sess.get('resume_baseline_done')
 if live:
  snap['running']=True
  snap['complete']=False
  snap['interrupted']=False
  snap['scope']='running'
  done=int(snap.get('done') or 0)
  total=int(snap.get('total') or 0)
  remaining=max(0,total-done) if total else int(rec.get('remaining') or 0)
  snap['remaining']=remaining
  prog=_hetero_progress(run_dir)
  running=prog.get('running') if prog else None
  if running:
   snap['current_cell']=running.get('index')
   snap['current_seed']=running.get('seed')
   snap['current_design']=running.get('design')
   snap['current_model']=_hetero_model_label(running.get('design'), running.get('repeat'), running.get('index'))
  started=snap.get('resume_started_at') or snap.get('started_at') or sess.get('started_at')
  if started:
   elapsed=max(0.0, time.time()-float(started))
   snap['elapsed_sec']=round(elapsed,1)
   baseline=snap.get('resume_baseline_done')
   new_done=None
   if baseline is not None:
    new_done=max(0, done-int(baseline))
    snap['resume_new_done']=new_done
   if new_done and remaining>0:
    snap['eta_sec']=round((elapsed/new_done)*remaining,1)
   else:
    snap['eta_sec']=None
  if total:
   snap['pct']=round(100.0*done/total,2)
 else:
  snap['running']=False
  snap['eta_sec']=None
  snap['elapsed_sec']=None
  if rec['interrupted']:
   snap['complete']=False
   snap['scope']='interrupted'
   snap['recovery_display']=f"Interrupted — {rec['done']}/{rec['total']} cells completed"
  elif rec['complete']:
   snap['complete']=True
   snap['scope']='complete'
 return snap

def _hetero_turns(obs:dict, ev:dict):
 turns=[]
 if not isinstance(obs,dict):
  return turns
 trades=obs.get('trades') if isinstance(obs.get('trades'),list) else []
 bids=obs.get('bids') if isinstance(obs.get('bids'),list) else []
 for i,row in enumerate(obs.get('agent_turns') or []):
  if not isinstance(row,dict):
   continue
  pa=_extract_json_obj(row.get('raw_provider_response'))
  req=row.get('canonical_request')
  req_obj=_extract_json_obj(req) if isinstance(req,str) else (req if isinstance(req,dict) else {})
  aid=row.get('agent_id')
  mine=[t for t in trades if t.get('buyer')==aid or t.get('seller')==aid]
  filled=sum(int(t.get('quantity') or 0) for t in mine)
  prices=[t.get('price') for t in mine if t.get('price') is not None]
  action=str(pa.get('action') or '').upper()
  placed=any(b.get('agent_id')==aid for b in bids)
  accepted=True if action=='HOLD' else (placed if action in ('BUY','SELL') else None)
  sub={
   'accepted':accepted,
   'filled_quantity':filled,
   'average_fill_price':(sum(prices)/len(prices) if prices else None),
   'trades':mine,
   'submitted_quantity':pa.get('quantity'),
  }
  agent=req_obj.get('agent') if isinstance(req_obj.get('agent'),dict) else {}
  market=req_obj.get('market') if isinstance(req_obj.get('market'),dict) else {}
  turns.append({
   'i':i,
   'time':row.get('time'),
   'agent_id':aid,
   'latency_ms':row.get('latency_ms'),
   'prompt_tokens':row.get('prompt_tokens'),
   'completion_tokens':row.get('completion_tokens'),
   'parsed_action':pa,
   'action':pa.get('action'),
   'price':pa.get('price'),
   'quantity':pa.get('quantity'),
   'asset':pa.get('asset'),
   'submission':sub,
   'accepted':accepted,
   'filled_quantity':filled,
   'parse':{'success':bool(pa)},
   'action_validation':{'valid':bool(pa.get('action'))},
   'canonical_request':req,
   'raw_provider_response':row.get('raw_provider_response'),
   'market_state':market,
   'agent_state_before':agent,
   'agent_state_after':agent,
   'model':row.get('model'),
   'request_id':row.get('request_id'),
   'seed':row.get('seed', ev.get('seed') if ev else None),
   'audit_metadata':{
    'provider':row.get('provider'),
    'model':row.get('model'),
    'version':row.get('version'),
    'request_id':row.get('request_id'),
    'experiment_seed':row.get('seed', ev.get('seed') if ev else None),
    'cell_index':ev.get('index') if ev else None,
    'design':ev.get('design') if ev else None,
    'repeat':ev.get('repeat') if ev else None,
   },
  })
 return turns

def _lab_run_dir():
 orig=HETERO_ROOT/'runs'/recovery.ORIGINAL_RUN_ID
 if orig.is_dir():
  return orig
 return _latest_hetero_run_dir()

def lab_cells_index():
 """Completed archived cells only. Read-only; does not follow campaign display reset."""
 run=_lab_run_dir()
 if not run:
  return {'cells':[],'n_completed':0,'read_only':True,'layout':'schematic_seating','error':'no heterogeneous run directory'}
 cells=[]
 ev_dir=run/'cell_evidence'
 files=sorted(ev_dir.glob('cell_*.json'), key=lambda p:p.name) if ev_dir.is_dir() else []
 for path in files:
  ev=_read_json_file(path)
  if not ev or str(ev.get('status') or '')!='completed':
   continue
  cells.append({
   'index':ev.get('index'),
   'seed':ev.get('seed'),
   'repeat':ev.get('repeat'),
   'design':ev.get('design'),
   'efficiency':ev.get('efficiency'),
   'trades':ev.get('trades'),
   'turns':ev.get('observed_llm_turns'),
   'agents':ev.get('agents'),
   'status':ev.get('status'),
  })
 cells.sort(key=lambda c:(c.get('index') is None, c.get('index') or 0))
 n=len(cells)
 planned=120
 remaining=max(0,planned-n)
 return {
  'cells':cells,
  'n_completed':n,
  'n_planned':planned,
  'n_remaining':remaining,
  'archive':True,
  'run_id':run.name,
  'run_dir':str(run.relative_to(ROOT)) if str(run).startswith(str(ROOT)) else str(run),
  'read_only':True,
  'protected':True,
  'layout':'schematic_seating',
  'source':'archived cell_evidence; read-only replay of completed cells',
 }

def lab_summary():
 idx=lab_cells_index()
 cells=idx.get('cells') or []
 by={}
 etas=[]
 for c in cells:
  design=str(c.get('design') or 'unknown')
  bucket=by.setdefault(design,{'design':design,'n':0,'etas':[],'trades':0,'turns':0})
  bucket['n']+=1
  if c.get('efficiency') is not None:
   try:
    eta=float(c.get('efficiency')); bucket['etas'].append(eta); etas.append(eta)
   except (TypeError,ValueError):
    pass
  try: bucket['trades']+=int(c.get('trades') or 0)
  except (TypeError,ValueError): pass
  try: bucket['turns']+=int(c.get('turns') or 0)
  except (TypeError,ValueError): pass
 def _stats(vals):
  if not vals: return {'n':0,'mean':None,'min':None,'max':None}
  return {'n':len(vals),'mean':sum(vals)/len(vals),'min':min(vals),'max':max(vals)}
 designs=[]
 for design,bucket in sorted(by.items()):
  st=_stats(bucket['etas'])
  designs.append({
   'design':design,'n':bucket['n'],'trades':bucket['trades'],'turns':bucket['turns'],
   'avg_h':st['mean'],'min_h':st['min'],'max_h':st['max'],
  })
 bins=[{'lo':i*10,'hi':(i+1)*10,'count':0} for i in range(10)]
 for eta in etas:
  b=min(9,max(0,int(eta//10)))
  bins[b]['count']+=1
 return {
  'n_completed':len(cells),
  'n_planned':idx.get('n_planned') or 120,
  'n_remaining':idx.get('n_remaining') or 0,
  'archive':True,
  'run_id':idx.get('run_id'),
  'efficiency':_stats(etas),
  'by_design':designs,
  'histogram':bins,
  'read_only':True,
  'note':'Archive of completed cell_evidence only. Remaining cells are not invented here.',
 }

def lab_cell_by_index(index:int):
 """Read-only replay payload for one archived completed cell. Never writes."""
 run=_lab_run_dir()
 if not run:
  return {'error':'no heterogeneous run directory'}
 try:
  index=int(index)
 except Exception:
  return {'error':'index must be an integer'}
 name=f'cell_{index}.json'
 ev=_read_json_file(run/'cell_evidence'/name)
 if not ev:
  return {'error':f'no archived completed cell {index}'}
 if str(ev.get('status') or '')!='completed':
  return {'error':f'cell {index} is not a completed observation','protected':True}
 obs=_read_json_file(run/'cell_observations'/name) or {}
 snap=_read_json_file(run/'cell_snapshots'/name) or {}
 turns=_hetero_turns(obs, ev)
 trades=obs.get('trades') if isinstance(obs.get('trades'),list) else []
 bids=obs.get('bids') if isinstance(obs.get('bids'),list) else []
 agents=[]
 seen=set()
 for t in turns:
  aid=t.get('agent_id')
  if not aid or aid in seen:
   continue
  seen.add(aid)
  role=str((t.get('agent_state_before') or {}).get('role') or '').lower()
  agents.append({
   'agent_id':aid,
   'model':t.get('model'),
   'role':role or None,
   'side':'buy' if role=='buyer' else ('sell' if role=='seller' else None),
   'private_value':(t.get('agent_state_before') or {}).get('private_value'),
  })
 if not agents:
  for fp in (snap.get('fingerprints') or []):
   if isinstance(fp,dict) and fp.get('agent_id'):
    agents.append({'agent_id':fp.get('agent_id'),'model':fp.get('model'),'role':None,'side':None,'private_value':None})
 events=[]
 for t in turns:
  events.append({'kind':'decision','time':t.get('time'),'agent_id':t.get('agent_id'),'model':t.get('model'),
   'action':t.get('action'),'price':t.get('price'),'quantity':t.get('quantity'),
   'latency_ms':t.get('latency_ms'),'turn_i':t.get('i')})
 for b in bids:
  if isinstance(b,dict):
   events.append({'kind':'bid','time':b.get('time'),'agent_id':b.get('agent_id'),'price':b.get('price'),
    'quantity':b.get('quantity'),'side':b.get('side'),'asset':b.get('asset')})
 for tr in trades:
  if isinstance(tr,dict):
   events.append({'kind':'trade','time':tr.get('time'),'buyer':tr.get('buyer'),'seller':tr.get('seller'),
    'price':tr.get('price'),'quantity':tr.get('quantity'),'asset':tr.get('asset')})
 kind_rank={'decision':0,'bid':1,'trade':2}
 events.sort(key=lambda e:(e.get('time') is None, e.get('time') or 0, kind_rank.get(e.get('kind'),9)))
 for i,e in enumerate(events):
  e['i']=i
 prices=[tr.get('price') for tr in trades if isinstance(tr,dict) and tr.get('price') is not None]
 ticks={}
 for t in turns:
  m=t.get('market_state') if isinstance(t.get('market_state'),dict) else {}
  tm=m.get('time', t.get('time'))
  if tm is None:
   continue
  ticks[tm]={
   'time':tm,
   'last_trade':m.get('last_trade'),
   'best_bid':m.get('best_bid'),
   'best_ask':m.get('best_ask'),
   'fundamental':m.get('fundamental'),
   'public_signal':m.get('public_signal'),
  }
 market_ticks=sorted(ticks.values(), key=lambda x:x.get('time') or 0)
 signals=[x.get('public_signal') for x in market_ticks if x.get('public_signal') not in (None,0,0.0)]
 return {
  'index':index,
  'seed':ev.get('seed'),
  'repeat':ev.get('repeat'),
  'design':ev.get('design'),
  'status':ev.get('status'),
  'efficiency':ev.get('efficiency'),
  'trade_count':len(trades),
  'agent_count':len(agents),
  'turn_count':len(turns),
  'mean_trade_price':(sum(prices)/len(prices) if prices else None),
  'layout':'schematic_seating',
  'layout_note':'One independent CDA session. Agents are listed by recorded role; the book shows archived bids still resting at this event and orange lines are recorded trades. Positions are schematic, not reconstructed motion.',
  'source':'archived cell_evidence + cell_observations; read-only',
  'protected':True,
  'run_id':run.name,
  'file':str((run/'cell_observations'/name).relative_to(ROOT)) if str(run).startswith(str(ROOT)) else name,
  'agents':agents,
  'events':events,
  'trades':trades,
  'bids':bids,
  'turns':turns,
  'market_ticks':market_ticks,
  'recorded_intervention':bool(signals),
 }

def _hetero_cell_detail(model:str, seed:int):
 items=_hetero_cells_index()
 match=None
 for c in items:
  if str(c.get('model'))==str(model) and int(c.get('seed') or -1)==int(seed):
   match=c; break
 if not match:
  return None
 prog=_hetero_progress()
 if not prog:
  return {'error':'hetero run directory missing'}
 run_dir=prog['run_dir']
 idx=match.get('cell_index')
 name=f"cell_{idx}.json" if idx is not None else None
 ev=_read_json_file(run_dir/'cell_evidence'/name) if name else None
 obs=_read_json_file(run_dir/'cell_observations'/name) if name else None
 turns=_hetero_turns(obs or {}, ev or {})
 return {
  'model':model,
  'seed':int(seed),
  'file':match.get('file'),
  'kind':'hetero',
  'turns':turns,
  'turn_count':len(turns),
  'cell':{
   'cell_ok':match.get('cell_ok'),
   'treatment_efficiency':match.get('treatment_efficiency'),
   'design':match.get('design'),
   'repeat':match.get('repeat'),
   'cell_index':idx,
   'agents':match.get('agents'),
   'error':match.get('error'),
  },
 }

CLAIM_BATCH=(
 "Scripted Phase I software-path seed sweep only. "
 "Does not claim live LLM economics, Phase I classifier accuracy, or DARPA milestone readiness."
)
CLAIM_CAMPAIGN=(
 "Live Ollama Layer B campaign. darpa_claim_ready only under scope=local_ollama_poc when readiness gates pass. "
 "No Phase II construct validation; no commercial multi-provider matrix claim."
)

def find_exe(name:str)->Path:
 for d in (ROOT/'build-dv026', ROOT/'build-review', ROOT/'build'):
  p=d/name
  if p.is_file() and os.access(p, os.X_OK): return p
 raise FileNotFoundError(
  f'{name} not found in build-dv026/ or build/. '
  f'Run: cmake -S . -B build-dv026 -DCMAKE_BUILD_TYPE=Release && '
  f'cmake --build build-dv026 -j8 --target {name}')

def _software_green(data:dict)->bool:
 if data.get('software_green') is True: return True
 pi=data.get('phase_i')
 if isinstance(pi,dict) and pi.get('software_green') is True: return True
 return False

def _fail_checks(data:dict):
 pi=data.get('phase_i') if isinstance(data.get('phase_i'),dict) else data
 checks=pi.get('checks') if isinstance(pi,dict) else None
 if not isinstance(checks,list): return []
 return [c.get('id','?') for c in checks if isinstance(c,dict) and c.get('status')=='FAIL']

class BatchJob:
 def __init__(self):
  self.reset_idle()
 def reset_idle(self):
  self.running=False; self.cancel=False; self.command='phase-i'
  self.base_seed=424242; self.n=0; self.done=0
  self.pass_count=0; self.fail_count=0; self.current_seed=None
  self.started_at=None; self.finished_at=None; self.last_error=None
  self.fail_seeds=[]; self.check_fail_counts={}; self.cancelled=False
  self.elapsed_sum=0.0
 def snapshot(self):
  pct=(100.0*self.done/self.n) if self.n else 0.0
  eta=None
  if self.running and self.done>0 and self.n>self.done:
   mean=self.elapsed_sum/self.done
   eta=mean*(self.n-self.done)
  return dict(
   running=self.running, cancelled=self.cancelled, command=self.command,
   base_seed=self.base_seed, n=self.n, done=self.done,
   pass_count=self.pass_count, fail_count=self.fail_count,
   current_seed=self.current_seed, started_at=self.started_at, finished_at=self.finished_at,
   last_error=self.last_error, fail_seeds=self.fail_seeds[:],
   check_fail_counts=dict(self.check_fail_counts),
   pct=round(pct,2), eta_sec=None if eta is None else round(eta,1),
   pass_rate=(self.pass_count/self.done) if self.done else None,
   live_llm=False, darpa_claim_ready=False,
   claim_boundary=CLAIM_BATCH,
  )
 def summary(self):
  s=self.snapshot()
  s['complete']= (not self.running) and self.n>0 and (self.done>=self.n or self.cancelled)
  return s

class CampaignJob:
 def __init__(self):
  self.reset_idle()
 def reset_idle(self):
  self.running=False; self.cancel=False
  self.base_seed=424242; self.n_seeds=20; self.smoke=False; self.population_design='reference_counterparty'; self.activation_design='sequential_interaction'; self.inference_repeats=3
  self.done=0; self.total=0
  self.pass_count=0; self.fail_count=0
  self.current_model=None; self.current_seed=None
  self.started_at=None; self.finished_at=None; self.last_error=None
  self.cancelled=False; self.proc=None
  self.result=None; self.readiness_checks=[]; self.scope='not_ready'
  self.darpa_claim_ready=False; self.available_models=0; self.cells_ok=0
  self.distinct_live_models_ok=0
  self.gate_partial=None
  self.stale_summary=None  # {path, on_disk_revision, required_revision} when hydrate skipped
 def snapshot(self):
  pct=(100.0*self.done/self.total) if self.total else (100.0 if self.result else 0.0)
  complete=(not self.running) and (self.result is not None or self.cancelled or (self.finished_at is not None and self.done>0))
  out=dict(
   running=self.running, cancelled=self.cancelled, command=('hetero-campaign' if self.population_design=='heterogeneous_multi_llm' else 'ollama-campaign'),
   base_seed=self.base_seed, n_seeds=self.n_seeds, smoke=self.smoke, population_design=self.population_design, activation_design=self.activation_design, inference_repeats=self.inference_repeats,
   done=self.done, total=self.total, pass_count=self.pass_count, fail_count=self.fail_count,
   current_model=self.current_model, current_seed=self.current_seed,
   started_at=self.started_at, finished_at=self.finished_at, last_error=self.last_error,
   pct=round(pct,2),
   eta_sec=(round(((time.time()-self.started_at)/self.done)*(self.total-self.done),1) if self.running and self.done>0 and self.total>self.done and self.started_at else None),
   live_llm=True,
   darpa_claim_ready=self.darpa_claim_ready, scope=self.scope,
   available_models=self.available_models, cells_ok=self.cells_ok,
   distinct_live_models_ok=self.distinct_live_models_ok,
   readiness_checks=list(self.readiness_checks),
   gate_partial=self.gate_partial,
   claim_boundary=CLAIM_CAMPAIGN,
   expectations_frozen=(_load_expectations().get('status')=='frozen'),
   expectations_manifest_sha256=_load_expectations().get('manifest_sha256'),
   complete=complete,
   implementation_revision=IMPLEMENTATION_REVISION,
   stale_summary=self.stale_summary,
  )
  if self.result is not None:
   out['result']=self.result
  return out
 def summary(self):
  s=self.snapshot()
  return s

class Session:
 def __init__(self):
  self.lock=threading.RLock();self.cond=threading.Condition(self.lock);self.proc=None
  self.generation=0;self.replay_plan=[];self.last_run=None;self.dv026_last=None
  self.layer_a_cache=None  # persists across preflight overwrites of dv026_last
  self.batch=BatchJob(); self.campaign=CampaignJob(); self.preflight=None
  self.campaign_display_reset=False  # True after console Reset until new campaign/Layer A
  self.reset()
  # Restore Layer A only when persisted fingerprints still match.
  try:
   restored=recovery.restore_layer_a_gate(ROOT,config_fingerprint=_layer_a_config_fp(),code_fingerprint=_code_fingerprint())
   if restored.get('restored'):
    self.layer_a_cache=restored
   else:
    self.layer_a_cache=restored if restored.get('stale') else None
    if self.layer_a_cache is None:
     path=ROOT/'results'/'dv026_ollama_campaign'/'summary.json'
     if path.is_file():
      self.layer_a_cache=_layer_a_payload_from_doc(json.loads(path.read_text()))
  except Exception:
   pass
  try:
   run=_latest_hetero_run_dir()
   if run: recovery.reconcile(run, persist=False)
  except Exception:
   pass
 def reset(self):
  with self.lock:
   self.generation+=1
   if self.proc and self.proc.poll() is None:self.proc.kill()
   self.proc=None;self.status='idle';self.seed=None;self.events=0;self.speed=30
   self.impulse=0.;self.edge=True;self.trace=[];self.outcomes={};self.interventions=[];self.error=None
   self.replay_plan=[];self.cond.notify_all()
 def console_reset(self):
  """Return Research Console to workflow start without deleting on-disk evidence files."""
  with self.lock:
   if self.campaign.running:
    self.campaign.cancel=True
    p=self.campaign.proc
    if p and p.poll() is None:
     try: p.kill()
     except Exception: pass
    # Wait briefly for worker to notice cancel
   running_was=bool(self.campaign.running)
  # Allow cancel path to settle outside lock if needed
  if running_was:
   deadline=time.time()+2.0
   while time.time()<deadline:
    with self.lock:
     if not self.campaign.running: break
    time.sleep(0.05)
  with self.lock:
   c=self.campaign
   c.reset_idle()
   c.result=None
   c.darpa_claim_ready=False
   c.scope='not_ready'
   c.readiness_checks=[]
   c.gate_partial=None
   c.stale_summary=None
   c.available_models=0; c.cells_ok=0; c.distinct_live_models_ok=0
   c.pass_count=0; c.fail_count=0; c.done=0; c.total=0
   c.started_at=None; c.finished_at=None; c.last_error=None
   c.cancelled=False; c.proc=None
   self.campaign_display_reset=True
   # Strip Layer A / last DV026 payload so qualify must be re-run
   self.layer_a_cache=None
   if isinstance(self.dv026_last,dict):
    self.dv026_last={k:v for k,v in self.dv026_last.items() if k not in ('layer_a','layer_a_pass','mean_efficiency_cda','mean_efficiency_sealed')}
   else:
    self.dv026_last=None
  expectations=_unfreeze_expectations_to_draft()
  # One preflight so Ready can return without a second toolbar button
  preflight=None; preflight_error=None
  try:
   pfraw=self.run_dv026('ollama-preflight',424242)
   preflight=pfraw.get('preflight',pfraw) if isinstance(pfraw,dict) else pfraw
   with self.lock:
    self.preflight=preflight if isinstance(preflight,dict) else None
  except Exception as e:
   preflight_error=str(e)
  with self.lock:
   snap=self.campaign.snapshot()
  return {
   'ok':True,
   'campaign_display_reset':True,
   'prior_evidence_on_disk':(
    CAMPAIGN_DIR.is_dir() and (
     (CAMPAIGN_DIR/'cells.jsonl').is_file() or any(CAMPAIGN_DIR.glob('cell_*.jsonl'))
    )
   ),
   'expectations':expectations,
   'campaign':snap,
   'preflight':preflight,
   'preflight_error':preflight_error,
   'message':'Console reset to workflow start. Prior campaign files remain on disk but are hidden from charts until a new run.',
  }
 def start(self,seed,events,speed):
  with self.lock:
   if self.batch.running or self.campaign.running: raise ValueError('DV026 job running; cancel or wait first')
   if self.status in ('running','paused'):raise ValueError('Experiment already active; reset first')
   if not 1<=events<=10000 or not 1<=speed<=250:raise ValueError('Invalid events or speed')
   self.reset();self.seed=seed;self.events=events;self.speed=speed;self.status='running'
   exe=find_exe('v27-live-runner')
   self.proc=subprocess.Popen([str(exe),str(seed),str(events)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,bufsize=1)
   gen=self.generation;threading.Thread(target=self.pump,args=(gen,),daemon=True).start()
 def pump(self,gen):
  p=self.proc
  try:
   for line in p.stdout:
    item=json.loads(line)
    with self.cond:
     if gen!=self.generation:return
     if item['kind']=='decision':self.trace.append(item)
     elif item['kind']=='outcome':self.outcomes[item['arm']]=item
     elif item['kind']=='complete':
      self.status='complete'
      self.last_run=dict(seed=self.seed,events=self.events,speed=self.speed,interventions=[dict(after_event=i['after_event'],impulse=i['impulse'],edge=i['edge']) for i in self.interventions])
     self.cond.notify_all()
     if item['kind']!='decision':continue
     while self.status=='paused' and gen==self.generation:self.cond.wait(.2)
     if gen!=self.generation:return
     if self.status!='running':return
     impulse=self.impulse;edge=int(self.edge);speed=self.speed
    time.sleep(1/speed)
    with self.cond:
     while self.status=='paused' and gen==self.generation:self.cond.wait(.2)
     if gen!=self.generation or self.status!='running':return
     for iv in list(self.replay_plan):
      if iv['after_event']==item['event']:
       self.impulse=iv['impulse'];self.edge=iv['edge']
       self.interventions.append(dict(iv,wall_time=time.time(),applies='next activation',replayed=True))
       self.replay_plan.remove(iv)
     p.stdin.write(f'NEXT {self.impulse} {int(self.edge)}\n');p.stdin.flush()
   rc=p.wait(timeout=3)
   with self.cond:
    if gen==self.generation and self.status!='complete':self.status='error';self.error=p.stderr.read() or f'Engine exit {rc}'
  except Exception as e:
   with self.cond:
    if gen==self.generation:self.status='error';self.error=str(e)
 def snapshot(self,from_index=0):
  with self.lock:
   return dict(status=self.status,seed=self.seed,events=self.events,speed=self.speed,impulse=self.impulse,edge=self.edge,
    count=len(self.trace),trace=self.trace[max(0,from_index):],outcomes=self.outcomes.copy(),interventions=self.interventions[:],error=self.error,
    source='live C++ controlled 12-trader CDA simulation',live_llm=False)
 def run_dv026(self,command='bundle',seed=424242,model=None):
  with self.lock:
   if self.batch.running or self.campaign.running: raise ValueError('DV026 job running; cancel or wait first')
  allowed={'bundle','phase-i','phase-ii','layer-a','ten-llm','llm-slice','population','adaptive',
           'ollama-preflight','ollama-slice','ollama-paired','ollama-campaign','hetero-pop','hetero-campaign','info-contrast','info-contrast-live'}
  if command not in allowed: raise ValueError('Invalid DV026 command')
  if not isinstance(seed,int) or seed<0: raise ValueError('Invalid seed')
  exe=find_exe('dv026-workbench-runner')
  env=os.environ.copy()
  timeout=60
  args=[str(exe),command,str(seed)]
  if command.startswith('ollama'):
   env.setdefault('COAGENTICS_LLM_PROVIDER','ollama')
   env.setdefault('COAGENTICS_LLM_BASE_URL','http://127.0.0.1:11434/v1')
   if model: env['COAGENTICS_LLM_MODEL']=str(model)
   elif 'COAGENTICS_LLM_MODEL' not in env: env['COAGENTICS_LLM_MODEL']='llama3.2'
   timeout=300
   ex=_load_expectations()
   if ex.get('status')=='frozen' and ex.get('manifest_sha256'):
    env['COAGENTICS_EXPECTATIONS_SHA256']=str(ex['manifest_sha256'])
    env['COAGENTICS_EXPECTATIONS_PATH']=str(EXPECTATIONS_PATH)
  if command=='ollama-campaign':
   timeout=None  # long-running; prefer campaign_start API
   args=[str(exe),command,str(seed),'2','--smoke']
  try:
   proc=subprocess.run(args,capture_output=True,text=True,timeout=timeout,cwd=str(ROOT),env=env)
  except subprocess.TimeoutExpired as e:
   raise ValueError(f'DV026 runner timed out: {e}') from e
  if proc.returncode!=0:
   raise ValueError(proc.stderr.strip() or f'DV026 runner exit {proc.returncode}')
  data=json.loads(proc.stdout)
  with self.lock:
   # Preflight must not erase a prior Layer A qualification from dv026_last.
   if command=='ollama-preflight' and isinstance(self.dv026_last,dict):
    keep=_layer_a_payload_from_doc(self.dv026_last) or self.layer_a_cache
    self.dv026_last=data if isinstance(data,dict) else {'result':data}
    if keep:
     self.dv026_last={**self.dv026_last, 'layer_a':keep, 'layer_a_pass':keep.get('layer_a_pass'),
                      'mean_efficiency_cda':keep.get('mean_efficiency_cda'),
                      'mean_efficiency_sealed':keep.get('mean_efficiency_sealed')}
     self.layer_a_cache=keep
   else:
    self.dv026_last=data
   if command in ('layer-a','bundle'):
    la=_layer_a_payload_from_doc(data if isinstance(data,dict) else {})
    if la:
     self.layer_a_cache=la
     _persist_layer_a_if_passed(la)
     # Fresh Layer A after Reset counts for Full gates; chart hide stays until campaign_start.
   if command=='ollama-preflight':
    self.preflight=data.get('preflight', data) if isinstance(data,dict) else data
    # Do not resurrect Layer A from a prior session after Reset.
    if self.campaign_display_reset and not self.layer_a_cache:
     if isinstance(self.dv026_last,dict):
      self.dv026_last={k:v for k,v in self.dv026_last.items()
                       if k not in ('layer_a','layer_a_pass','mean_efficiency_cda','mean_efficiency_sealed')}
  return data
 def batch_start(self,n=1000,base_seed=424242):
  with self.lock:
   if self.batch.running or self.campaign.running: raise ValueError('Job already running')
   if self.status in ('running','paused'): raise ValueError('Market Live experiment active; reset first')
   if not isinstance(n,int) or not 1<=n<=10000: raise ValueError('n must be 1..10000')
   if not isinstance(base_seed,int) or base_seed<0: raise ValueError('Invalid base_seed')
   find_exe('dv026-workbench-runner')  # fail fast
   b=self.batch
   b.reset_idle()
   b.running=True; b.command='phase-i'; b.base_seed=base_seed; b.n=n
   b.started_at=time.time()
   threading.Thread(target=self._batch_worker,daemon=True).start()
   return b.snapshot()
 def batch_cancel(self):
  with self.lock:
   if not self.batch.running: raise ValueError('No batch running')
   self.batch.cancel=True
   return self.batch.snapshot()
 def batch_status(self):
  with self.lock:
   return self.batch.snapshot()
 def batch_export(self):
  with self.lock:
   b=self.batch
   if b.n==0: raise ValueError('No batch has been run')
   if b.running: raise ValueError('Batch still running')
   return b.summary()
 def _batch_worker(self):
  exe=find_exe('dv026-workbench-runner')
  while True:
   with self.lock:
    b=self.batch
    if b.cancel or b.done>=b.n:
     b.running=False; b.cancelled=bool(b.cancel and b.done<b.n)
     b.finished_at=time.time(); b.current_seed=None
     return
    seed=b.base_seed+b.done
    b.current_seed=seed
   t0=time.time()
   try:
    proc=subprocess.run([str(exe),'phase-i',str(seed)],capture_output=True,text=True,timeout=60,cwd=str(ROOT))
    if proc.returncode!=0:
     raise RuntimeError(proc.stderr.strip() or f'exit {proc.returncode}')
    data=json.loads(proc.stdout)
    ok=_software_green(data)
    fails=_fail_checks(data)
   except Exception as e:
    ok=False; fails=[]; err=str(e)
    data=None
   else:
    err=None
   dt=time.time()-t0
   with self.lock:
    b=self.batch
    b.elapsed_sum+=dt
    b.done+=1
    if ok:
     b.pass_count+=1
    else:
     b.fail_count+=1
     if len(b.fail_seeds)<50: b.fail_seeds.append(seed)
     for cid in fails:
      b.check_fail_counts[cid]=b.check_fail_counts.get(cid,0)+1
     if err: b.last_error=err
     elif data is not None and not ok: b.last_error=f'software_green false at seed {seed}'
    if b.cancel or b.done>=b.n:
     b.running=False; b.cancelled=bool(b.cancel and b.done<b.n)
     b.finished_at=time.time(); b.current_seed=None
     return

 def _layer_a_is_qualified(self):
  """True when Layer A (η gate) has passed in this session (or pre-reset disk hydrate).

  After console Reset, on-disk summary.json is ignored until a fresh layer-a run
  repopulates layer_a_cache. Layer A is engine calibration (no LLMs); revision
  still gates claim READY, not η.
  """
  with self.lock:
   if isinstance(self.layer_a_cache,dict) and (self.layer_a_cache.get('layer_a_pass') or self.layer_a_cache.get('pass')):
    return True
   last=self.dv026_last if isinstance(self.dv026_last,dict) else {}
   la=_layer_a_payload_from_doc(last)
   if la and la.get('layer_a_pass'):
    self.layer_a_cache=la
    return True
   if self.campaign.result and isinstance(self.campaign.result,dict):
    camp=self.campaign.result.get('campaign') or self.campaign.result
    la=_layer_a_payload_from_doc(camp if isinstance(camp,dict) else {})
    if la and la.get('layer_a_pass'):
     self.layer_a_cache=la
     return True
   # After Reset, require a fresh in-session Layer A — do not rehydrate from disk.
   if self.campaign_display_reset:
    return False
  path=ROOT/'results'/'dv026_ollama_campaign'/'summary.json'
  if path.is_file():
   try:
    summ=json.loads(path.read_text())
    la=_layer_a_payload_from_doc(summ)
    if la and la.get('layer_a_pass'):
     with self.lock:
      if self.campaign_display_reset:
       return False
      self.layer_a_cache=la
     return True
   except Exception:
    pass
  return False
 def layer_a_status(self):
  """Layer A payload for Console hydration (survives preflight /last overwrites)."""
  empty={'layer_a_pass':False,'pass':False,'mean_efficiency_cda':None,'mean_efficiency_sealed':None,'trials':[]}
  with self.lock:
   if isinstance(self.layer_a_cache,dict):
    return dict(self.layer_a_cache)
   la=_layer_a_payload_from_doc(self.dv026_last if isinstance(self.dv026_last,dict) else {})
   if la: return la
   if self.campaign_display_reset:
    return empty
  path=ROOT/'results'/'dv026_ollama_campaign'/'summary.json'
  if path.is_file():
   try:
    la=_layer_a_payload_from_doc(json.loads(path.read_text()))
    if la:
     with self.lock:
      if self.campaign_display_reset:
       return empty
      self.layer_a_cache=la
     return la
   except Exception:
    pass
  return empty
 def campaign_start(self, base_seed=424242, n_seeds=20, smoke=False, population_design='reference_counterparty', activation_design='sequential_interaction'):
  with self.lock:
   if self.batch.running or self.campaign.running: raise ValueError('Job already running')
   if self.status in ('running','paused'): raise ValueError('Market Live experiment active; reset first')
   if not isinstance(n_seeds,int) or not 1<=n_seeds<=100: raise ValueError('n_seeds must be 1..100')
   if not isinstance(base_seed,int) or base_seed<0: raise ValueError('Invalid base_seed')
  expectations=_load_expectations()
  if expectations.get('integrity_ok') is False or expectations.get('integrity_blocked'):
   raise ValueError(expectations.get('integrity_error') or 'Research expectations failed integrity verification; re-freeze before starting a full campaign.')
  if not smoke and expectations.get('status')!='frozen':
   raise ValueError('Research expectations must be frozen before starting a full live campaign.')
  if not smoke:
   _validate_expectations(expectations)
   planned=int((expectations.get('sample_size') or {}).get('planned_seeds',0))
   if n_seeds!=planned:
    raise ValueError(f'Campaign seed count ({n_seeds}) does not match frozen preregistration ({planned}). Create/amend and refreeze expectations to change sample size.')
   if not self._layer_a_is_qualified():
    raise ValueError('Layer A market qualification is required before starting a full live campaign (run layer-a; Smoke mode exempt).')
  if population_design not in ('reference_counterparty','heterogeneous_multi_llm'):
   raise ValueError('Invalid population_design')
  if activation_design not in ('sequential_interaction','frozen_snapshot','both'):
   raise ValueError('Invalid activation_design')
  if population_design=='reference_counterparty' and activation_design!='sequential_interaction':
   raise ValueError('Frozen Snapshot applies to the heterogeneous multi-LLM population design, not the reference-counterparty campaign.')
  # Refresh the runtime before starting so the UI has a real denominator and never
  # starts a "live" campaign against an unavailable provider.
  try:
   pfraw=self.run_dv026('ollama-preflight', base_seed)
   pf=pfraw.get('preflight',pfraw) if isinstance(pfraw,dict) else {}
  except Exception as e:
   raise ValueError(f'Live LLM preflight failed: {e}') from e
  if not pf.get('ollama_reachable'):
   raise ValueError('Ollama is not reachable. Start Ollama and refresh LLM availability before starting a live campaign.')
  available=len(pf.get('available_models') or [])
  if available==0:
   raise ValueError('Ollama is reachable but no configured DV026 models are installed.')
  model_count=min(available,2) if smoke else available
  seed_count=min(n_seeds,2) if smoke else n_seeds
  inference_repeats=1 if smoke else 3
  design_count=1 if activation_design!='both' else 2
  with self.lock:
   find_exe('dv026-workbench-runner')
   c=self.campaign
   c.reset_idle()
   self.campaign_display_reset=False
   c.running=True; c.base_seed=base_seed; c.n_seeds=seed_count; c.smoke=bool(smoke)
   c.population_design=population_design; c.activation_design=activation_design; c.inference_repeats=inference_repeats
   c.available_models=model_count; c.total=(seed_count*inference_repeats*design_count if population_design=='heterogeneous_multi_llm' else model_count*seed_count)
   c.started_at=time.time()
   c.current_model='(starting)'; c.current_seed=base_seed
   threading.Thread(target=self._campaign_worker,daemon=True).start()
   return c.snapshot()
 def campaign_cancel(self):
  with self.lock:
   if not self.campaign.running: raise ValueError('No campaign running')
   self.campaign.cancel=True
   p=self.campaign.proc
   if p and p.poll() is None:
    try: p.kill()
    except Exception: pass
   return self.campaign.snapshot()
 def campaign_recovery(self, run_dir=None):
  run=Path(run_dir) if run_dir else _lab_run_dir()
  if not run:
   return {'execution_state':'idle','interrupted':False,'done':0,'total':0,'note':'no heterogeneous run directory'}
  rec=recovery.reconcile(run, persist=False)
  rec=recovery.apply_live_session(rec, recovery.load_session(HETERO_ROOT))
  rec['protected_completed_indices']=(rec.get('inventory') or {}).get('completed_indices') or []
  rec['note']=recovery.PROTECTED_NOTE
  return rec
 def campaign_verify_recovery(self, run_dir=None):
  run=Path(run_dir) if run_dir else _lab_run_dir()
  if not run:
   raise ValueError('No heterogeneous campaign run to verify')
  models=_ollama_status().get('models')
  v=recovery.verify_recovery(run, ollama_models=models, persist=False)
  v['note']=recovery.PROTECTED_NOTE
  return v
 def campaign_export_recovery(self, run_dir=None):
  run=Path(run_dir) if run_dir else _lab_run_dir()
  if not run:
   raise ValueError('No heterogeneous campaign run to export')
  dest=HETERO_ROOT/'recovery_report.json'
  models=_ollama_status().get('models')
  recovery.export_recovery_report(run, dest, ollama_models=models)
  return {'path':str(dest.relative_to(ROOT)), 'report':recovery.verify_recovery(run, ollama_models=models, persist=False)}
 def campaign_resume(self, run_dir=None):
  with self.lock:
   if self.batch.running or self.campaign.running: raise ValueError('Job already running')
   if self.status in ('running','paused'): raise ValueError('Market Live experiment active; reset first')
  run=Path(run_dir) if run_dir else _lab_run_dir()
  if not run:
   raise ValueError('No interrupted campaign to resume')
  before=recovery.snapshot_completed_hashes(run)
  v=self.campaign_verify_recovery(run)
  if not v.get('resume_eligible'):
   raise ValueError('Resume blocked; original completed cells were left unchanged. '+'; '.join(v.get('resume_blocked_reasons') or ['not eligible']))
  inv=v.get('inventory') or {}
  n_seeds=int(inv.get('config',{}).get('n_seeds') or 20)
  designs=list(inv.get('config',{}).get('designs') or recovery.DESIGNS)
  activation='both' if len(designs)>1 else (designs[0] if designs else 'sequential_interaction')
  with self.lock:
   find_exe('dv026-workbench-runner')
   c=self.campaign
   c.reset_idle()
   self.campaign_display_reset=False
   c.running=True; c.base_seed=int(inv.get('config',{}).get('base_seed') or 424242)
   c.n_seeds=n_seeds; c.smoke=False
   c.population_design='heterogeneous_multi_llm'; c.activation_design=activation
   c.inference_repeats=int(inv.get('config',{}).get('repeats') or 3)
   c.total=int(inv.get('planned') or 0)
   c.done=int(inv.get('completed') or 0)
   c.cells_ok=c.done; c.pass_count=c.done
   c.started_at=time.time()
   c.current_model='(resume)'; c.current_seed=c.base_seed
   c._resume_run_dir=str(run)
   c._resume_completed_sha256=before
  threading.Thread(target=self._campaign_resume_worker,daemon=True).start()
  return {**c.snapshot(), 'resumed':True, 'protected_completed':sorted(int(i) for i in before), 'note':recovery.PROTECTED_NOTE}
 def _campaign_resume_worker(self):
  resume_dir=getattr(self.campaign,'_resume_run_dir',None)
  before=getattr(self.campaign,'_resume_completed_sha256',{}) or {}
  try:
   self._campaign_worker()
  finally:
   run=Path(resume_dir) if resume_dir else _latest_hetero_run_dir()
   if run and before:
    after=recovery.snapshot_completed_hashes(run)
    changed=[idx for idx, digest in before.items() if after.get(idx)!=digest]
    if changed:
     with self.lock:
      self.campaign.last_error=f'completed cell(s) {changed} changed during resume; originals must remain byte-identical'
      self.campaign.running=False
 def campaign_status(self):
  with self.lock:
   self._hydrate_campaign_from_disk_locked()
   snap=self.campaign.snapshot()
   snap['campaign_display_reset']=bool(self.campaign_display_reset)
  snap=_overlay_hetero_progress(snap)
  la=self.layer_a_status()
  snap['layer_a']=la
  snap['layer_a_pass']=bool(la.get('layer_a_pass') or la.get('pass'))
  if snap.get('mean_efficiency_cda') is None:
   snap['mean_efficiency_cda']=la.get('mean_efficiency_cda')
  if snap.get('mean_efficiency_sealed') is None:
   snap['mean_efficiency_sealed']=la.get('mean_efficiency_sealed')
  return snap
 def _hydrate_campaign_from_disk_locked(self):
  """If no in-memory campaign result, surface the last on-disk CLI/UI summary."""
  c=self.campaign
  if c.running or c.result is not None:
   # Keep any prior stale_summary notice only when still idle without a current result.
   return
  if self.campaign_display_reset:
   # Operator Reset: do not re-claim READY from disk until a new campaign/Layer A.
   c.stale_summary=None
   return
  path=ROOT/'results'/'dv026_ollama_campaign'/'summary.json'
  if not path.is_file():
   c.stale_summary=None
   return
  try:
   camp=json.loads(path.read_text())
  except Exception:
   c.stale_summary=None
   return
  if not isinstance(camp,dict):
   c.stale_summary=None
   return
  if not _current_campaign_summary(camp):
   # Never hydrate readiness from an artifact produced by different code/config semantics.
   on_disk=str(camp.get('implementation_revision') or '') or '(missing)'
   c.stale_summary={
    'path':'results/dv026_ollama_campaign/summary.json',
    'on_disk_revision':on_disk,
    'required_revision':IMPLEMENTATION_REVISION,
    'ignored_darpa_claim_ready':bool(camp.get('darpa_claim_ready')),
    'message':(
     f'On-disk campaign summary is from a different implementation revision '
     f'({on_disk}) and was not loaded as READY. Current revision is '
     f'{IMPLEMENTATION_REVISION}. Re-run Layer A / a full campaign under this build.'
    ),
   }
   return
  c.stale_summary=None
  c.result={'campaign':camp,'darpa_claim_ready':bool(camp.get('darpa_claim_ready')),'scope':str(camp.get('scope') or 'not_ready')}
  c.darpa_claim_ready=bool(camp.get('darpa_claim_ready'))
  c.scope=str(camp.get('scope') or 'not_ready')
  c.available_models=int(camp.get('available_models') or 0)
  c.cells_ok=int(camp.get('cells_ok') or 0)
  c.distinct_live_models_ok=int(camp.get('distinct_live_models_ok') or 0)
  c.pass_count=c.cells_ok
  attempted=int(camp.get('cells_attempted') or 0)
  c.fail_count=max(0, attempted-c.cells_ok)
  c.done=attempted; c.total=attempted
  c.n_seeds=int(camp.get('n_seeds') or c.n_seeds)
  c.base_seed=int(camp.get('base_seed') or c.base_seed)
  c.readiness_checks=camp.get('checks') if isinstance(camp.get('checks'),list) else []
  c.gate_partial=None
  c.finished_at=c.finished_at or path.stat().st_mtime
 def campaign_export(self):
  with self.lock:
   self._hydrate_campaign_from_disk_locked()
   c=self.campaign
   if c.result is None and not c.cancelled and c.started_at is None:
    raise ValueError('No campaign has been run')
   if c.running: raise ValueError('Campaign still running')
   return c.summary()
 def campaign_charts(self):
  """Aggregate cells.jsonl for Research Console SVG panels. No invented live values."""
  with self.lock:
   display_reset=bool(self.campaign_display_reset)
   running=bool(self.campaign.running)
   n_seeds=int(self.campaign.n_seeds or 20)
   layer_a_cda=None
   if self.campaign.result and isinstance(self.campaign.result,dict):
    camp=self.campaign.result.get('campaign') or self.campaign.result
    if isinstance(camp,dict) and camp.get('mean_efficiency_cda') is not None:
     layer_a_cda=float(camp.get('mean_efficiency_cda'))
  if display_reset:
   return {
    'by_model':[],
    'human_ref':{'literature_means':[],'live_mean_efficiency':None,'live_mean_n':0},
    'layer_a_mean_efficiency_cda':None,
    'pipeline':{'attempted':0,'parse_ok':0,'action_valid':0,'market_action_ok':0,'market_accepted':0,'replay_ok':0,'cell_ok':0},
    'n_seeds':n_seeds,
    'partial':False,
    'campaign_display_reset':True,
    'prior_evidence_on_disk':(CAMPAIGN_DIR/'cells.jsonl').is_file() if CAMPAIGN_DIR.is_dir() else False,
    'methods':CHART_METHODS,
   }
  lit=[]; lit_src=''; lit_err=None
  lit_path=ROOT/'docs'/'GODE_SUNDER_1993_TABLE2.json'
  try:
   lit_doc=json.loads(lit_path.read_text())
   means=lit_doc.get('means') or []
   lit=[float(x) for x in means]
   lit_src=str(lit_doc.get('source') or lit_doc.get('citation') or 'archival literature')
   if not lit:
    lit_err='archival fixture has empty means'
  except Exception as e:
   lit_err=f'archival fixture missing/unreadable: {e}'
  cells_path=ROOT/'results'/'dv026_ollama_campaign'/'cells.jsonl'
  summary_path=ROOT/'results'/'dv026_ollama_campaign'/'summary.json'
  by={}
  pipe={'attempted':0,'parse_ok':0,'action_valid':0,'market_action_ok':0,'market_accepted':0,'replay_ok':0,'cell_ok':0}
  live_eff_sum=0.0
  live_eff_n=0
  if summary_path.is_file():
   try:
    summ=json.loads(summary_path.read_text())
    if not _current_campaign_summary(summ): summ={}
    n_seeds=int(summ.get('n_seeds') or n_seeds)
    if layer_a_cda is None and summ.get('mean_efficiency_cda') is not None:
     layer_a_cda=float(summ.get('mean_efficiency_cda'))
   except Exception:
    pass
  if cells_path.is_file():
   for line in cells_path.read_text().splitlines():
    line=line.strip()
    if not line: continue
    try: row=json.loads(line)
    except Exception: continue
    if not isinstance(row,dict) or row.get('skipped'): continue
    pipe['attempted']+=1
    m=str(row.get('model') or 'unknown')
    slot=by.setdefault(m,{'model':m,'attempted':0,'cell_ok':0,'eff_sum':0.0,'eff_n':0})
    slot['attempted']+=1
    if row.get('parse_ok'): pipe['parse_ok']+=1
    if row.get('action_valid'): pipe['action_valid']+=1
    if row.get('market_action_ok'): pipe['market_action_ok']+=1
    if row.get('market_accepted'): pipe['market_accepted']+=1
    if row.get('replay_ok') or row.get('cross_event_valid'): pipe['replay_ok']+=1
    te=None
    try:
     te=float(row.get('treatment_efficiency'))
    except (TypeError,ValueError):
     te=None
    if row.get('cell_ok'):
     pipe['cell_ok']+=1; slot['cell_ok']+=1
     if te is not None:
      slot['eff_sum']+=te; slot['eff_n']+=1
      live_eff_sum+=te; live_eff_n+=1
  by_model=[]
  for m,slot in sorted(by.items(), key=lambda kv:(-kv[1]['cell_ok'], kv[0])):
   mean_eff=(slot['eff_sum']/slot['eff_n']) if slot['eff_n'] else None
   by_model.append({
    'model':slot['model'],'attempted':slot['attempted'],'cell_ok':slot['cell_ok'],
    'mean_treatment_efficiency':mean_eff,'n_seeds':n_seeds
   })
  # Live η = LLM cell_ok treatment_efficiency only — never Layer A CDA.
  live_mean=(live_eff_sum/live_eff_n) if live_eff_n else None
  human_ref={
   'literature_means':lit,
   'literature_mean':(sum(lit)/len(lit)) if lit else None,
   'live_mean_efficiency':live_mean,
   'live_mean_source':('cells.jsonl treatment_efficiency' if live_mean is not None else None),
   'live_mean_n':live_eff_n,
   'source':lit_src or None,
   'kind':'archival_literature' if lit else None,
  }
  if lit_err: human_ref['literature_error']=lit_err
  return {
   'by_model':by_model,
   'human_ref':human_ref,
   'layer_a_mean_efficiency_cda':layer_a_cda,
   'pipeline':pipe,
   'n_seeds':n_seeds,
   'partial':running,
   'campaign_display_reset':False,
   'methods':CHART_METHODS,
  }
 def campaign_analysis(self):
  """Measured campaign analysis: distributions, model comparisons, paired deltas, CIs and observed fingerprints."""
  import math, statistics
  with self.lock:
   display_reset=bool(self.campaign_display_reset)
   running=bool(self.campaign.running)
  if display_reset:
   return {
    'cells':0,'models':[],'overall':{},'fingerprints':[],
    'ci_method':'95% normal-approximation CI over observed seed cells; descriptive, not equivalence testing',
    'source':'persisted cells.jsonl + cell_*.jsonl',
    'partial':False,
    'campaign_display_reset':True,
    'prior_evidence_on_disk':(CAMPAIGN_DIR/'cells.jsonl').is_file() if CAMPAIGN_DIR.is_dir() else False,
    'methods':ANALYSIS_METHODS,
   }
  cells_path=CAMPAIGN_DIR/'cells.jsonl'
  rows=[]
  if cells_path.is_file():
   for line in cells_path.read_text().splitlines():
    try: r=json.loads(line)
    except Exception: continue
    if isinstance(r,dict) and not r.get('skipped'): rows.append(r)
  def nums(rs,key):
   out=[]
   for r in rs:
    try:
     v=float(r.get(key))
     if math.isfinite(v): out.append(v)
    except (TypeError,ValueError): pass
   return out
  def stats(v):
   if not v:return {'n':0,'mean':None,'sd':None,'ci95_low':None,'ci95_high':None,'min':None,'max':None,'values':[]}
   m=sum(v)/len(v); sd=statistics.stdev(v) if len(v)>1 else 0.0; half=1.96*sd/math.sqrt(len(v)) if len(v)>1 else 0.0
   return {'n':len(v),'mean':m,'sd':sd,'ci95_low':m-half,'ci95_high':m+half,'min':min(v),'max':max(v),'values':v}
  groups={}
  for r in rows: groups.setdefault(str(r.get('model') or 'unknown'),[]).append(r)
  models=[]
  for model,rs in sorted(groups.items()):
   models.append({'model':model,'cells':len(rs),'efficiency':stats(nums(rs,'treatment_efficiency')),
    'delta_efficiency':stats(nums(rs,'delta_efficiency')),'delta_surplus':stats(nums(rs,'delta_surplus')),
    'delta_mean_price':stats(nums(rs,'delta_mean_price')),
    'parse_rate':sum(bool(x.get('parse_ok')) for x in rs)/len(rs),
    'valid_rate':sum(bool(x.get('action_valid')) for x in rs)/len(rs),
    'acceptance_rate':sum(bool(x.get('market_accepted')) for x in rs)/len(rs),
    'execution_rate':sum(bool(x.get('execution_observed')) for x in rs)/len(rs)})
  # Fingerprints are computed only from persisted observed turns, never invented from aggregate cells.
  fp={}
  for path in CAMPAIGN_DIR.glob('cell_*.jsonl') if CAMPAIGN_DIR.is_dir() else []:
   try:
    turns=[json.loads(x) for x in path.read_text().splitlines() if x.strip()]
   except Exception: continue
   for t in turns:
    md=t.get('model') or {}; model=str(md.get('name') if isinstance(md,dict) else md or 'unknown')
    a=fp.setdefault(model,{'turns':0,'valid':0,'quotes':0,'violations':0,'aggressiveness_sum':0.0,'fills':0,'payoff_max':0.0,'prices':[]})
    a['turns']+=1
    if (t.get('parse') or {}).get('success') and (t.get('action_validation') or {}).get('valid'): a['valid']+=1
    pa=t.get('parsed_action') or {}; action=pa.get('action'); px=pa.get('price'); before=t.get('agent_state_before') or {}; role=before.get('role'); reservation=before.get('private_value')
    try: px=float(px); reservation=float(reservation)
    except (TypeError,ValueError): px=reservation=None
    if action in ('BUY','SELL') and px is not None and reservation is not None:
     a['quotes']+=1; a['prices'].append(px); den=max(1.0,abs(reservation))
     if action=='BUY': a['aggressiveness_sum']+=px/den; a['violations']+=int(px>reservation+1e-9)
     else: a['aggressiveness_sum']+=(2.0-px/den); a['violations']+=int(px<reservation-1e-9)
    sub=t.get('submission') or {}; a['fills']+=int((sub.get('filled_quantity') or 0)>0)
    try:a['payoff_max']=max(a['payoff_max'],float((t.get('agent_state_after') or {}).get('realized_payoff') or 0))
    except Exception:pass
  fingerprints=[]
  for model,a in sorted(fp.items()):
   prices=a.pop('prices'); mean=sum(prices)/len(prices) if prices else None
   sd=statistics.pstdev(prices) if len(prices)>1 else 0.0
   fingerprints.append({'model':model,'turns':a['turns'],'aggressiveness':a['aggressiveness_sum']/a['quotes'] if a['quotes'] else None,
    'reservation_violation_rate':a['violations']/a['quotes'] if a['quotes'] else None,'trade_frequency':a['fills']/a['turns'] if a['turns'] else None,
    'action_validity':a['valid']/a['turns'] if a['turns'] else None,'response_consistency':(max(0.0,min(1.0,1.0-sd/max(1.0,abs(mean))))) if mean is not None else None,
    'realized_payoff':a['payoff_max']})
  return {'cells':len(rows),'models':models,'overall':{'efficiency':stats(nums(rows,'treatment_efficiency')),'delta_efficiency':stats(nums(rows,'delta_efficiency')),
   'delta_surplus':stats(nums(rows,'delta_surplus')),'delta_mean_price':stats(nums(rows,'delta_mean_price'))},'fingerprints':fingerprints,
   'ci_method':'95% normal-approximation CI over observed seed cells; descriptive, not equivalence testing','source':'persisted cells.jsonl + cell_*.jsonl','partial':running,
   'campaign_display_reset':False,
   'methods':ANALYSIS_METHODS}
 def campaign_cells_index(self):
  """List cell_*.jsonl files with model/seed/turns + cells.jsonl cell_ok join."""
  with self.lock:
   display_reset=bool(self.campaign_display_reset)
   running=bool(self.campaign.running)
   current_model=self.campaign.current_model
   current_seed=self.campaign.current_seed
  if display_reset:
   return {
    'cells':[],'count':0,'running':running,
    'current_model':current_model,'current_seed':current_seed,
    'campaign_display_reset':True,
    'prior_evidence_on_disk':any(CAMPAIGN_DIR.glob('cell_*.jsonl')) if CAMPAIGN_DIR.is_dir() else False,
    'note':'Console was reset; prior cell files remain on disk but are hidden until a new campaign run.',
   }
  hetero_items=_hetero_cells_index()
  pop=''
  with self.lock:
   pop=str(self.campaign.population_design or '')
  if pop=='heterogeneous_multi_llm' or (hetero_items and not (CAMPAIGN_DIR/'cells.jsonl').is_file()):
   current_model=None; current_seed=None
   prog=_hetero_progress()
   if prog and prog.get('running'):
    r=prog['running']
    current_model=_hetero_model_label(r.get('design'), r.get('repeat'), r.get('index'))
    current_seed=r.get('seed')
   elif hetero_items:
    current_model=hetero_items[-1].get('model')
    current_seed=hetero_items[-1].get('seed')
   return {
    'cells':hetero_items,
    'count':len(hetero_items),
    'running':running,
    'current_model':current_model,
    'current_seed':current_seed,
    'campaign_display_reset':False,
    'source':'hetero_cell_evidence',
    'kind':'hetero',
   }
  camp=CAMPAIGN_DIR
  # Prefer cells.jsonl as the campaign index. Listing every historic cell_*.jsonl
  # floods the UI with unmatched traces (null cell_ok → misleading red CHECK).
  index_rows=[]
  cells_path=camp/'cells.jsonl'
  if cells_path.is_file():
   for line in cells_path.read_text().splitlines():
    line=line.strip()
    if not line: continue
    try: row=json.loads(line)
    except Exception: continue
    if not isinstance(row,dict) or row.get('skipped'): continue
    index_rows.append(row)
  items=[]
  if index_rows:
   for row in index_rows:
    model=str(row.get('model') or '')
    seed=int(row.get('seed') or 0)
    slug=_model_cell_slug(model)
    path=camp/f'cell_{slug}_{seed}.jsonl'
    turns=0
    if path.is_file():
     try:
      turns=len(list(_iter_jsonl_records(path.read_text(errors='replace'))))
     except Exception:
      turns=0
    items.append({
     'model':model,
     'seed':seed,
     'slug':slug,
     'file':path.name if path.is_file() else None,
     'turns':turns,
     'cell_ok':bool(row.get('cell_ok')),
     'control_efficiency':row.get('control_efficiency'),
     'treatment_efficiency':row.get('treatment_efficiency'),
     'delta_efficiency':row.get('delta_efficiency'),
     'control_surplus':row.get('control_surplus'),
     'treatment_surplus':row.get('treatment_surplus'),
     'delta_surplus':row.get('delta_surplus'),
     'control_mean_price':row.get('control_mean_price'),
     'treatment_mean_price':row.get('treatment_mean_price'),
     'delta_mean_price':row.get('delta_mean_price'),
     'latency_ms':row.get('latency_ms'),
     'units_filled':row.get('units_filled'),
     'error':row.get('error') or '',
     'indexed':True,
    })
  elif camp.is_dir():
   for path in sorted(camp.glob('cell_*.jsonl')):
    parsed=_parse_cell_filename(path.name)
    if not parsed: continue
    slug, seed=parsed
    model=None
    turns=0
    try:
     records=list(_iter_jsonl_records(path.read_text(errors='replace')))
     turns=len(records)
     if records:
      md=records[0].get('model')
      if isinstance(md,dict):
       model=md.get('name') or md.get('model')
      elif isinstance(md,str):
       model=md
    except Exception:
     pass
    if not model:
     model=slug.replace('_', ':', 1) if '_' in slug else slug
    items.append({
     'model':model,
     'seed':seed,
     'slug':slug,
     'file':path.name,
     'turns':turns,
     'cell_ok':None,
     'control_efficiency':None,
     'treatment_efficiency':None,
     'delta_efficiency':None,
     'control_surplus':None,
     'treatment_surplus':None,
     'delta_surplus':None,
     'control_mean_price':None,
     'treatment_mean_price':None,
     'delta_mean_price':None,
     'latency_ms':None,
     'units_filled':None,
     'error':'',
     'indexed':False,
    })
  return {
   'cells':items,
   'count':len(items),
   'running':running,
   'current_model':current_model,
   'current_seed':current_seed,
   'campaign_display_reset':False,
   'source':('cells.jsonl' if index_rows else 'cell_files'),
  }
 def campaign_cell_detail(self, model:str, seed:int):
  """Load one cell_*.jsonl with per-turn bid I/O."""
  hetero=_hetero_cell_detail(model, seed)
  if hetero and not hetero.get('error'):
   return hetero
  try:
   path=_safe_campaign_cell_path(model, seed)
  except ValueError as e:
   if hetero:
    return hetero
   return {'error':str(e)}
  if not path.is_file():
   # Retry via index match if slug guess wrong
   idx=self.campaign_cells_index()
   match=None
   for c in idx.get('cells') or []:
    if str(c.get('model'))==str(model) and int(c.get('seed') or -1)==int(seed):
     match=c; break
   if match:
    try:
     path=_safe_campaign_cell_path(match['model'], match['seed'])
    except ValueError:
     path=None
   if not path or not path.is_file():
    return {'error':f'cell file not found for model={model} seed={seed}'}
  turns=[]
  try:
   for i,row in enumerate(_iter_jsonl_records(path.read_text(errors='replace'))):
    pa=row.get('parsed_action') if isinstance(row.get('parsed_action'),dict) else {}
    sub=row.get('submission') if isinstance(row.get('submission'),dict) else {}
    turns.append({
     'i':i,
     'time':row.get('time'),
     'agent_id':row.get('agent_id'),
     'latency_ms':row.get('latency_ms'),
     'prompt_tokens':row.get('prompt_tokens'),
     'completion_tokens':row.get('completion_tokens'),
     'parsed_action':pa,
     'action':pa.get('action'),
     'price':pa.get('price'),
     'quantity':pa.get('quantity'),
     'asset':pa.get('asset'),
     'submission':sub,
     'accepted':sub.get('accepted'),
     'filled_quantity':sub.get('filled_quantity'),
     'parse':row.get('parse'),
     'action_validation':row.get('action_validation'),
     'canonical_request':row.get('canonical_request'),
     'raw_provider_response':row.get('raw_provider_response'),
     'market_state':row.get('market_state'),
     'agent_state_before':row.get('agent_state_before'),
     'agent_state_after':row.get('agent_state_after'),
     'model':row.get('model'),
     'request_id':row.get('request_id'),
     'run_id':row.get('run_id'),
     'seed':row.get('seed', seed),
     'inference_config':row.get('inference_config'),
     'adapter_version':row.get('adapter_version'),
     'audit_metadata':{
      'run_id':row.get('run_id'), 'request_id':row.get('request_id'),
      'provider':(row.get('model') or {}).get('provider') if isinstance(row.get('model'),dict) else None,
      'model':(row.get('model') or {}).get('name') if isinstance(row.get('model'),dict) else row.get('model'),
      'version':(row.get('model') or {}).get('version') if isinstance(row.get('model'),dict) else None,
      'experiment_seed':row.get('seed', seed), 'inference_config':row.get('inference_config'),
      'adapter_version':row.get('adapter_version'),
     },
    })
  except Exception as e:
   return {'error':f'failed to read cell file: {e}'}
  meta=None
  cells_path=CAMPAIGN_DIR/'cells.jsonl'
  if cells_path.is_file():
   for line in cells_path.read_text().splitlines():
    line=line.strip()
    if not line: continue
    try: row=json.loads(line)
    except Exception: continue
    if not isinstance(row,dict): continue
    if str(row.get('model') or '')==str(model) and int(row.get('seed') or -1)==int(seed):
     meta=row; break
  return {
   'model':model,
   'seed':int(seed),
   'file':path.name,
   'turns':turns,
   'turn_count':len(turns),
   'cell':meta,
  }
 def proposal_narrative(self):
  """Operational what’s-happening summary from best available live evidence."""
  fixture_path=ROOT/'docs'/'DV026_PROPOSAL_EXPECTATIONS.json'
  try:
   fixture=json.loads(fixture_path.read_text())
  except Exception as e:
   return {'error':f'missing expectations fixture: {e}','text':'','items':[],'summary':''}
  charts=self.campaign_charts()
  summary_path=ROOT/'results'/'dv026_ollama_campaign'/'summary.json'
  summ={}
  if summary_path.is_file():
   try:
    summ=json.loads(summary_path.read_text())
    if not _current_campaign_summary(summ): summ={}
   except Exception: summ={}
  with self.lock:
   self._hydrate_campaign_from_disk_locked()
   c=self.campaign
   running=bool(c.running)
   camp={}
   if isinstance(c.result,dict):
    camp=c.result.get('campaign') if isinstance(c.result.get('campaign'),dict) else c.result
   checks=list(c.readiness_checks or [])
   if not checks and not running:
    checks=list(camp.get('checks') or summ.get('checks') or [])
   if running:
    scope='not_ready'
    darpa=False
   else:
    scope=str(c.scope or camp.get('scope') or summ.get('scope') or 'not_ready')
    if scope=='running':
     scope='not_ready'
    darpa=bool(c.darpa_claim_ready if c.result is not None else
               (camp.get('darpa_claim_ready') if camp else summ.get('darpa_claim_ready')))
   # Respect Reset: do not score Layer A MET from on-disk summary alone.
   layer_a_pass=False
   if isinstance(self.layer_a_cache,dict) and (self.layer_a_cache.get('layer_a_pass') or self.layer_a_cache.get('pass')):
    layer_a_pass=True
   elif not self.campaign_display_reset:
    layer_a_pass=bool(camp.get('layer_a_pass') or summ.get('layer_a_pass'))
   last=self.dv026_last if isinstance(self.dv026_last,dict) else {}
   la=last.get('layer_a') if isinstance(last.get('layer_a'),dict) else last
   if isinstance(la,dict) and (la.get('layer_a_pass') or la.get('pass')):
    layer_a_pass=True
   n_seeds=int(c.n_seeds or charts.get('n_seeds') or summ.get('n_seeds') or 20)
   cur_model=c.current_model
   cur_seed=c.current_seed
   done=int(c.done or 0)
   total=int(c.total or 0)
   eta_sec=None
   if c.running and c.done>0 and c.total>c.done and c.started_at:
    eta_sec=round(((time.time()-c.started_at)/c.done)*(c.total-c.done),1)
  by_model=charts.get('by_model') or []
  mok=sum(1 for b in by_model if int(b.get('cell_ok') or 0)>=1)
  full_cov=sum(1 for b in by_model if int(b.get('cell_ok') or 0)>=n_seeds)
  if not running and mok==0:
   mok=int(camp.get('distinct_live_models_ok') or summ.get('distinct_live_models_ok') or 0)
  incomplete_models=[]
  for b in by_model:
   ok=int(b.get('cell_ok') or 0)
   if ok<n_seeds:
    incomplete_models.append({'model':b.get('model'),'cell_ok':ok,'n_seeds':n_seeds})
  incomplete_models.sort(key=lambda x:(x['cell_ok'], str(x['model'] or '')))
  incomplete_str=', '.join(f"{m['model']} ({m['cell_ok']}/{n_seeds})" for m in incomplete_models[:6])
  if len(incomplete_models)>6:
   incomplete_str+=f', +{len(incomplete_models)-6} more'
  pipe=charts.get('pipeline') or {}
  attempted=int(pipe.get('attempted') or 0)
  parse_ok=int(pipe.get('parse_ok') or 0)
  action_valid=int(pipe.get('action_valid') or 0)
  replay_ok=int(pipe.get('replay_ok') or 0)
  mkt_ok=int(pipe.get('market_action_ok') or 0)
  mkt_acc=int(pipe.get('market_accepted') or 0)
  cell_ok_n=int(pipe.get('cell_ok') or 0)
  iface_num=min(parse_ok, action_valid) if attempted else 0
  iface_rate=(iface_num/attempted) if attempted else None
  prov_rate=(replay_ok/attempted) if attempted else None
  mkt_rate=(mkt_ok/attempted) if attempted else None
  acc_rate=(mkt_acc/mkt_ok) if mkt_ok else None
  href_human=charts.get('human_ref') or {}
  live_eta=href_human.get('live_mean_efficiency')
  live_eta_n=href_human.get('live_mean_n')
  lit_means=list(href_human.get('literature_means') or [])
  lit_mean=href_human.get('literature_mean')
  lit_min=min(lit_means) if lit_means else None
  lit_max=max(lit_means) if lit_means else None
  def check_pass(cid):
   for ch in checks:
    if isinstance(ch,dict) and ch.get('id')==cid:
     return bool(ch.get('pass')), str(ch.get('detail') or '')
   return None, ''
  def check_detail(cid):
   return check_pass(cid)[1]
  href_p,href_d=check_pass('human_ref')
  if href_p is None and summ.get('human_ref_gate') is not None:
   href_p=bool(summ.get('human_ref_gate')); href_d=''
  iface_d=prov_d=mkt_d=acc_d=live_d=cov_d=layer_d=''
  if running:
   href_p=None; href_d=''
   prov_p=(prov_rate is not None and prov_rate>=0.95)
   iface_p=(iface_rate is not None and iface_rate>=0.90)
   mkt_p=(mkt_rate is not None and mkt_rate>=0.80)
   acc_p=(acc_rate is not None and acc_rate>=0.80)
  else:
   if href_p is None: href_p=False
   prov_p,prov_d=check_pass('provenance')
   if prov_p is None and attempted:
    prov_p=prov_rate>=0.95; prov_d=''
   iface_p,iface_d=check_pass('interface_health')
   if iface_p is None and iface_rate is not None:
    iface_p=iface_rate>=0.90; iface_d=''
   mkt_p,mkt_d=check_pass('market_action')
   if mkt_p is None and mkt_rate is not None:
    mkt_p=mkt_rate>=0.80
   acc_p,acc_d=check_pass('market_acceptance')
   if acc_p is None and acc_rate is not None:
    acc_p=acc_rate>=0.80
   live_d=check_detail('live_count'); cov_d=check_detail('coverage'); layer_d=check_detail('layer_a')
   if not iface_d: iface_d=check_detail('interface_health')
   if not prov_d: prov_d=check_detail('provenance')
   if not href_d: href_d=check_detail('human_ref')
  layer_present=layer_a_pass or (summ.get('mean_efficiency_cda') is not None) or (camp.get('mean_efficiency_cda') is not None)
  cda=summ.get('mean_efficiency_cda', camp.get('mean_efficiency_cda'))
  sealed=summ.get('mean_efficiency_sealed', camp.get('mean_efficiency_sealed'))
  def pct(v):
   try: return f'{float(v):.1f}%'
   except (TypeError,ValueError): return '—'
  def rate_pct(r):
   if r is None: return '—'
   return f'{100.0*float(r):.0f}%'
  def fmt_eta(sec):
   if sec is None: return '—'
   try: sec=float(sec)
   except (TypeError,ValueError): return '—'
   if sec<60: return f'{int(sec)}s'
   return f'{int(sec//60)}m {int(sec%60)}s'

  def score(gate):
   if gate=='layer_a_present':
    if layer_a_pass:
     detail=layer_d or f'means CDA {pct(cda)} / sealed {pct(sealed)}'
     return 'MET', f'Market engine qualification finished (CDA + sealed-bid exercised; {detail}).', None
    if layer_present:
     return 'PARTIAL', 'Market metrics exist but qualification did not pass.', 'Re-run Layer A until every trial clears above 90%.'
    return 'NOT_YET', 'Market engine qualification has not been run yet.', 'Run Layer A market qualification before the live campaign.'
   if gate=='layer_a_qualified':
    if layer_a_pass:
     return 'MET', layer_d or f'Every Layer A trial cleared >90%. Means: CDA {pct(cda)}, sealed {pct(sealed)}.', None
    return 'NOT_YET', 'Market efficiency gate is not passed yet.', 'Every Layer A trial must clear above 90% on CDA and sealed-bid.'
   if gate=='interface_health':
    happening=(f'Of {attempted} live cells, {rate_pct(iface_rate)} parse+validate (need ≥90%); '
               f'{rate_pct(mkt_rate)} non-HOLD (need ≥80%); acceptance {rate_pct(acc_rate)} (need ≥80%).')
    if not running and iface_d:
     happening=f'{iface_d}. Non-HOLD {rate_pct(mkt_rate)}; acceptance {rate_pct(acc_rate)}'+(f'; market_action: {mkt_d}' if mkt_d else '')+(f'; acceptance: {acc_d}' if acc_d else '')+'.'
    if running:
     okish=bool(iface_p) and (mkt_p is None or mkt_p) and (acc_p is None or acc_p)
     if okish:
      return 'PROVISIONAL', happening, 'Final interface/execution score waits until the campaign finishes.'
     if attempted:
      return 'PARTIAL', happening, 'More cells still need valid non-HOLD accepted actions; final score at campaign end.'
     return 'NOT_YET', 'No live agent cells yet — campaign has not produced parseable actions.', 'Continue the live campaign.'
    if iface_p:
     return 'MET', happening, None
    if attempted:
     return 'PARTIAL', happening, 'Improve action-schema / non-HOLD compliance on failing cells.'
    return 'NOT_YET', 'No live agent cells yet.', 'Start a live Ollama campaign.'
   if gate=='ten_models_covered':
    incomplete=max(0,10-full_cov)
    if not running and (live_d or cov_d):
     happening=f'{live_d or ("distinct_ok="+str(mok))} · {cov_d or ("full_coverage="+str(full_cov))}'
     happening+=f' · Incomplete: {incomplete_str}' if incomplete_str else f' · All tracked models at {n_seeds}/{n_seeds}.'
    else:
     happening=f'{full_cov} models finished {n_seeds}/{n_seeds}; {mok} with ≥1 accepted cell'
     happening+=f'. Incomplete: {incomplete_str}' if incomplete_str else '.'
    if running:
     if mok>=10 and full_cov>=10:
      return 'PROVISIONAL', happening, 'Coverage looks complete so far; gate locks when the campaign finishes.'
     if mok>=1:
      return 'PARTIAL', happening, f'Need {incomplete} more model{"s" if incomplete!=1 else ""} with full {n_seeds}-seed coverage.'
     return 'NOT_YET', 'No successful live model cells yet.', f'Exercise at least 10 models across {n_seeds} seeds each.'
    if mok>=10 and full_cov>=10:
     return 'MET', happening, None
    if mok>=1:
     return 'PARTIAL', happening, f'Need {incomplete} more model{"s" if incomplete!=1 else ""} with full {n_seeds}-seed coverage.'
    return 'NOT_YET', 'No successful live model cells yet.', f'Exercise at least 10 models across {n_seeds} seeds each.'
   if gate=='human_ref':
    if lit_min is not None and lit_max is not None and lit_mean is not None:
     band=f'archival Table 2 band {lit_min:.1f}–{lit_max:.1f}% (mean {float(lit_mean):.1f}%)'
    elif lit_min is not None and lit_max is not None:
     band=f'archival Table 2 band {lit_min:.1f}–{lit_max:.1f}%'
    else:
     band='archival Table 2 band unavailable'
    live_bit=(f'LLM cell mean η={float(live_eta):.1f}% (n={live_eta_n})' if live_eta is not None else 'LLM cell mean η not available yet')
    if running:
     return 'PROVISIONAL', f'{live_bit} vs {band}. HumanComparison gate waits until this campaign finishes.', 'Wait for campaign finish for the final human-ref score.'
    if href_p is True:
     return 'MET', f'{(href_d or "HumanComparison DESCRIPTIVE passed")}. {live_bit} vs {band}.', None
    return 'NOT_YET', f'HumanComparison DESCRIPTIVE has not passed. {live_bit} vs {band}.', 'Need LiveProvider observations matched to archival literature.'
   if gate=='provenance':
    if not running and prov_d:
     happening=f'{prov_d}. cell_ok={cell_ok_n}/{attempted}.'
    elif attempted:
     happening=f'Evidence chain retained on {replay_ok}/{attempted} cells ({rate_pct(prov_rate)}; need ≥95%); cell_ok={cell_ok_n}.'
    else:
     happening='No live cells yet — no observation→action→outcome chain recorded.'
    if running:
     if prov_p:
      return 'PROVISIONAL', happening, 'Provenance looks healthy so far; final score waits until the campaign finishes.'
     if attempted:
      return 'PARTIAL', happening, 'Raise replay/provenance retention; final score at campaign end.'
     return 'NOT_YET', happening, 'Continue the campaign so cells retain full evidence chains.'
    if prov_p:
     return 'MET', happening, None
    if attempted:
     return 'PARTIAL', happening, 'Raise replay/provenance retention to at least 95% of attempted cells.'
    return 'NOT_YET', happening, 'Run live cells so observation → response → action → outcome is retained.'
   if gate=='claim_boundary':
    if running:
     return 'MET', 'Claim boundary held: local Ollama evidence only; Phase II and FAQ 31 accuracy not claimed; PoC gate not decided until the campaign ends.', None
    if scope=='local_ollama_poc' and darpa:
     return 'MET', 'Claim boundary held: READY only under scope=local_ollama_poc; Phase II and FAQ 31 remain unclaimed.', None
    if attempted or layer_present:
     return 'MET', 'Claim boundary held: evidence is local executable only; commercial multi-provider readiness is not claimed.', None
    return 'PARTIAL', 'Boundary text is ready, but little live evidence has been collected yet.', 'Collect Layer A and/or live campaign evidence.'
   return 'NOT_YET', f'Unknown gate {gate}.', None

  items=[]; counts={'MET':0,'PROVISIONAL':0,'PARTIAL':0,'NOT_YET':0}
  for exp in fixture.get('expectations') or []:
   st, happening, nxt=score(exp.get('gate'))
   counts[st]=counts.get(st,0)+1
   items.append({
    'id':exp.get('id'),'milestone':exp.get('milestone'),'title':exp.get('title'),
    'text':exp.get('text'),'gate':exp.get('gate'),'status':st,
    'happening':happening,'next':nxt,'evidence':happening,'still_needed':nxt
   })

  camp_label='IN PROGRESS' if running else ('READY' if darpa and scope=='local_ollama_poc' else 'NOT READY')
  if running:
   la_bit=(f'market engine already qualified (CDA {pct(cda)}, sealed {pct(sealed)})' if layer_a_pass
           else 'market engine not yet qualified')
   now_bit=''
   if cur_model:
    now_bit=f'Now running {cur_model}'+(f' seed {cur_seed}' if cur_seed is not None else '')
    if total: now_bit+=f' · {done}/{total} cells'
    if eta_sec is not None: now_bit+=f' · ETA {fmt_eta(eta_sec)}'
    now_bit+=' · '
   gap=f' Incomplete: {incomplete_str}.' if incomplete_str else ''
   summary=(f'{now_bit}Live campaign in progress · {attempted} cells in cells.jsonl · '
            f'{full_cov}/10 models finished all {n_seeds} seeds · {la_bit}.{gap} '
            f'Gate scores finalize when the campaign ends.')
  elif darpa and scope=='local_ollama_poc':
   summary=(f'Campaign finished READY under local_ollama_poc · {attempted} cells · '
            f'{full_cov}/10 models at {n_seeds} seeds · Layer A CDA {pct(cda)} / sealed {pct(sealed)}'
            +(f' · LLM η {pct(live_eta)} (n={live_eta_n})' if live_eta is not None else '')+
            '. Local PoC claim only — not commercial multi-provider readiness.')
  elif attempted or layer_a_pass:
   gap=f' Incomplete: {incomplete_str}.' if incomplete_str else ''
   summary=(f'Campaign not READY (scope={scope}) · {attempted} cells attempted · '
            f'{full_cov}/10 models finished all {n_seeds} seeds · '
            f'Layer A {"qualified" if layer_a_pass else "not qualified"} '
            f'(CDA {pct(cda)}, sealed {pct(sealed)}).{gap}')
  else:
   summary='Nothing measured yet: run Layer A market qualification, then start the live Ollama campaign.'

  lines=['DV026 Phase I — what’s happening (local evidence)','',f'SUMMARY: {summary}',
         f"Snapshot: campaign {camp_label} · {counts['MET']} MET · {counts['PROVISIONAL']} PROVISIONAL · {counts['PARTIAL']} PARTIAL · {counts['NOT_YET']} NOT YET",
         'MET = finished gate · PROVISIONAL = looks good mid-run (not final)','']
  for it in items:
   title=it.get('title') or it.get('id') or 'Item'
   ms=it.get('milestone') or ''
   ms_tag=f' ({ms})' if ms and ms!='cross-cutting' else ''
   lines.append(f"{it['status']}  · {title}{ms_tag}")
   lines.append(f"  {it.get('happening')}")
   if it.get('next'): lines.append(f"  Next: {it['next']}")
   lines.append('')
  text='\n'.join(lines).rstrip()+'\n'
  return {
   'text':text,'summary':summary,'items':items,
   'meta':{
    'darpa_claim_ready':darpa,'scope':scope,'running':running,'campaign_label':camp_label,
    'distinct_ok':mok,'coverage':full_cov,'n_seeds':n_seeds,'attempted':attempted,
    'layer_a_pass':layer_a_pass,'counts':counts,
    'mean_efficiency_cda':cda,'mean_efficiency_sealed':sealed,
    'current_model':cur_model,'current_seed':cur_seed,'done':done,'total':total,
    'eta_sec':eta_sec,'incomplete_models':incomplete_models,
    'live_mean_efficiency':live_eta,'live_mean_n':live_eta_n,
    'literature_band':({'min':lit_min,'max':lit_max,'mean':lit_mean} if lit_min is not None else None),
    'rates':{'interface':iface_rate,'provenance':prov_rate,'market_action':mkt_rate,'market_acceptance':acc_rate}
   }
  }
 def _campaign_worker(self):
  exe=find_exe('dv026-workbench-runner')
  env=os.environ.copy()
  env.setdefault('COAGENTICS_LLM_PROVIDER','ollama')
  env.setdefault('COAGENTICS_LLM_BASE_URL','http://127.0.0.1:11434/v1')
  ex=_load_expectations()
  if ex.get('status')=='frozen' and ex.get('manifest_sha256'):
   env['COAGENTICS_EXPECTATIONS_SHA256']=str(ex['manifest_sha256'])
   env['COAGENTICS_EXPECTATIONS_PATH']=str(EXPECTATIONS_PATH)
   env['COAGENTICS_EXPECTATIONS_JSON']=_canonical_json(ex)
  env['COAGENTICS_POPULATION_DESIGN']=str(getattr(self.campaign,'population_design','reference_counterparty'))
  env['COAGENTICS_ACTIVATION_DESIGN']=str(getattr(self.campaign,'activation_design','sequential_interaction'))
  env['COAGENTICS_INFERENCE_REPEATS']=str(getattr(self.campaign,'inference_repeats',3))
  resume_dir=getattr(self.campaign,'_resume_run_dir',None)
  if resume_dir:
   env['COAGENTICS_RESUME_RUN_DIR']=str(resume_dir)
   env['COAGENTICS_INCREMENTAL_EVIDENCE']='1'
  with self.lock:
   c=self.campaign
   args=[str(exe),('hetero-campaign' if c.population_design=='heterogeneous_multi_llm' else 'ollama-campaign'),str(c.base_seed),str(c.n_seeds)]
   if c.smoke: args.append('--smoke')
  try:
   proc=subprocess.Popen(args,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,cwd=str(ROOT),env=env)
   with self.lock:
    self.campaign.proc=proc
   if c.population_design=='heterogeneous_multi_llm':
    try:
     resume=env.get('COAGENTICS_RESUME_RUN_DIR')
     _write_campaign_session(proc,exe,args,run_dir=Path(resume) if resume else None,extra={'resume':bool(resume)})
    except Exception:
     pass
   # Poll cancel while waiting; campaign is one C++ process (progress via summary on finish)
   while True:
    with self.lock:
     if self.campaign.cancel:
      try: proc.kill()
      except Exception: pass
      break
    rc=proc.poll()
    if rc is not None: break
    # Soft progress: ollama cells.jsonl, else hetero_progress.jsonl
    cells=ROOT/'results'/'dv026_ollama_campaign'/'cells.jsonl'
    if not cells.is_file():
     prog=_hetero_progress()
     if prog and (prog['done'] or prog.get('running')):
      with self.lock:
       self.campaign.done=prog['done']
       self.campaign.cells_ok=prog['done']
       self.campaign.pass_count=prog['done']
       if prog['total']:
        self.campaign.total=max(int(self.campaign.total or 0), int(prog['total']))
       self.campaign.scope='running'
       cur=prog.get('running')
       if not cur and prog['completed']:
        cur=prog['completed'][max(prog['completed'])]
       if cur:
        self.campaign.current_model=_hetero_model_label(cur.get('design'), cur.get('repeat'), cur.get('index'))
        if cur.get('seed') is not None:
         self.campaign.current_seed=int(cur.get('seed'))
     time.sleep(1.0)
     continue
    cells=ROOT/'results'/'dv026_ollama_campaign'/'cells.jsonl'
    if cells.is_file():
     try:
      lines=cells.read_text().splitlines()
      n=len(lines)
      latest=json.loads(lines[-1]) if lines else {}
      ok=0
      distinct=set()
      ok_by_model={}
      iface_ok=0; prov_ok=0; mkt_ok=0; mkt_acc=0; attempted=0
      for line in lines:
       try:
        row=json.loads(line)
       except Exception:
        continue
       if not isinstance(row,dict) or row.get('skipped'):
        continue
       attempted+=1
       if row.get('interface_ok') or (row.get('parse_ok') and row.get('action_valid')):
        iface_ok+=1
       if row.get('replay_ok') or row.get('cross_event_valid'):
        prov_ok+=1
       if row.get('market_action_ok'):
        mkt_ok+=1
        if row.get('market_accepted'):
         mkt_acc+=1
       if row.get('cell_ok'):
        ok+=1
        m=row.get('model')
        if m:
         ms=str(m); distinct.add(ms)
         ok_by_model[ms]=ok_by_model.get(ms,0)+1
      N=max(1,int(self.campaign.n_seeds or 20))
      full_cov=sum(1 for v in ok_by_model.values() if v>=N)
      partial=dict(
       attempted=attempted,
       cells_ok=ok,
       distinct_ok=len(distinct),
       models_with_full_coverage=full_cov,
       n_seeds=N,
       provenance_rate=(prov_ok/attempted) if attempted else None,
       interface_rate=(iface_ok/attempted) if attempted else None,
       market_action_rate=(mkt_ok/attempted) if attempted else None,
       market_acceptance_rate=(mkt_acc/mkt_ok) if mkt_ok else None,
       partial=True,
      )
      with self.lock:
       self.campaign.done=n
       self.campaign.cells_ok=ok
       self.campaign.pass_count=ok
       self.campaign.fail_count=max(0,n-ok)
       self.campaign.distinct_live_models_ok=len(distinct)
       self.campaign.scope='running'
       self.campaign.darpa_claim_ready=False
       self.campaign.readiness_checks=[]
       self.campaign.result=None
       self.campaign.gate_partial=partial
       if isinstance(latest,dict):
        self.campaign.current_model=latest.get('model') or latest.get('provider') or self.campaign.current_model
        self.campaign.current_seed=latest.get('seed',self.campaign.current_seed)
     except Exception:
      pass
    time.sleep(1.0)
   out, err = proc.communicate(timeout=5)
   with self.lock:
    c=self.campaign
    c.proc=None
    if c.cancel:
     c.running=False; c.cancelled=True; c.finished_at=time.time()
     c.current_model=None; c.current_seed=None
     c.last_error='cancelled'
     _finish_campaign_session('cancelled')
     return
    if proc.returncode!=0:
     c.running=False; c.cancelled=False; c.finished_at=time.time()
     c.last_error=(err or out or f'exit {proc.returncode}').strip()
     c.current_model=None; c.current_seed=None
     _finish_campaign_session('interrupted', c.last_error)
     return
    data=json.loads(out)
    hetero=data.get('heterogeneous_campaign') if isinstance(data.get('heterogeneous_campaign'),dict) else None
    camp=(data.get('campaign') if isinstance(data.get('campaign'),dict) else data)
    c.result=data
    if hetero is not None:
     cells=hetero.get('cells') if isinstance(hetero.get('cells'),list) else []
     models=set()
     for cell in cells:
      for a in (cell.get('assignment') or []):
       if a.get('model'): models.add(str(a.get('model')))
     c.darpa_claim_ready=False
     c.scope='heterogeneous_experimental_campaign'
     c.available_models=len(models)
     c.cells_ok=len(cells)
     c.distinct_live_models_ok=len(models)
     c.pass_count=len(cells); c.fail_count=0; c.done=len(cells); c.total=len(cells)
     c.readiness_checks=[]
    else:
     c.darpa_claim_ready=bool(camp.get('darpa_claim_ready'))
     c.scope=str(camp.get('scope') or 'not_ready')
     c.available_models=int(camp.get('available_models') or 0)
     c.cells_ok=int(camp.get('cells_ok') or 0)
     c.distinct_live_models_ok=int(camp.get('distinct_live_models_ok') or 0)
     c.pass_count=c.cells_ok
     c.fail_count=max(0, int(camp.get('cells_attempted') or 0)-c.cells_ok)
     c.done=int(camp.get('cells_attempted') or 0)
     c.total=c.done
     c.readiness_checks=camp.get('checks') if isinstance(camp.get('checks'),list) else []
    c.gate_partial=None
    c.running=False; c.finished_at=time.time()
    c.current_model=None; c.current_seed=None
    self.dv026_last=data
    _finish_campaign_session('complete')
  except Exception as e:
   with self.lock:
    c=self.campaign
    c.running=False; c.finished_at=time.time()
    c.last_error=str(e)
    c.current_model=None; c.current_seed=None
   _finish_campaign_session('interrupted', str(e))

AUDIT_PATH = ROOT / 'workbench' / 'audit.jsonl'

LIFECYCLE_ORDER=['DRAFT','DESIGN_REVIEW','REGISTERED','FROZEN','RUNNING','DISCOVERY_COMPLETE','HOLDOUT','REPLICATION','VALIDATION_REVIEW','APPROVED_CLAIM']
LIFECYCLE_TERMINAL={'APPROVED_CLAIM'}

def _lifecycle_default():
 return {'schema_version':LIFECYCLE_SCHEMA,'experiment_id':'DV026','state':'DRAFT','version':1,'history':[]}

def _lifecycle_read():
 try:
  if not LIFECYCLE_PATH.is_file(): return _lifecycle_default()
  d=json.loads(LIFECYCLE_PATH.read_text())
  return d if isinstance(d,dict) else _lifecycle_default()
 except Exception:
  return _lifecycle_default()

def _lifecycle_write(d):
 LIFECYCLE_PATH.parent.mkdir(parents=True,exist_ok=True)
 LIFECYCLE_PATH.write_text(json.dumps(d,indent=2,ensure_ascii=False)+'\n')
 return d

def _lifecycle_derived(doc=None):
 d=_lifecycle_read()
 # Runner truth can advance the lifecycle, but never skip registration/freeze.
 try:
  c=S.campaign_status()
  if c.get('running') and LIFECYCLE_ORDER.index(d.get('state','DRAFT')) < LIFECYCLE_ORDER.index('RUNNING'):
   d['state']='RUNNING'
  elif c.get('complete') and LIFECYCLE_ORDER.index(d.get('state','DRAFT')) < LIFECYCLE_ORDER.index('DISCOVERY_COMPLETE'):
   d['state']='DISCOVERY_COMPLETE'
 except Exception: pass
 return d

def _lifecycle_transition(target, actor='Researcher', reason=''):
 d=_lifecycle_read(); cur=d.get('state','DRAFT')
 if target not in LIFECYCLE_ORDER: raise ValueError('Invalid lifecycle state')
 if cur==target: return d
 ci=LIFECYCLE_ORDER.index(cur); ti=LIFECYCLE_ORDER.index(target)
 # Runner completion is a governed automatic boundary: RUNNING -> DISCOVERY_COMPLETE.
 if target=='DISCOVERY_COMPLETE' and cur=='RUNNING':
  try:
   if not S.campaign_status().get('complete'): raise ValueError('Campaign must be complete before discovery can be closed.')
  except AttributeError: raise ValueError('Campaign status unavailable; cannot close discovery.')
 elif target=='DRAFT':
  if cur in ('RUNNING','DISCOVERY_COMPLETE','HOLDOUT','REPLICATION','VALIDATION_REVIEW','APPROVED_CLAIM'):
   raise ValueError('Cannot reset a running or completed experiment lifecycle; create a new experiment version.')
 elif target!='DISCOVERY_COMPLETE' and ti!=ci+1:
  raise ValueError(f'Invalid lifecycle transition: {cur} → {target}. Advance one governed state at a time.')
 if target in ('REGISTERED','FROZEN'):
  ex=_load_expectations()
  if target=='REGISTERED' and ex.get('status')!='frozen': raise ValueError('Expectations must be frozen before registration.')
  if target=='FROZEN' and ex.get('status')!='frozen': raise ValueError('Freeze expectations before entering FROZEN.')
 d['state']=target; d['version']=int(d.get('version',1))+1
 d.setdefault('history',[]).append({'timestamp':time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime()),'from':cur,'to':target,'actor':actor,'reason':reason})
 _lifecycle_write(d)
 _audit_write({'timestamp':time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime()),'action':'experiment.lifecycle.transition','detail':f'{cur} → {target}'+((' · '+reason) if reason else ''),'workspace':'DV026 Economic Behavior','actor':actor})
 return d

def _audit_read():
 try:
  if not AUDIT_PATH.is_file(): return []
  out=[]
  for line in AUDIT_PATH.read_text().splitlines():
   try: out.append(json.loads(line))
   except Exception: pass
  return out
 except Exception: return []

def _audit_write(event):
 AUDIT_PATH.parent.mkdir(parents=True,exist_ok=True)
 with AUDIT_PATH.open('a',encoding='utf-8') as f: f.write(json.dumps(event,ensure_ascii=False)+'\n')

S=Session()
class Handler(BaseHTTPRequestHandler):
 def log_message(self,*a):pass
 def handle(self):
  try:
   super().handle()
  except (BrokenPipeError, ConnectionResetError, ConnectionAbortedError, TimeoutError):
   pass
 def finish(self):
  try:
   super().finish()
  except (BrokenPipeError, ConnectionResetError, ConnectionAbortedError, TimeoutError):
   pass
 def reply(self,obj,code=200):
  try:
   raw=json.dumps(obj).encode();self.send_response(code);self.send_header('Content-Type','application/json');self.send_header('Cache-Control','no-store');self.send_header('Content-Length',str(len(raw)));self.end_headers();self.wfile.write(raw)
  except (BrokenPipeError, ConnectionResetError, ConnectionAbortedError, TimeoutError):
   pass
 def _load_design(self):
  if not DESIGN_PATH.is_file():
   return {
    'schema_version':'coagentics-market-lab-design-v1',
    'objectives':['compare_llms','compare_frozen_sequential'],
    'population_design':'heterogeneous_multi_llm',
    'market_mechanism':'continuous_double_auction',
    'activation_design':'both',
    'seeds':20,'repeats_per_cell':3,
    'role_assignment':'balanced_crossover',
    'private_values':'crossover',
    'activation_order':'randomized_balanced'
   }
  try:
   d=json.loads(DESIGN_PATH.read_text())
   return d if isinstance(d,dict) else {}
  except Exception:
   return {}

 def _save_design(self,doc):
  if not isinstance(doc,dict): raise ValueError('Design must be an object')
  DESIGN_DIR.mkdir(parents=True,exist_ok=True)
  DESIGN_PATH.write_text(json.dumps(doc,indent=2,ensure_ascii=False)+'\n')
  return doc

 def do_GET(self):
  url=urllib.parse.urlsplit(self.path)
  if url.path=='/api/llm/status':
   self.reply(_ollama_status());return
  if url.path=='/api/platform/audit':
   self.reply({'events':_audit_read()[-200:]});return
  if url.path=='/api/platform/research-objects':
   payload={name:_ent_read(kind) for name,kind in ENTERPRISE_KINDS.items()}
   self.reply(payload);return
  if url.path.startswith('/api/platform/research-objects/graph'):
   q=urllib.parse.parse_qs(url.query); eid=str(q.get('experiment_id',[''])[0])
   exp=_ent_get('experiment',eid) if eid else None
   if not exp:
    self.reply({'experiment_id':eid,'nodes':[],'edges':[]},404);return
   nodes=[]; edges=[]
   def add(kind,obj):
    if not obj:return
    oid=obj.get('id')
    if not oid:return
    if not any(n.get('id')==oid and n.get('_kind')==kind for n in nodes):
     nodes.append(dict(obj,_kind=kind))
   add('experiment',exp)
   # Explicit experiment links are authoritative. For version, use the experiment's current version when available.
   for kind in ('experiment_version','model','prompt','dataset','evidence_package','review','claim'):
    for obj in _ent_read(kind):
     if obj.get('experiment_id')==eid or obj.get('workspace_id')==exp.get('workspace_id') and kind in ('model','prompt','dataset'):
      add(kind,obj)
   versions=[n for n in nodes if n.get('_kind')=='experiment_version']
   if exp.get('current_version') and not versions:
    versions=[{'id':str(exp['current_version']),'name':'Version '+str(exp['current_version']),'version':exp['current_version'],'status':'registered','_kind':'experiment_version'}]
    nodes.extend(versions)
   # Run is represented by the current DV026 campaign state; it is deliberately not fabricated as a persistent object.
   try:
    snap=S.snapshot(0)
    run={'id':'dv026-live-run','name':'DV026 execution','status':'running' if snap.get('running') else 'available','source':'DV026 campaign'}
   except Exception:
    run={'id':'dv026-live-run','name':'DV026 execution','status':'unavailable','source':'DV026 campaign'}
   add('run',run)
   # Link the research chain using explicit IDs where present, otherwise sensible experiment lineage.
   for n in list(nodes):
    k=n.get('_kind'); oid=n.get('id')
    if k=='experiment_version': edges.append({'from':eid,'to':oid,'relationship':'has_version'})
    elif k in ('model','prompt','dataset'): edges.append({'from':oid,'to':eid,'relationship':'registered_for_experiment'})
    elif k=='run': edges.append({'from':eid,'to':oid,'relationship':'executes'})
    elif k=='evidence_package': edges.append({'from':oid,'to':eid,'relationship':'evidence_for'})
    elif k=='review': edges.append({'from':oid,'to':eid,'relationship':'reviews'})
    elif k=='claim': edges.append({'from':oid,'to':eid,'relationship':'claims_about'})
   for ev in [n for n in nodes if n.get('_kind')=='evidence_package']:
    edges.append({'from':'dv026-live-run','to':ev.get('id'),'relationship':'produces'})
   self.reply({'experiment_id':eid,'nodes':nodes,'edges':edges});return
  m=re.match(r'^/api/platform/research-objects/([^/]+)/([^/]+)$',url.path)
  if m and m.group(1) in ENTERPRISE_KINDS:
   kind=ENTERPRISE_KINDS[m.group(1)]; obj=_ent_get(kind,urllib.parse.unquote(m.group(2)))
   self.reply(obj or {'error':'Not found'},200 if obj else 404);return
  if url.path=='/api/platform/research-objects':
   self.reply({name:_ent_read(kind) for name,kind in ENTERPRISE_KINDS.items()});return
  if url.path=='/api/dv026/lifecycle':
   self.reply(_lifecycle_derived());return
  if url.path=='/api/state':
   q=urllib.parse.parse_qs(url.query);self.reply(S.snapshot(int(q.get('from',['0'])[0])));return
  if url.path=='/api/export':
   data=json.dumps(S.snapshot(0),indent=2).encode();self.send_response(200);self.send_header('Content-Type','application/json');self.send_header('Content-Disposition','attachment; filename="coagentics-v27-run.json"');self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data);return
  if url.path=='/api/dv026/design':
   self.reply(self._load_design());return
  if url.path=='/api/dv026/expectations':
   self.reply(_load_expectations());return
  if url.path=='/api/dv026/preflight':
   # Don't fight a running campaign for the runner lock — return last-known/busy instead of ERROR.
   with S.lock:
    camp_running=bool(S.campaign.running)
    camp_models=int(S.campaign.available_models or 0)
    last_pf=S.preflight
   if camp_running:
    avail=[]
    if isinstance(last_pf,dict):
     avail=last_pf.get('available_models') or last_pf.get('available') or []
    n=len(avail) if isinstance(avail,list) else camp_models
    self.reply({
     'status':'busy',
     'runner_ok':True,
     'live_llm':True,
     'error':None,
     'detail':'Campaign in progress — LLM availability refresh is paused until it finishes.',
     'runtime_status':'unknown',
     'preflight':{
      'ollama_reachable':None,
      'available_models':avail if isinstance(avail,list) else [],
      'catalog_size':(last_pf or {}).get('catalog_size',10) if isinstance(last_pf,dict) else 10,
      'available_count':n or camp_models or 0,
     },
     'ollama_reachable':None,
     'available_models':avail if isinstance(avail,list) else [],
     'catalog_size':(last_pf or {}).get('catalog_size',10) if isinstance(last_pf,dict) else 10,
    });return
   try:
    payload=S.run_dv026('ollama-preflight',424242)
    p=payload.get('preflight',payload) if isinstance(payload,dict) else {}
    with S.lock:
     S.preflight=p if isinstance(p,dict) else payload
    payload['status']='ready' if p.get('ollama_reachable') else 'offline'
    payload['runner_ok']=True
    self.reply(payload);return
   except Exception as e:
    msg=str(e)
    if 'job running' in msg.lower() or 'already running' in msg.lower():
     self.reply({'status':'busy','runner_ok':True,'runtime_status':'unknown','error':None,'detail':msg,'ollama_reachable':None,'available_models':[],'catalog_size':10},200);return
    self.reply({'status':'error','runner_ok':False,'error':msg,'live_llm':True,'ollama_reachable':False,'available_models':[],'catalog_size':10},200);return
  if url.path=='/api/dv026/last':
   with S.lock:
    payload=dict(S.dv026_last) if isinstance(S.dv026_last,dict) else ({'empty':True} if S.dv026_last is None else S.dv026_last)
   la=S.layer_a_status()
   if isinstance(payload,dict) and (la.get('layer_a_pass') or la.get('mean_efficiency_cda') is not None):
    payload={**payload, 'layer_a':la, 'layer_a_pass':la.get('layer_a_pass'),
             'mean_efficiency_cda':la.get('mean_efficiency_cda'),
             'mean_efficiency_sealed':la.get('mean_efficiency_sealed')}
    payload.pop('empty',None)
   self.reply(payload if payload is not None else {'empty':True});return
  if url.path=='/api/dv026/export':
   with S.lock:
    payload=S.dv026_last
   if not payload:
    self.reply({'error':'No DV026 result yet; run bundle first'},400);return
   data=json.dumps(payload,indent=2).encode()
   self.send_response(200);self.send_header('Content-Type','application/json')
   self.send_header('Content-Disposition','attachment; filename="coagentics-dv026-evidence.json"')
   self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data);return
  if url.path=='/api/dv026/batch/status':
   self.reply(S.batch_status());return
  if url.path=='/api/dv026/batch/export':
   try:
    payload=S.batch_export()
   except ValueError as e:
    self.reply({'error':str(e)},400);return
   data=json.dumps(payload,indent=2).encode()
   self.send_response(200);self.send_header('Content-Type','application/json')
   self.send_header('Content-Disposition','attachment; filename="coagentics-dv026-batch-summary.json"')
   self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data);return
  if url.path=='/api/dv026/campaign/status':
   self.reply(S.campaign_status());return
  if url.path=='/api/dv026/campaign/recovery':
   self.reply(S.campaign_recovery());return
  if url.path=='/api/dv026/campaign/recovery/export':
   try:
    payload=S.campaign_export_recovery()
   except ValueError as e:
    self.reply({'error':str(e)},400);return
   data=json.dumps(payload,indent=2).encode()
   self.send_response(200);self.send_header('Content-Type','application/json')
   self.send_header('Content-Disposition','attachment; filename="coagentics-campaign-recovery.json"')
   self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data);return
  if url.path=='/api/dv026/campaign/charts':
   self.reply(S.campaign_charts());return
  if url.path=='/api/dv026/campaign/analysis':
   self.reply(S.campaign_analysis());return
  if url.path=='/api/dv026/campaign/cells':
   self.reply(S.campaign_cells_index());return
  if url.path=='/api/dv026/lab/cells':
   self.reply(lab_cells_index());return
  if url.path=='/api/dv026/lab/summary':
   self.reply(lab_summary());return
  if url.path=='/api/dv026/lab/cell':
   q=urllib.parse.parse_qs(url.query)
   idx=(q.get('index',[''])[0] or '').strip()
   if not idx:
    self.reply({'error':'index query param required'},400);return
   payload=lab_cell_by_index(idx)
   code=400 if payload.get('error') else 200
   self.reply(payload, code);return
  if url.path=='/api/dv026/campaign/cell':
   q=urllib.parse.parse_qs(url.query)
   model=(q.get('model',[''])[0] or '').strip()
   seed_raw=(q.get('seed',[''])[0] or '').strip()
   if not model or not seed_raw:
    self.reply({'error':'model and seed query params required'},400);return
   try:
    seed=int(seed_raw)
   except ValueError:
    self.reply({'error':'seed must be an integer'},400);return
   payload=S.campaign_cell_detail(model, seed)
   code=400 if payload.get('error') else 200
   self.reply(payload, code);return
  if url.path=='/api/dv026/proposal-narrative':
   q=urllib.parse.parse_qs(url.query)
   fmt=(q.get('format',['json'])[0] or 'json').lower()
   payload=S.proposal_narrative()
   if fmt in ('txt','text','plain'):
    data=(payload.get('text') or '').encode('utf-8')
    self.send_response(200)
    self.send_header('Content-Type','text/plain; charset=utf-8')
    self.send_header('Content-Disposition','attachment; filename="dv026-proposal-expectations.txt"')
    self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data);return
   self.reply(payload);return
  if url.path=='/api/dv026/campaign/export':
   try:
    payload=S.campaign_export()
   except ValueError as e:
    self.reply({'error':str(e)},400);return
   data=json.dumps(payload,indent=2).encode()
   self.send_response(200);self.send_header('Content-Type','application/json')
   self.send_header('Content-Disposition','attachment; filename="coagentics-dv026-campaign-summary.json"')
   self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data);return
  if url.path in ('/','/index.html','/interface.html'):
   data=(ROOT/'workbench'/'interface.html').read_bytes();self.send_response(200);self.send_header('Content-Type','text/html; charset=utf-8');self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data);return
  if url.path in ('/how.html','/how'):
   data=(ROOT/'workbench'/'how.html').read_bytes();self.send_response(200);self.send_header('Content-Type','text/html; charset=utf-8');self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data);return
  if url.path=='/live.html':
   data=(ROOT/'workbench'/'live.html').read_bytes();self.send_response(200);self.send_header('Content-Type','text/html; charset=utf-8');self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data);return
  if url.path=='/market_lab.html':
   data=(ROOT/'workbench'/'market_lab.html').read_bytes();self.send_response(200);self.send_header('Content-Type','text/html; charset=utf-8');self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data);return
  if url.path=='/matlab.html':
   data=(ROOT/'workbench'/'matlab.html').read_bytes();self.send_response(200);self.send_header('Content-Type','text/html; charset=utf-8');self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data);return
  if url.path=='/api.js':
   data=(ROOT/'workbench'/'api.js').read_bytes();self.send_response(200);self.send_header('Content-Type','application/javascript; charset=utf-8');self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data);return
  if url.path=='/favicon.ico':
   # Tiny empty response so browsers stop 404-spamming the console.
   self.send_response(204);self.send_header('Content-Length','0');self.end_headers();return
  self.send_error(404)
 def do_POST(self):
  try:
   size=int(self.headers.get('Content-Length','0'))
   if size>65536:raise ValueError('Request too large')
   d=json.loads(self.rfile.read(size) or b'{}')
   if self.path=='/api/llm/chat':
    msg=str(d.get('message','')).strip()
    if not msg or len(msg)>4000:
     self.reply({'ok':False,'error':'message is required and must be <= 4000 characters'},400); return
    result=_ollama_chat(msg)
    _audit_write({'timestamp':time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime()),'action':'llm.communication','detail':msg[:500],'workspace':d.get('workspace','DV026 Economic Behavior'),'actor':d.get('actor','Researcher')})
    self.reply(result,200 if result.get('ok') else 503); return
   if self.path=='/api/platform/audit':
    event={'timestamp':time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime()),'action':str(d.get('action','ui.event')),'detail':str(d.get('detail','')),'workspace':str(d.get('workspace','unknown')),'actor':str(d.get('actor','unknown'))}
    _audit_write(event); self.reply({'ok':True,'event':event}); return
   mtrans=re.match(r'^/api/platform/research-objects/([^/]+)/([^/]+)/transition$',self.path)
   if mtrans and mtrans.group(1) in ENTERPRISE_KINDS:
    name=mtrans.group(1); kind=ENTERPRISE_KINDS[name]; oid=urllib.parse.unquote(mtrans.group(2)); obj=_ent_get(kind,oid)
    if not obj: self.reply({'error':'Not found'},404); return
    if name!='experiments': self.reply({'error':'Lifecycle transitions are currently supported for experiments.'},400); return
    order=['draft','design_review','registered','frozen','running','discovery_complete','hold_out','replication','validation_review','approved_claim']
    cur=str(obj.get('status','draft')); target=str(d.get('target',''))
    if cur not in order or target not in order or order.index(target)!=order.index(cur)+1:
     self.reply({'error':f'Invalid governed transition: {cur} -> {target}'},400); return
    obj['status']=target; obj=_ent_upsert(kind,obj)
    _audit_write({'timestamp':time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime()),'action':'research-object.lifecycle.transition','detail':f'{oid}: {cur} -> {target}'+((' · '+str(d.get('reason'))) if d.get('reason') else ''),'workspace':d.get('workspace','unknown'),'actor':d.get('actor','Researcher')})
    self.reply(obj); return
   m=re.match(r'^/api/platform/research-objects/([^/]+)(?:/([^/]+))?$',self.path)
   if m and m.group(1) in ENTERPRISE_KINDS:
    name=m.group(1); kind=ENTERPRISE_KINDS[name]; oid=urllib.parse.unquote(m.group(2)) if m.group(2) else None
    if self.path.startswith('/api/platform/research-objects/'):
     if oid and d.get('_delete') is True:
      _ent_delete(kind,oid); _audit_write({'timestamp':time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime()),'action':'research-object.delete','detail':name+' '+oid,'workspace':d.get('workspace','unknown'),'actor':d.get('actor','Researcher')}); self.reply({'ok':True,'deleted':oid}); return
     obj=dict(d); obj.pop('_delete',None); obj.pop('actor',None); obj.pop('workspace',None)
     if oid: obj['id']=oid
     saved=_ent_upsert(kind,obj)
     _audit_write({'timestamp':time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime()),'action':'research-object.upsert','detail':name+' '+saved.get('id',''),'workspace':d.get('workspace','unknown'),'actor':d.get('actor','Researcher')})
     self.reply(saved); return
   if self.path=='/api/dv026/lifecycle/transition':
    self.reply(_lifecycle_transition(str(d.get('target','')),str(d.get('actor','Researcher')),str(d.get('reason','')))); return
   if self.path=='/api/dv026/run':
    cmd=str(d.get('command','bundle'))
    seed=int(d.get('seed',424242))
    model=d.get('model')
    self.reply(S.run_dv026(cmd, seed, model));return
   if self.path=='/api/dv026/batch/start':
    self.reply(S.batch_start(int(d.get('n',1000)), int(d.get('base_seed',424242))));return
   if self.path=='/api/dv026/batch/cancel':
    self.reply(S.batch_cancel());return
   if self.path=='/api/dv026/design/save':
    out=self._save_design(d)
    try: _lifecycle_write({**_lifecycle_read(),'state':'DRAFT','history':_lifecycle_read().get('history',[])})
    except Exception: pass
    self.reply(out);return
   if self.path=='/api/dv026/expectations/save':
    doc=_normalize_expectations(d)
    doc['status']='draft'
    doc.pop('manifest_sha256',None);doc.pop('frozen_at',None);doc.pop('frozen_before_observation',None)
    _validate_expectations(doc) if doc.get('expectations') else None
    EXPECTATIONS_DIR.mkdir(parents=True,exist_ok=True)
    EXPECTATIONS_PATH.write_text(json.dumps(doc,indent=2,ensure_ascii=False)+'\n')
    self.reply(doc);return
   if self.path=='/api/dv026/expectations/freeze':
    doc=_normalize_expectations(d)
    frozen=_freeze_expectations(doc)
    try:
     cur=_lifecycle_read().get('state','DRAFT')
     if cur=='DRAFT': _lifecycle_transition('DESIGN_REVIEW','Researcher','Expectations frozen; design review required.')
    except Exception: pass
    self.reply(frozen);return
   if self.path=='/api/dv026/campaign/start':
    try:
     cur=_lifecycle_read().get('state','DRAFT')
     if cur=='DESIGN_REVIEW': _lifecycle_transition('REGISTERED','Researcher','Registered frozen experiment design.')
     if _lifecycle_read().get('state')=='REGISTERED': _lifecycle_transition('FROZEN','Researcher','Experiment manifest frozen before execution.')
    except Exception as e:
     self.reply({'error':str(e)},400); return
    self.reply(S.campaign_start(
     int(d.get('base_seed',424242)),
     int(d.get('n_seeds',20)),
     bool(d.get('smoke',False)),
     str(d.get('population_design','reference_counterparty')),
     str(d.get('activation_design','sequential_interaction'))));
    try:
     if _lifecycle_read().get('state')=='FROZEN': _lifecycle_transition('RUNNING','Researcher','Campaign execution started.')
    except Exception: pass
    return
   if self.path=='/api/dv026/campaign/cancel':
    self.reply(S.campaign_cancel());return
   if self.path=='/api/dv026/campaign/recovery/verify':
    try:
     self.reply(S.campaign_verify_recovery(d.get('run_dir')))
    except ValueError as e:
     self.reply({'error':str(e)},400)
    return
   if self.path=='/api/dv026/campaign/resume':
    try:
     self.reply(S.campaign_resume(d.get('run_dir')))
    except ValueError as e:
     self.reply({'error':str(e)},400)
    return
   if self.path=='/api/dv026/reset':
    self.reply(S.console_reset());return
   with S.lock:
    if self.path=='/api/start':S.start(int(d.get('seed',424242)),int(d.get('events',600)),int(d.get('speed',30)))
    elif self.path=='/api/pause':
     if S.status!='running':raise ValueError('Not running')
     S.status='paused'
    elif self.path=='/api/resume':
     if S.status!='paused':raise ValueError('Not paused')
     S.status='running';S.cond.notify_all()
    elif self.path=='/api/replay':
     if S.status in ('running','paused'):raise ValueError('Stop or finish the current run before replay')
     plan=S.last_run
     if not plan:raise ValueError('No completed experiment to replay')
     S.start(plan['seed'],plan['events'],plan['speed'])
     S.replay_plan=[dict(x) for x in plan['interventions']]
    elif self.path=='/api/reset':S.reset()
    elif self.path=='/api/intervention':
     if S.status not in ('running','paused'):raise ValueError('Start experiment first')
     impulse=float(d.get('impulse',0));edge=d.get('edge',True)
     if not -30<=impulse<=30 or not isinstance(edge,bool):raise ValueError('Invalid intervention')
     S.impulse=impulse;S.edge=edge
     S.interventions.append(dict(after_event=len(S.trace)-1,impulse=impulse,edge=edge,wall_time=time.time(),applies='next activation'))
    else:self.reply({'error':'Not found'},404);return
   self.reply(S.snapshot(len(S.trace)))
  except (ValueError,TypeError,KeyError,FileNotFoundError,json.JSONDecodeError) as e:self.reply({'error':str(e)},400)
if __name__=='__main__':
 import sys
 port=int(sys.argv[1]) if len(sys.argv)>1 else 8787
 print(f'CoAgentics workbench (Market Live + DV026 Evidence): http://127.0.0.1:{port}',flush=True)
 ThreadingHTTPServer(('127.0.0.1',port),Handler).serve_forever()
