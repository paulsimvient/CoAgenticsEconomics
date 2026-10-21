#!/usr/bin/env python3
"""Independent, fail-closed verification of Q1 protocol-driven market archives.

Does not infer decision quality or certify model behavior. Read-only.
"""
import argparse
import hashlib
import json
from pathlib import Path

try:
    from av1_execute_plan import validate
except ImportError:
    from tools.av1_execute_plan import validate


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def audit(directory):
    root = Path(directory)
    plan_path = root / 'plan.json'
    plan = json.loads(plan_path.read_text())
    assignments = validate(plan)
    manifest = json.loads((root / 'manifest.json').read_text())
    errors = []
    if manifest.get('plan_sha256') != digest(plan_path):
        errors.append('plan fingerprint mismatch')
    cells = manifest.get('cells')
    if not isinstance(cells, list):
        raise ValueError('manifest cells must be an array')
    if len(cells) != len(assignments):
        errors.append('assignment count mismatch')
    if manifest.get('status') != 'completed':
        errors.append('manifest not completed')
    seen = set()
    total_turns = 0
    parsed_turns = 0
    for cell in cells:
        index = cell.get('index')
        if type(index) is not int or index < 0 or index >= len(assignments) or index in seen:
            errors.append('duplicate/invalid cell index')
            continue
        seen.add(index)
        a = assignments[index]
        if cell.get('assignment') != a or cell.get('returncode') != 0 or cell.get('error'):
            errors.append(f'cell {index}: manifest assignment or execution mismatch')
        folder = root / f'cell_{index:04d}'
        observation = folder / 'protocol_cell_0.json'
        campaign = folder / 'campaign.json'
        if not observation.is_file() or not campaign.is_file():
            errors.append(f'cell {index}: missing evidence files')
            continue
        if cell.get('observation_sha256') != digest(observation):
            errors.append(f'cell {index}: observation hash mismatch')
        try:
            data = json.loads(observation.read_text())
            report = json.loads(campaign.read_text())
        except (ValueError, OSError):
            errors.append(f'cell {index}: malformed evidence')
            continue
        expected = {
            'protocol_id': plan['protocol_id'], 'protocol_sha256': plan['protocol_sha256'],
            'scenario_id': a['scenario_id'], 'trial': a['trial'],
            'condition': a['condition'], 'requested_model': a['model'],
            'seed': a['seed'], 'public_signal': 100 if a['condition'] == 'control' else 70,
        }
        for key, value in expected.items():
            if data.get(key) != value:
                errors.append(f'cell {index}: {key} mismatch')
        for key in ('protocol_id', 'protocol_sha256', 'scenario_id', 'trial', 'condition'):
            if report.get(key) != expected[key]:
                errors.append(f'cell {index}: campaign {key} mismatch')
        if not report.get('minimum_model_requirement_met') or report.get('distinct_models_used', 0) < 2:
            errors.append(f'cell {index}: insufficient model evidence')
        if len(report.get('cells', [])) != 1 or report['cells'][0].get('seed') != a['seed']:
            errors.append(f'cell {index}: campaign cell mismatch')
        trades = data.get('trades')
        metrics = data.get('market_metrics')
        if not isinstance(trades, list) or not isinstance(metrics, dict):
            errors.append(f'cell {index}: missing market outcomes')
        else:
            if metrics.get('trade_count') != len(trades) or len(trades) != report['cells'][0].get('trades'):
                errors.append(f'cell {index}: inconsistent trade count')
            for trade in trades:
                if not isinstance(trade, dict) or not all(k in trade for k in ('time','asset','quantity','price','buyer','seller')):
                    errors.append(f'cell {index}: invalid trade evidence')
        turns = data.get('turns')
        if not isinstance(turns, list) or not turns:
            errors.append(f'cell {index}: no inference turns')
            continue
        models = set()
        for turn in turns:
            if not isinstance(turn, dict) or not turn.get('model') or not turn.get('canonical_request') or not turn.get('raw_response'):
                errors.append(f'cell {index}: incomplete model inference record')
                continue
            models.add(turn['model'])
            total_turns += 1
            parsed_turns += int(turn.get('parse_ok') is True)
        if a['model'] not in models or len(models) < 2:
            errors.append(f'cell {index}: requested and second model not both observed')
    return {
        'status': 'passed' if not errors else 'failed',
        'cells_verified': len(seen), 'cells_expected': len(assignments),
        'inference_turns': total_turns, 'parse_success_turns': parsed_turns,
        'errors': errors,
        'claim_boundary': 'Archive integrity and assignment consistency only; no independent validation of AI provider, economic outcomes, or causal effects.',
    }


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('results_directory')
    args = parser.parse_args()
    try:
        result = audit(args.results_directory)
        print(json.dumps(result, indent=2))
        raise SystemExit(0 if result['status'] == 'passed' else 1)
    except (OSError, ValueError, KeyError, TypeError) as exc:
        parser.error(str(exc))
