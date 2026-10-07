/** Shared DV026 workbench fetch helpers (Interface + Research Console). */
(function (global) {
  'use strict';

  async function api(path, opt, timeoutMs) {
    const ms = timeoutMs == null ? 15000 : timeoutMs;
    const ctl = new AbortController();
    const timer = setTimeout(() => ctl.abort(), ms);
    try {
      const r = await fetch(path, {
        cache: 'no-store',
        ...opt,
        signal: ctl.signal,
        headers: { 'Content-Type': 'application/json', ...(opt && opt.headers) },
      });
      const d = await r.json().catch(() => ({ error: 'Invalid JSON' }));
      if (!r.ok || d.error) throw new Error(d.error || ('HTTP ' + r.status));
      return d;
    } catch (e) {
      if (e && e.name === 'AbortError') {
        throw new Error('Request timed out after ' + Math.round(ms / 1000) + 's: ' + path);
      }
      throw e;
    } finally {
      clearTimeout(timer);
    }
  }

  function get(path, timeoutMs) {
    return api(path, {}, timeoutMs == null ? 10000 : timeoutMs);
  }

  function post(path, body, timeoutMs) {
    return api(
      path,
      { method: 'POST', body: JSON.stringify(body || {}) },
      timeoutMs == null ? 15000 : timeoutMs
    );
  }

  global.CoAgenticsApi = { api, get, post };
})(typeof window !== 'undefined' ? window : globalThis);
