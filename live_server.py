#!/usr/bin/env python3
"""Localhost workbench controller: v27 paced CDA + DV026 evidence runner + batch validation."""
import json, os, subprocess, threading, time, urllib.parse
from http.server import ThreadingHTTPServer, BaseHTTPRequestHandler
from pathlib import Path
ROOT=Path(__file__).resolve().parent

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
  self.base_seed=424242; self.n_seeds=20; self.smoke=False
  self.done=0; self.total=0
  self.pass_count=0; self.fail_count=0
  self.current_model=None; self.current_seed=None
  self.started_at=None; self.finished_at=None; self.last_error=None
  self.cancelled=False; self.proc=None
  self.result=None; self.readiness_checks=[]; self.scope='not_ready'
  self.darpa_claim_ready=False; self.available_models=0; self.cells_ok=0
  self.distinct_live_models_ok=0
  self.gate_partial=None
 def snapshot(self):
  pct=(100.0*self.done/self.total) if self.total else (100.0 if self.result else 0.0)
  complete=(not self.running) and (self.result is not None or self.cancelled or (self.finished_at is not None and self.done>0))
  out=dict(
   running=self.running, cancelled=self.cancelled, command='ollama-campaign',
   base_seed=self.base_seed, n_seeds=self.n_seeds, smoke=self.smoke,
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
   complete=complete,
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
  self.batch=BatchJob(); self.campaign=CampaignJob(); self.preflight=None
  self.reset()
 def reset(self):
  with self.lock:
   self.generation+=1
   if self.proc and self.proc.poll() is None:self.proc.kill()
   self.proc=None;self.status='idle';self.seed=None;self.events=0;self.speed=30
   self.impulse=0.;self.edge=True;self.trace=[];self.outcomes={};self.interventions=[];self.error=None
   self.replay_plan=[];self.cond.notify_all()
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
           'ollama-preflight','ollama-slice','ollama-paired','ollama-campaign'}
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
   self.dv026_last=data
   if command=='ollama-preflight':
    self.preflight=data.get('preflight', data) if isinstance(data,dict) else data
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

 def campaign_start(self, base_seed=424242, n_seeds=20, smoke=False):
  with self.lock:
   if self.batch.running or self.campaign.running: raise ValueError('Job already running')
   if self.status in ('running','paused'): raise ValueError('Market Live experiment active; reset first')
   if not isinstance(n_seeds,int) or not 1<=n_seeds<=100: raise ValueError('n_seeds must be 1..100')
   if not isinstance(base_seed,int) or base_seed<0: raise ValueError('Invalid base_seed')
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
  with self.lock:
   find_exe('dv026-workbench-runner')
   c=self.campaign
   c.reset_idle()
   c.running=True; c.base_seed=base_seed; c.n_seeds=seed_count; c.smoke=bool(smoke)
   c.available_models=model_count; c.total=model_count*seed_count
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
 def campaign_status(self):
  with self.lock:
   self._hydrate_campaign_from_disk_locked()
   return self.campaign.snapshot()
 def _hydrate_campaign_from_disk_locked(self):
  """If no in-memory campaign result, surface the last on-disk CLI/UI summary."""
  c=self.campaign
  if c.running or c.result is not None: return
  path=ROOT/'results'/'dv026_ollama_campaign'/'summary.json'
  if not path.is_file(): return
  try:
   camp=json.loads(path.read_text())
  except Exception:
   return
  if not isinstance(camp,dict): return
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
  n_seeds=20
  layer_a_cda=None
  live_eff_sum=0.0
  live_eff_n=0
  with self.lock:
   running=bool(self.campaign.running)
   n_seeds=int(self.campaign.n_seeds or 20)
   if self.campaign.result and isinstance(self.campaign.result,dict):
    camp=self.campaign.result.get('campaign') or self.campaign.result
    if isinstance(camp,dict) and camp.get('mean_efficiency_cda') is not None:
     layer_a_cda=float(camp.get('mean_efficiency_cda'))
  if summary_path.is_file():
   try:
    summ=json.loads(summary_path.read_text())
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
  }
 def proposal_narrative(self):
  """Plain-language checklist: white-paper expectations vs live evidence."""
  fixture_path=ROOT/'docs'/'DV026_PROPOSAL_EXPECTATIONS.json'
  try:
   fixture=json.loads(fixture_path.read_text())
  except Exception as e:
   return {'error':f'missing expectations fixture: {e}','text':'','items':[]}
  charts=self.campaign_charts()
  summary_path=ROOT/'results'/'dv026_ollama_campaign'/'summary.json'
  summ={}
  if summary_path.is_file():
   try: summ=json.loads(summary_path.read_text())
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
   # Claim / scope only when the campaign gate has been scored (not mid-run).
   if running:
    scope='not_ready'
    darpa=False
   else:
    scope=str(c.scope or camp.get('scope') or summ.get('scope') or 'not_ready')
    if scope=='running':
     scope='not_ready'
    darpa=bool(c.darpa_claim_ready if c.result is not None else
               (camp.get('darpa_claim_ready') if camp else summ.get('darpa_claim_ready')))
   layer_a_pass=bool(camp.get('layer_a_pass') or summ.get('layer_a_pass'))
   last=self.dv026_last if isinstance(self.dv026_last,dict) else {}
   la=last.get('layer_a') if isinstance(last.get('layer_a'),dict) else last
   if isinstance(la,dict) and (la.get('layer_a_pass') or la.get('pass')):
    layer_a_pass=True
   n_seeds=int(c.n_seeds or charts.get('n_seeds') or summ.get('n_seeds') or 20)
  # Always prefer live cells.jsonl counts for model coverage (honest mid-run).
  by_model=charts.get('by_model') or []
  mok=sum(1 for b in by_model if int(b.get('cell_ok') or 0)>=1)
  full_cov=sum(1 for b in by_model if int(b.get('cell_ok') or 0)>=n_seeds)
  if not running and mok==0:
   mok=int(camp.get('distinct_live_models_ok') or summ.get('distinct_live_models_ok') or 0)
  pipe=charts.get('pipeline') or {}
  attempted=int(pipe.get('attempted') or 0)
  parse_ok=int(pipe.get('parse_ok') or 0)
  action_valid=int(pipe.get('action_valid') or 0)
  replay_ok=int(pipe.get('replay_ok') or 0)
  iface_num=min(parse_ok, action_valid) if attempted else 0
  iface_rate=(iface_num/attempted) if attempted else None
  prov_rate=(replay_ok/attempted) if attempted else None
  def check_pass(cid):
   for ch in checks:
    if isinstance(ch,dict) and ch.get('id')==cid:
     return bool(ch.get('pass')), str(ch.get('detail') or '')
   return None, ''
  # Mid-run: ignore finished-gate checks for live rates; use cells.
  # human_ref MET only from real campaign gate — never auto-true.
  href_p,_=check_pass('human_ref')
  if href_p is None:
   href_p=bool(summ.get('human_ref_gate')) if summ.get('human_ref_gate') is not None else None
  if running:
   # Prior finished gate must not count as MET while a new campaign is in progress.
   href_p=None
   prov_p=(prov_rate is not None and prov_rate>=0.95)
   iface_p=(iface_rate is not None and iface_rate>=0.90)
  else:
   if href_p is None:
    href_p=False
   prov_p,_=check_pass('provenance')
   if prov_p is None and attempted:
    prov_p=prov_rate>=0.95
   iface_p,_=check_pass('interface_health')
   if iface_p is None and iface_rate is not None:
    iface_p=iface_rate>=0.90
  layer_present=layer_a_pass or (summ.get('mean_efficiency_cda') is not None) or (camp.get('mean_efficiency_cda') is not None)
  cda=summ.get('mean_efficiency_cda', camp.get('mean_efficiency_cda'))
  sealed=summ.get('mean_efficiency_sealed', camp.get('mean_efficiency_sealed'))
  def pct(v):
   try: return f'{float(v):.1f}%'
   except (TypeError,ValueError): return '—'
  def rate_pct(r):
   if r is None: return '—'
   return f'{100.0*float(r):.0f}%'

  def score(gate):
   """Return (status, found, still_needed|None)."""
   if gate=='layer_a_present':
    if layer_a_pass:
     return 'MET', 'Market qualification ran and passed (CDA and sealed-bid rules exercised).', None
    if layer_present:
     return 'PARTIAL', 'Market metrics exist but qualification did not pass.', 'Re-run Layer A until every trial clears above 90%.'
    return 'NOT_YET', 'Market qualification has not been run yet.', 'Run Layer A market qualification.'
   if gate=='layer_a_qualified':
    if layer_a_pass:
     return 'MET', f'Market qualification passed; mean efficiency CDA {pct(cda)}, sealed-bid {pct(sealed)}.', None
    return 'NOT_YET', 'Market qualification not passed.', 'Every Layer A trial must clear above 90% efficiency on CDA and sealed-bid.'
   if gate=='interface_health':
    if iface_p:
     return 'MET', f'Parse/validate rate {rate_pct(iface_rate)} on {attempted} live cells (need at least 90%).', None
    if attempted:
     return 'PARTIAL', f'Parse/validate rate {rate_pct(iface_rate)} on {attempted} live cells so far (need at least 90%).', 'Improve action-schema compliance on remaining cells.'
    return 'NOT_YET', 'No live agent cells yet.', 'Start a live Ollama campaign.'
   if gate=='ten_models_covered':
    found=f'{mok} models with at least 1 accepted cell; {full_cov} models completed all {n_seeds} seeds.'
    if mok>=10 and full_cov>=10:
     return 'MET', found, None
    if mok>=1:
     need=max(0,10-full_cov)
     return 'PARTIAL', found, f'{need} more model{"s" if need!=1 else ""} with full {n_seeds}-seed coverage before this is MET.'
    return 'NOT_YET', 'No successful live model cells yet.', f'Exercise at least 10 models across {n_seeds} seeds each.'
   if gate=='human_ref':
    if href_p is True:
     return 'MET', 'HumanComparison DESCRIPTIVE gate passed (archival literature + LiveProvider evidence).', None
    if href_p is None:
     return 'PARTIAL', 'Human-ref gate not scored yet (campaign in progress).', 'Wait for campaign finish; gate requires provenance-matched HumanComparison.'
    return 'NOT_YET', 'HumanComparison DESCRIPTIVE gate not passed.', 'Need LiveProvider observations matched to archival literature via HumanComparison.'
   if gate=='provenance':
    found=f'Evidence chain retained on {replay_ok}/{attempted} cells ({rate_pct(prov_rate)}; need at least 95%).' if attempted else 'No live cells yet.'
    if prov_p:
     return 'MET', found, None
    if attempted:
     return 'PARTIAL', found, 'Raise replay/provenance retention to at least 95% of attempted cells.'
    return 'NOT_YET', found, 'Run live cells so observation → response → action → outcome is retained.'
   if gate=='claim_boundary':
    if running:
     return 'MET', 'Console keeps software fixtures distinct from live LLM evidence; local PoC gate is not decided while the campaign is running. Phase II and FAQ 31 accuracy remain unclaimed.', None
    if scope=='local_ollama_poc' and darpa:
     return 'MET', 'Claim scope is local Ollama PoC only. Phase II construct validation and FAQ 31 classifier accuracy remain unclaimed.', None
    if attempted or layer_present:
     return 'MET', 'Boundary stated: local executable evidence only; commercial multi-provider readiness and Phase II remain unclaimed.', None
    return 'PARTIAL', 'Boundary text is ready, but little live evidence has been collected yet.', 'Collect Layer A and/or live campaign evidence under local PoC scope.'
   return 'NOT_YET', f'Unknown gate {gate}.', None

  items=[]
  counts={'MET':0,'PARTIAL':0,'NOT_YET':0}
  for exp in fixture.get('expectations') or []:
   st, found, still=score(exp.get('gate'))
   counts[st]=counts.get(st,0)+1
   items.append({
    'id':exp.get('id'),'milestone':exp.get('milestone'),'title':exp.get('title'),
    'text':exp.get('text'),'gate':exp.get('gate'),'status':st,
    'evidence':found,'still_needed':still
   })

  camp_label='IN PROGRESS' if running else ('READY' if darpa and scope=='local_ollama_poc' else 'NOT READY')
  lines=[]
  lines.append('DV026 Phase I — proposal checklist (local evidence)')
  lines.append('')
  lines.append(f"Snapshot: campaign {camp_label} · {counts['MET']} MET · {counts['PARTIAL']} PARTIAL · {counts['NOT_YET']} NOT YET")
  lines.append('Evidence base: Layer A market qualification + local Ollama live cells')
  lines.append('Not claimed: commercial 10-provider matrix · Phase II constructs · FAQ 31 classifier accuracy')
  lines.append('')
  lines.append('────────────────────────────────')
  for it in items:
   title=it.get('title') or it.get('id') or 'Expectation'
   ms=it.get('milestone') or ''
   ms_tag=f' ({ms})' if ms and ms!='cross-cutting' else ''
   lines.append(f"{it['status']}  · {title}{ms_tag}")
   lines.append(f"Asks: {it.get('text')}")
   lines.append(f"Found: {it.get('evidence')}")
   if it.get('still_needed'):
    lines.append(f"Still needed: {it['still_needed']}")
   lines.append('')
  lines.append('────────────────────────────────')
  if running:
   bottom=f'Local PoC gate not ready (campaign still running).'
  elif darpa and scope=='local_ollama_poc':
   bottom='Local PoC gate READY under scope=local_ollama_poc (not a commercial multi-provider claim).'
  else:
   bottom=f'Local PoC gate not ready (scope={scope}).'
  lines.append(f'Bottom line: {bottom}')
  lines.append(f'  Models with accepted cells: {mok}/10 · Full-coverage models: {full_cov}/10 · Cells attempted: {attempted}')
  text='\n'.join(lines)
  return {
   'text':text,
   'items':items,
   'meta':{
    'darpa_claim_ready':darpa,'scope':scope,'running':running,'campaign_label':camp_label,
    'distinct_ok':mok,'coverage':full_cov,'n_seeds':n_seeds,'attempted':attempted,
    'layer_a_pass':layer_a_pass,'counts':counts
   }
  }
 def _campaign_worker(self):
  exe=find_exe('dv026-workbench-runner')
  env=os.environ.copy()
  env.setdefault('COAGENTICS_LLM_PROVIDER','ollama')
  env.setdefault('COAGENTICS_LLM_BASE_URL','http://127.0.0.1:11434/v1')
  with self.lock:
   c=self.campaign
   args=[str(exe),'ollama-campaign',str(c.base_seed),str(c.n_seeds)]
   if c.smoke: args.append('--smoke')
  try:
   proc=subprocess.Popen(args,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,cwd=str(ROOT),env=env)
   with self.lock:
    self.campaign.proc=proc
   # Poll cancel while waiting; campaign is one C++ process (progress via summary on finish)
   while True:
    with self.lock:
     if self.campaign.cancel:
      try: proc.kill()
      except Exception: pass
      break
    rc=proc.poll()
    if rc is not None: break
    # Soft progress: count cells.jsonl lines if present
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
     return
    if proc.returncode!=0:
     c.running=False; c.cancelled=False; c.finished_at=time.time()
     c.last_error=(err or out or f'exit {proc.returncode}').strip()
     c.current_model=None; c.current_seed=None
     return
    data=json.loads(out)
    camp=data.get('campaign') if isinstance(data.get('campaign'),dict) else data
    c.result=data
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
  except Exception as e:
   with self.lock:
    c=self.campaign
    c.running=False; c.finished_at=time.time()
    c.last_error=str(e)
    c.current_model=None; c.current_seed=None

S=Session()
class Handler(BaseHTTPRequestHandler):
 def log_message(self,*a):pass
 def reply(self,obj,code=200):
  raw=json.dumps(obj).encode();self.send_response(code);self.send_header('Content-Type','application/json');self.send_header('Cache-Control','no-store');self.send_header('Content-Length',str(len(raw)));self.end_headers();self.wfile.write(raw)
 def do_GET(self):
  url=urllib.parse.urlsplit(self.path)
  if url.path=='/api/state':
   q=urllib.parse.parse_qs(url.query);self.reply(S.snapshot(int(q.get('from',['0'])[0])));return
  if url.path=='/api/export':
   data=json.dumps(S.snapshot(0),indent=2).encode();self.send_response(200);self.send_header('Content-Type','application/json');self.send_header('Content-Disposition','attachment; filename="coagentics-v27-run.json"');self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data);return
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
     'preflight':{
      'ollama_reachable':True,
      'available_models':avail if isinstance(avail,list) else [],
      'catalog_size':(last_pf or {}).get('catalog_size',10) if isinstance(last_pf,dict) else 10,
      'available_count':n or camp_models or 10,
     },
     'ollama_reachable':True,
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
     self.reply({'status':'busy','runner_ok':True,'error':None,'detail':msg,'ollama_reachable':True,'available_models':[],'catalog_size':10},200);return
    self.reply({'status':'error','runner_ok':False,'error':msg,'live_llm':True,'ollama_reachable':False,'available_models':[],'catalog_size':10},200);return
  if url.path=='/api/dv026/last':
   with S.lock:
    self.reply(S.dv026_last or {'empty':True});return
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
  if url.path=='/api/dv026/campaign/charts':
   self.reply(S.campaign_charts());return
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
  if url.path in ('/','/index.html','/live.html'):
   data=(ROOT/'workbench'/'live.html').read_bytes();self.send_response(200);self.send_header('Content-Type','text/html; charset=utf-8');self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data);return
  self.send_error(404)
 def do_POST(self):
  try:
   size=int(self.headers.get('Content-Length','0'))
   if size>8192:raise ValueError('Request too large')
   d=json.loads(self.rfile.read(size) or b'{}')
   if self.path=='/api/dv026/run':
    cmd=str(d.get('command','bundle'))
    seed=int(d.get('seed',424242))
    model=d.get('model')
    self.reply(S.run_dv026(cmd, seed, model));return
   if self.path=='/api/dv026/batch/start':
    self.reply(S.batch_start(int(d.get('n',1000)), int(d.get('base_seed',424242))));return
   if self.path=='/api/dv026/batch/cancel':
    self.reply(S.batch_cancel());return
   if self.path=='/api/dv026/campaign/start':
    self.reply(S.campaign_start(
     int(d.get('base_seed',424242)),
     int(d.get('n_seeds',20)),
     bool(d.get('smoke',False))));return
   if self.path=='/api/dv026/campaign/cancel':
    self.reply(S.campaign_cancel());return
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
