import unittest
from tools.av1_execute_plan import validate
class ProtocolPlanTests(unittest.TestCase):
 def plan(self):
  return {'protocol_id':'demo','protocol_sha256':'a'*64,'assignments':[{'scenario_id':'s','trial':'1','model':m,'condition':c,'seed':17} for m in ('m1','m2') for c in ('control','intervention')]}
 def test_valid(self):self.assertEqual(len(validate(self.plan())),4)
 def test_unpaired(self):
  p=self.plan();p['assignments'].pop()
  with self.assertRaisesRegex(ValueError,'matched'):validate(p)
 def test_seed_mismatch(self):
  p=self.plan();p['assignments'][1]['seed']=18
  with self.assertRaisesRegex(ValueError,'matched'):validate(p)
 def test_one_model(self):
  p=self.plan();p['assignments']=p['assignments'][:2]
  with self.assertRaisesRegex(ValueError,'two distinct'):validate(p)
 def test_duplicate(self):
  p=self.plan();p['assignments'].append(dict(p['assignments'][0]))
  with self.assertRaisesRegex(ValueError,'duplicate'):validate(p)
