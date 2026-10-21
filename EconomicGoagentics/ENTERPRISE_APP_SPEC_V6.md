# CoAgentics Economics — Enterprise Application V6

## Product posture
CoAgentics is presented as an enterprise research workbench, not a consumer dashboard. The application uses workspace/project context, persistent research navigation, command search, role context, audit affordances, environment/build context, and explicit evidence boundaries.

## Enterprise shell
- Global top bar: product identity, workspace/project selector, global command/search, help/activity, user/role context.
- Persistent left navigation grouped as Research, Assurance, Platform.
- Workspace breadcrumb/context strip.
- Persistent environment/build footer.
- Responsive navigation behavior.

## Research workflow
Overview → Experiment Builder → Run & Replay → Agents & Traces → Explanations → Memory → Compare → Evidence → Validation & Claims.

## Trust model
- Backend remains authoritative for observed state, experiment status, evidence, validation and claims.
- UI must not invent market events, prices, trades, causal results, or validation outcomes.
- Enterprise affordances are presentational until backed by identity/access/audit APIs.

## Enterprise-ready integration targets
1. SSO/OIDC/SAML identity provider.
2. Workspace/project RBAC.
3. Immutable audit-event API.
4. Server-side evidence retention and export policy.
5. Environment separation (development/staging/production).
6. API/service-account management.
7. Centralized telemetry and health endpoints.
8. Multi-user experiment locking/versioning.
9. Approval workflow for registered designs and claims.
10. Organization-level policy controls.

## Communication Layer — V18 design requirement

The enterprise application must include a governed LLM communications layer as specified in `docs/COAGENTICS_COMMUNICATION_PROTOCOL_V18.md`. The LLM is a communications/navigation mechanism over authoritative research objects and evidence. It is not the source of truth. The implementation must use explicit context resolution, structured research-object references, epistemic classes, response validation, fail-closed behavior, and server-side action authorization. Qualification is defined in `docs/LLM_QUALIFICATION_V18.md`.
