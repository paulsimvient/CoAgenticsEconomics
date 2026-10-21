#!/usr/bin/env python3
"""Recompute descriptive Q1 economic metrics from audited protocol archives.

No causal or statistical feasibility claims. Requires a passed archive audit.
"""
import argparse
import json
import math
from pathlib import Path
try:
    from q1_audit_market import audit
except ImportError:
    from tools.q1_audit_market import audit


def score(root):
    root = Path(root)
    verified = audit(root)
    if verified['status'] != 'passed':
        raise ValueError('evidence audit failed: ' + '; '.join(verified['errors']))
    manifest = json.loads((root/'manifest.json').read_text())
    rows = []
    for cell in manifest['cells']:
        index = cell['index']
        folder = root/f'cell_{index:04d}'
        observation = json.loads((folder/'protocol_cell_0.json').read_text())
        report = json.loads((folder/'campaign.json').read_text())
        fundamental = observation.get('fundamental')
        if type(fundamental) not in (float,int) or not math.isfinite(fundamental):
            raise ValueError(f'cell {index}: no valid fundamental in evidence')
        assignments = report['cells'][0]['assignment']
        values = {a['seat']:a for a in assignments}
        trades = observation['trades']
        surplus = 0.0
        deviation = 0.0
        volume = 0
        for trade in trades:
            qty, price = trade['quantity'], trade['price']
            if type(qty) is not int or qty <= 0 or type(price) not in (int,float) or not math.isfinite(price):
                raise ValueError(f'cell {index}: invalid trade quantity or price')
            buyer, seller = values.get(trade['buyer']), values.get(trade['seller'])
            if not buyer or not seller or buyer['role'] != 'buyer' or seller['role'] != 'seller':
                raise ValueError(f'cell {index}: trade parties not matched to archived assignments')
            surplus += qty*(buyer['private_value_or_cost']-seller['private_value_or_cost'])
            deviation += qty*abs(price-fundamental)
            volume += qty
        rows.append(dict(index=index, scenario_id=cell['assignment']['scenario_id'],
                         trial=cell['assignment']['trial'], model=cell['assignment']['model'],
                         condition=cell['assignment']['condition'], seed=cell['assignment']['seed'],
                         trade_count=len(trades), trade_volume=volume,
                         realized_surplus=surplus,
                         mean_abs_price_deviation=(deviation/volume if volume else None),
                         reported_allocative_efficiency=observation['market_metrics']['allocative_efficiency']))
    return {'status':'descriptive_only','cells':rows,
            'claim_boundary':'Surplus and price deviations recalculated from archived trades and assigned values. No benchmark comparison, inferential uncertainty, model-specific attribution of market-wide outcomes, or causal conclusion.'}


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('results_directory');p.add_argument('--output');a=p.parse_args()
    try:
        result=score(a.results_directory)
        rendered=json.dumps(result,indent=2)+'\n'
        if a.output:
            out=Path(a.output)
            with out.open('x') as f:f.write(rendered)
        else:print(rendered)
    except (ValueError,KeyError,TypeError,OSError) as e:p.error(str(e))
