#!/usr/bin/env python3
"""Execute matched protocol assignments in the live heterogeneous C++ market."""
import argparse,hashlib,json,os,pathlib,subprocess
from av1_execute_plan import validate

def execute(plan_path,runner,output):
 raw=pathlib.Path(plan_path).read_bytes();plan=json.loads(raw);assignments=validate(plan)
 out=pathlib.Path(output)
 if out.exists() and any(out.iterdir()):raise ValueError("output must be empty; resume not implemented")
 out.mkdir(parents=True,exist_ok=True)
 (out/'plan.json').write_bytes(raw)
 manifest={'status':'running','plan_sha256':hashlib.sha256(raw).hexdigest(),'cells':[]}
 def save():
  t=out/'manifest.tmp';t.write_text(json.dumps(manifest,indent=2)+'\n');t.replace(out/'manifest.json')
 save()
 for i,a in enumerate(assignments):
  cell_dir=out/f'cell_{i:04d}'
  env=os.environ.copy();env.update(COAGENTICS_PROTOCOL_ID=plan['protocol_id'],COAGENTICS_PROTOCOL_SHA256=plan['protocol_sha256'],COAGENTICS_PROTOCOL_SCENARIO=a['scenario_id'],COAGENTICS_PROTOCOL_TRIAL=a['trial'],COAGENTICS_PROTOCOL_MODEL=a['model'],COAGENTICS_PROTOCOL_CONDITION=a['condition'],COAGENTICS_PROTOCOL_RESULTS_DIR=str(cell_dir.resolve()))
  proc=subprocess.run([str(pathlib.Path(runner).resolve()),'hetero-campaign',str(a['seed']),'1'],env=env,capture_output=True,text=True)
  item={'index':i,'assignment':a,'returncode':proc.returncode}
  try:
   if proc.returncode:raise ValueError(proc.stderr[-1500:] or 'C++ runner failed')
   result=json.loads(proc.stdout)['heterogeneous_campaign']
   if result.get('protocol_id')!=plan['protocol_id'] or result.get('protocol_sha256')!=plan['protocol_sha256'] or result.get('condition')!=a['condition'] or result.get('scenario_id')!=a['scenario_id'] or result.get('trial')!=a['trial'] or not result.get('minimum_model_requirement_met') or len(result.get('cells',[]))!=1:raise ValueError('C++ campaign mismatch or incomplete execution')
   if result['cells'][0]['seed']!=a['seed'] or a['model'] not in [s['model'] for s in result['cells'][0]['assignment']]:raise ValueError('seed/model mismatch')
   evidence=cell_dir/'protocol_cell_0.json';data=json.loads(evidence.read_text())
   if data['seed']!=a['seed'] or data['condition']!=a['condition'] or data['requested_model']!=a['model'] or data['protocol_sha256']!=plan['protocol_sha256'] or data['public_signal']!=(100 if a['condition']=='control' else 70):raise ValueError('observation mismatch')
   turns=data.get('turns',[])
   if not turns or not any(t.get('model')==a['model'] and t.get('raw_response') for t in turns):raise ValueError('missing requested live model response')
   if len({t.get('model') for t in turns if t.get('raw_response')})<2:raise ValueError('second live model response missing')
   if any(not t.get('raw_response') for t in turns):raise ValueError('empty live model response')
   item['observation_sha256']=hashlib.sha256(evidence.read_bytes()).hexdigest()
  except (ValueError,KeyError,TypeError,OSError) as e:item['error']=str(e)
  manifest['cells'].append(item)
  if 'error' in item:
   manifest['status']='failed';save();raise RuntimeError(f'cell {i}: {item["error"]}')
  save()
 manifest['status']='completed';save()
 # Q1 independent evidence verification and descriptive scoring are mandatory
 # before a run may be reported as qualified. A failure marks the manifest.
 try:
  from q1_audit_market import audit
  from q1_score_market import score
  result=audit(out)
  if result['status']!='passed':raise ValueError('; '.join(result['errors']))
  (out/'q1_evidence_audit.json').write_text(json.dumps(result,indent=2)+'\n')
  (out/'q1_descriptive_scores.json').write_text(json.dumps(score(out),indent=2)+'\n')
 except (ValueError,KeyError,TypeError,OSError) as exc:
  manifest['status']='failed';manifest['qualification_error']=str(exc);save()
  raise RuntimeError(f'Q1 evidence qualification failed: {exc}') from exc
 return manifest

if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('plan');p.add_argument('runner');p.add_argument('output_dir');a=p.parse_args()
 try:print(json.dumps(execute(a.plan,a.runner,a.output_dir),indent=2))
 except (ValueError,RuntimeError,OSError) as e:p.error(str(e))
