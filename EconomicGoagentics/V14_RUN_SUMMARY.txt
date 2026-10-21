# V14 Behavioral Phenotyping

**Live LLMs used:** NO — known-mechanism controls validate feature recovery before live-model use.

| Mechanism | News sens. | Latency | Peer susc. | Adaptation | Recovery | Influence proxy | Utility proxy |
|---|---:|---:|---:|---:|---:|---:|---:|
| information-blind | 0.000 | -1 | 0.000 | 0.000 | 1.000 | 0.000 | 1.000 |
| signal-responsive | 1.000 | 0 | 0.000 | 0.000 | 1.000 | 0.550 | 1.000 |
| risk-sensitive | 0.750 | 0 | 0.000 | 0.000 | 1.000 | 0.413 | 1.000 |
| peer-responsive | 0.000 | -1 | 0.600 | 0.000 | 1.000 | 0.270 | 1.000 |
| adaptive | 1.000 | 0 | 0.200 | 0.538 | 1.000 | 0.640 | 1.000 |

## Competing explanations and discriminators

### information-blind
- H1 direct information response: support=0.000; discriminator: information ON / peer visibility OFF
- H2 peer-mediated response: support=0.000; discriminator: information OFF / peer visibility ON
- H3 adaptive state update: support=0.000; discriminator: repeat perturbation then withdraw it
- H4 market/institution artifact: support=1.000; discriminator: replace focal policy with information-blind matched control
- **Next experiment:** replace focal policy with information-blind matched control

### signal-responsive
- H1 direct information response: support=1.000; discriminator: information ON / peer visibility OFF
- H2 peer-mediated response: support=0.000; discriminator: information OFF / peer visibility ON
- H3 adaptive state update: support=0.000; discriminator: repeat perturbation then withdraw it
- H4 market/institution artifact: support=0.000; discriminator: replace focal policy with information-blind matched control
- **Next experiment:** information ON / peer visibility OFF

### risk-sensitive
- H1 direct information response: support=0.750; discriminator: information ON / peer visibility OFF
- H2 peer-mediated response: support=0.000; discriminator: information OFF / peer visibility ON
- H3 adaptive state update: support=0.000; discriminator: repeat perturbation then withdraw it
- H4 market/institution artifact: support=0.250; discriminator: replace focal policy with information-blind matched control
- **Next experiment:** information ON / peer visibility OFF

### peer-responsive
- H1 direct information response: support=0.000; discriminator: information ON / peer visibility OFF
- H2 peer-mediated response: support=0.600; discriminator: information OFF / peer visibility ON
- H3 adaptive state update: support=0.000; discriminator: repeat perturbation then withdraw it
- H4 market/institution artifact: support=0.400; discriminator: replace focal policy with information-blind matched control
- **Next experiment:** information OFF / peer visibility ON

### adaptive
- H1 direct information response: support=1.000; discriminator: information ON / peer visibility OFF
- H2 peer-mediated response: support=0.200; discriminator: information OFF / peer visibility ON
- H3 adaptive state update: support=1.000; discriminator: repeat perturbation then withdraw it
- H4 market/institution artifact: support=0.000; discriminator: replace focal policy with information-blind matched control
- **Next experiment:** information ON / peer visibility OFF

## Interpretation boundary
Behavioral fingerprints are mechanism-recovery controls, not live-LLM findings. Efficiency is intentionally absent from the fingerprint: v13 showed that institutionally disciplined markets can hide distinct policies behind similar efficiency. Operational failure fields remain zero for these in-process controls and must be populated from the v8 provider harness for live models.
