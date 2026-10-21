import hashlib
import json
import tempfile
import unittest
from pathlib import Path
from tools.q1_audit_market import audit


class Q1AuditTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.assignments = [dict(scenario_id='s', trial='1', model=m, condition=c, seed=9)
                            for m in ('model-a', 'model-b') for c in ('control', 'intervention')]
        plan = dict(protocol_id='p', protocol_sha256='a'*64, assignments=self.assignments)
        (self.root/'plan.json').write_text(json.dumps(plan))
        cells = []
        for i, a in enumerate(self.assignments):
            folder = self.root/f'cell_{i:04d}'
            folder.mkdir()
            data = dict(protocol_id='p', protocol_sha256='a'*64, scenario_id='s', trial='1',
                        condition=a['condition'], requested_model=a['model'], seed=9,
                        fundamental=100, public_signal=100 if a['condition']=='control' else 70,
                        trades=[], market_metrics={'allocative_efficiency': 0.0, 'trade_count': 0},
                        turns=[dict(model=m, canonical_request='{}', raw_response='{"action":"HOLD"}', parse_ok=True)
                               for m in ('model-a', 'model-b')])
            (folder/'protocol_cell_0.json').write_text(json.dumps(data))
            report = dict(protocol_id='p', protocol_sha256='a'*64, scenario_id='s', trial='1',
                          condition=a['condition'], minimum_model_requirement_met=True,
                          distinct_models_used=2, cells=[dict(seed=9, trades=0)])
            (folder/'campaign.json').write_text(json.dumps(report))
            cells.append(dict(index=i, assignment=a, returncode=0,
                              observation_sha256=hashlib.sha256((folder/'protocol_cell_0.json').read_bytes()).hexdigest()))
        manifest = dict(status='completed', plan_sha256=hashlib.sha256((self.root/'plan.json').read_bytes()).hexdigest(), cells=cells)
        (self.root/'manifest.json').write_text(json.dumps(manifest))

    def test_valid_archive(self):
        self.assertEqual(audit(self.root)['status'], 'passed')

    def test_tampered_observation(self):
        p=self.root/'cell_0000/protocol_cell_0.json'
        p.write_text(p.read_text()+' ')
        self.assertIn('observation hash mismatch', ' '.join(audit(self.root)['errors']))

    def test_missing_second_model(self):
        p=self.root/'cell_0000/protocol_cell_0.json'
        data=json.loads(p.read_text());data['turns']=data['turns'][:1]
        p.write_text(json.dumps(data))
        self.assertIn('second model not both observed', ' '.join(audit(self.root)['errors']))

    def test_incomplete_manifest(self):
        p=self.root/'manifest.json';m=json.loads(p.read_text());m['status']='running';p.write_text(json.dumps(m))
        self.assertIn('manifest not completed', ' '.join(audit(self.root)['errors']))

    def test_wrong_assignment(self):
        p=self.root/'manifest.json';m=json.loads(p.read_text());m['cells'][0]['assignment']['seed']=10;p.write_text(json.dumps(m))
        self.assertIn('manifest assignment', ' '.join(audit(self.root)['errors']))
