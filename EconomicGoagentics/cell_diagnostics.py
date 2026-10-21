"""Conservative, evidence-backed per-cell diagnostic classification.

A valid HOLD is not itself a failed experiment or proof of irrationality.
"""

def diagnose_cell(row):
    """Return an explanatory diagnostic without changing scientific pass/fail gates."""
    if not isinstance(row, dict):
        return {'code': 'UNKNOWN', 'explanation': 'No persisted cell summary available.'}
    if row.get('skipped'):
        return {'code': 'SKIPPED', 'explanation': 'Cell was skipped; no execution outcome established.'}
    error = str(row.get('error') or '').strip()
    if error:
        return {'code': 'EXECUTION_ERROR', 'explanation': 'Runner reported an error: ' + error[:300]}
    checks = (
        ('interface_ok', 'INTERFACE_FAILURE', 'Provider/interface contract check failed.'),
        ('parse_ok', 'PARSE_FAILURE', 'Model response could not be parsed.'),
        ('action_valid', 'INVALID_ACTION', 'Parsed action failed action validation.'),
        ('market_action_ok', 'MARKET_ACTION_FAILURE', 'Market action check failed.'),
        ('market_accepted', 'NOT_ACCEPTED', 'Market did not accept the submitted action.'),
        ('execution_observed', 'EXECUTION_NOT_OBSERVED', 'Required execution evidence was not observed.'),
        ('replay_ok', 'REPLAY_FAILURE', 'Deterministic replay verification failed.'),
        ('cross_event_valid', 'CROSS_EVENT_FAILURE', 'Cross-event evidence validation failed.'),
    )
    for key, code, explanation in checks:
        if row.get(key) is False:
            return {'code': code, 'explanation': explanation}
    if row.get('cell_ok') is True:
        return {'code': 'PASS', 'explanation': 'All required persisted cell gates passed.'}
    if row.get('cell_ok') is False:
        return {'code': 'FAILED_UNRESOLVED', 'explanation': 'Cell failed, but persisted summary does not identify a decisive failing gate. Inspect trace evidence.'}
    return {'code': 'UNVERIFIED', 'explanation': 'No confirmed cell outcome. Inspect persisted evidence.'}
