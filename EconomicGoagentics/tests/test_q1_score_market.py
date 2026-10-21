import json
import unittest
from tools.q1_score_market import score
from test_q1_audit_market import Q1AuditTests


class Q1ScoringTests(Q1AuditTests):
    def setUp(self):
        super().setUp()
        for i in range(4):
            folder = self.root/f'cell_{i:04d}'
            report_path = folder/'campaign.json'
            report=json.loads(report_path.read_text())
            report['cells'][0]['assignment'] = [
                dict(seat='B', role='buyer', private_value_or_cost=120),
                dict(seat='S', role='seller', private_value_or_cost=80)]
            report_path.write_text(json.dumps(report))

    def test_no_trades(self):
        result=score(self.root)
        self.assertEqual(result['status'],'descriptive_only')
        self.assertEqual(result['cells'][0]['realized_surplus'],0)
        self.assertIsNone(result['cells'][0]['mean_abs_price_deviation'])

    def test_recompute_trade_surplus_and_deviation(self):
        folder=self.root/'cell_0000'
        p=folder/'protocol_cell_0.json';data=json.loads(p.read_text())
        data['trades']=[dict(time=0,asset='ASSET',quantity=1,price=95,buyer='B',seller='S')]
        data['market_metrics']['trade_count']=1
        p.write_text(json.dumps(data))
        campaign=folder/'campaign.json';report=json.loads(campaign.read_text());report['cells'][0]['trades']=1
        campaign.write_text(json.dumps(report))
        import hashlib
        manifest=self.root/'manifest.json';m=json.loads(manifest.read_text())
        m['cells'][0]['observation_sha256']=hashlib.sha256(p.read_bytes()).hexdigest()
        manifest.write_text(json.dumps(m))
        result=score(self.root)['cells'][0]
        self.assertEqual(result['realized_surplus'],40)
        self.assertEqual(result['mean_abs_price_deviation'],5)
