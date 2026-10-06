# H01 metric.operational_definition bug fix

The persisted H01 record used the legacy metric field `metric.definition` while the current DV026 research-expectations schema requires `metric.operational_definition`.

Fixes:
- Added a backward-compatible migration on server load/save/freeze: `definition` -> `operational_definition` when the canonical field is absent.
- Removed the legacy alias after migration so the canonical schema is persisted.
- Added editable UI fields for metric name, operational definition, unit, population, and aggregation.
- Added the fields to the existing tooltip system through the `.field label` selector.
- Validated the current H01 after migration against the existing server schema.

No experimental results or scientific values were fabricated or changed. The existing H01 wording is preserved; only the schema field name is normalized.
