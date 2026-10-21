#!/usr/bin/env python3
"""Execute a preregistered matched-decision plan via the C++ Ollama runner.

This is a *decision-probe* runner, not the shared-market campaign engine.
"""
import argparse, hashlib, json, os, pathlib, subprocess

REQUIRED={'protocol_id','protocol_sha256','assignments'}
def validate(plan):
    if set(plan)!=REQUIRED: raise ValueError('plan requires exactly protocol_id, protocol_sha256, assignments')
    if not isinstance(plan['protocol_id'],str) or not plan['protocol_id'].strip(): raise ValueError('protocol_id required')
    if not isinstance(plan['protocol_sha256'],str) or len(plan['protocol_sha256'])!=64 or any(c not in '0123456789abcdef' for c in plan['protocol_sha256']): raise ValueError('invalid protocol hash')
    assignments=plan['assignments']
    if not isinstance(assignments,list) or not assignments: raise ValueError('assignments required')
    seen=set(); groups={}
    for a in assignments:
        if set(a)!={'scenario_id','trial','model','condition','seed'}: raise ValueError('assignment fields invalid')
        if any(not isinstance(a[k],str) or not a[k].strip() for k in ('scenario_id','trial','model')): raise ValueError('invalid assignment identifier')
        if a['condition'] not in ('control','intervention'): raise ValueError('invalid condition')
        if type(a['seed']) is not int or a['seed']<0 or a['seed']>=2**64: raise ValueError('invalid seed')
        key=(a['scenario_id'],a['trial'],a['model'],a['condition'])
        if key in seen: raise ValueError('duplicate assignment')
        seen.add(key)
        groups.setdefault(key[:3],{})[a['condition']]=a['seed']
    if any(set(pair)!={'control','intervention'} or len(set(pair.values()))!=1 for pair in groups.values()):
        raise ValueError('each scenario/trial/model must have matched control and intervention seeds')
    if len({a['model'] for a in assignments})<2: raise ValueError('at least two distinct models required')
    return assignments

def execute(plan_path, runner, output):
    raw=pathlib.Path(plan_path).read_bytes(); plan=json.loads(raw); assignments=validate(plan)
    out=pathlib.Path(output);out.mkdir(parents=True,exist_ok=True)
    if list(out.iterdir()): raise ValueError('output directory must be empty (no overwrite or resume)')
    (out/'plan.json').write_bytes(raw)
    manifest={'plan_sha256':hashlib.sha256(raw).hexdigest(),'protocol_sha256':plan['protocol_sha256'],'status':'running','cells':[]}
    def save():
        tmp=out/'manifest.tmp';tmp.write_text(json.dumps(manifest,indent=2)+'\n');tmp.replace(out/'manifest.json')
    save()
    for index,a in enumerate(assignments):
        env=os.environ.copy()
        env.update(COAGENTICS_PROTOCOL_MODEL=a['model'],COAGENTICS_PROTOCOL_CONDITION=a['condition'],
                   COAGENTICS_PROTOCOL_ID=plan['protocol_id'],COAGENTICS_PROTOCOL_SHA256=plan['protocol_sha256'],
                   COAGENTICS_PROTOCOL_SCENARIO=a['scenario_id'],COAGENTICS_PROTOCOL_TRIAL=a['trial'])
        proc=subprocess.run([str(runner),'protocol-decision',str(a['seed'])],env=env,capture_output=True,text=True)
        item={'index':index,'assignment':a,'returncode':proc.returncode}
        try:
            result=json.loads(proc.stdout)
            if result.get('protocol',{}).get('model')!=a['model'] or result['protocol']['condition']!=a['condition'] or result['protocol']['seed']!=a['seed']:
                raise ValueError('runner assignment mismatch')
            if not result['protocol'].get('provider_response_received'): raise ValueError('no provider response')
            cell=out/f'cell_{index:04d}.json';cell.write_text(json.dumps(result,indent=2)+'\n')
            item['result_sha256']=hashlib.sha256(cell.read_bytes()).hexdigest()
        except (ValueError,KeyError,TypeError) as exc:
            item['error']=str(exc) if proc.returncode==0 else (proc.stderr[-1000:] or 'runner failed')
        manifest['cells'].append(item);save()
        if proc.returncode or 'error' in item:
            manifest['status']='failed';save();raise RuntimeError(f'cell {index} failed: {item.get("error",proc.stderr[-1000:])}')
    manifest['status']='completed';save()
    return manifest

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('plan');p.add_argument('runner');p.add_argument('output_dir');args=p.parse_args()
    try: print(json.dumps(execute(args.plan,args.runner,args.output_dir),indent=2))
    except (ValueError,RuntimeError,OSError) as e:p.error(str(e))
