# DV026 Ollama Layer B Campaign

**darpa_claim_ready:** true  
**scope:** local_ollama_poc  
**Layer A pass:** yes  
**Available models:** 10 / catalog 10  
**Cells ok:** 200 / attempted 200  
**Distinct live models with ≥1 ok cell:** 10

## Readiness checklist

- [x] **layer_a** — trials=5 cda=100.000000 sealed=100.000000 required_trials>=5 rule=every_trial>90
- [x] **live_count** — distinct_ok=10 required>=10
- [x] **coverage** — models_with_>=20 seeds=10 required>=10
- [x] **provenance** — rate=1.000000 required>=0.950000 cells=200
- [x] **interface_health** — parse+valid rate=1.000000 required>=0.900000
- [x] **market_action** — non-HOLD action rate=1.000000 required>=0.800000 (HOLD-only does not qualify as economic execution)
- [x] **market_acceptance** — accepted/non-HOLD rate=1.000000 required>=0.800000
- [x] **human_ref** — HumanComparison DESCRIPTIVE on live allocative_efficiency (provenance-gated)
- [x] **non_claims** — Report forbids Phase II construct validation and commercial 10-provider claim without second matrix

Even when darpa_claim_ready=true under scope=local_ollama_poc: no Phase II construct validation, no classifier accuracy claim (FAQ 31), no commercial multi-provider matrix claim.
