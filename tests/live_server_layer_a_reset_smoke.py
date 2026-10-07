#!/usr/bin/env python3
"""Regression: after Reset, on-disk Layer A must not qualify Full campaigns."""
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

import live_server as ls  # noqa: E402

summary = ROOT / 'results' / 'dv026_ollama_campaign' / 'summary.json'
# Ensure on-disk evidence looks qualified so the smoke exercises the gate.
if summary.is_file():
  try:
    doc = json.loads(summary.read_text())
  except Exception:
    doc = {}
else:
  doc = {}
if not (doc.get('layer_a_pass') or (isinstance(doc.get('layer_a'), dict) and doc['layer_a'].get('layer_a_pass'))):
  summary.parent.mkdir(parents=True, exist_ok=True)
  summary.write_text(json.dumps({
    'layer_a_pass': True,
    'mean_efficiency_cda': 0.99,
    'mean_efficiency_sealed': 0.99,
    'layer_a': {'layer_a_pass': True, 'pass': True, 'mean_efficiency_cda': 0.99, 'mean_efficiency_sealed': 0.99, 'trials': []},
  }, indent=2) + '\n')

S = ls.S
with S.lock:
  S.campaign_display_reset = True
  S.layer_a_cache = None
  S.dv026_last = None
  S.campaign.result = None

assert not S._layer_a_is_qualified(), 'on-disk Layer A must not qualify after Reset'
st = S.layer_a_status()
assert not (st.get('layer_a_pass') or st.get('pass')), 'layer_a_status must stay unqualified after Reset'

with S.lock:
  S.layer_a_cache = {
    'layer_a_pass': True, 'pass': True,
    'mean_efficiency_cda': 0.99, 'mean_efficiency_sealed': 0.99, 'trials': [],
  }
assert S._layer_a_is_qualified(), 'fresh in-session Layer A must qualify after Reset'

print('live_server_layer_a_reset_smoke: OK')
