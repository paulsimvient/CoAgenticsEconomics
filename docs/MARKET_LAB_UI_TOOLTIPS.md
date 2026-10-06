# Market Lab UI — Tooltip Coverage

Every major scientific concept, configuration control, workflow step, result metric, evidence stage, and data-table column has a compact `?` affordance or native title tooltip.

The tooltip system is attached to the real `workbench/market_lab.html` DOM and survives page re-rendering through a MutationObserver. It does not change experiment semantics or fabricate data.

Design rule: the UI remains concise; explanatory text appears on demand.
