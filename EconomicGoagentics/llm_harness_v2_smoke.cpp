#include "coagentics/llm/LlmHarnessV2.hpp"
#include <cassert>
#include <iostream>

using namespace coagentics::llm;

int main() {
    ModelRegistry registry;
    registry.register_model({"provider-a", "model-a", "2026-01", "mock://a"});
    registry.register_model({"provider-b", "model-b", "2026-02", "mock://b"});
    registry.set_status("provider-a", "model-a", "2026-01", Availability::Available);
    registry.set_status("provider-b", "model-b", "2026-02", Availability::Available);

    LlmHarness h(registry);
    h.set_budget({2,0,0.0,0.0});
    int calls = 0;
    h.bind_provider("provider-a", [&](const LlmRequest& r) {
        ++calls;
        LlmResponse x; x.status = InvocationStatus::Executed; x.raw_text = "BUY 1 at 102";
        x.input_tokens = r.canonical_observation.size(); x.output_tokens = 4; x.estimated_cost_usd = 0.01;
        return x;
    });

    LlmRequest r;
    r.model = {"provider-a","model-a","2026-01","mock://a"};
    r.canonical_observation = "price=100|ask=102|signal=positive";
    r.observation_fingerprint = LlmHarness::fingerprint(r.canonical_observation);
    r.run_id = "RUN-1";
    r.config.deterministic = true;

    auto a = h.invoke(r);
    assert(a.status == InvocationStatus::Executed);
    assert(calls == 1);
    auto b = h.invoke(r);
    assert(b.status == InvocationStatus::CacheHit);
    assert(calls == 1);
    assert(b.from_cache);

    auto r2 = r; r2.run_id = "RUN-2";
    auto c = h.invoke(r2);
    assert(c.status == InvocationStatus::Executed);
    assert(calls == 2);

    auto r3 = r; r3.run_id = "RUN-3";
    auto d = h.invoke(r3);
    assert(d.status == InvocationStatus::BudgetDenied);

    registry.set_status("provider-b", "model-b", "2026-02", Availability::Unavailable, "offline");
    LlmRequest unavailable = r; unavailable.model = {"provider-b","model-b","2026-02","mock://b"}; unavailable.run_id="RUN-U";
    auto u = h.invoke(unavailable);
    assert(u.status == InvocationStatus::Unavailable);

    assert(!h.records().empty());
    std::cout << "llm_harness_v2_smoke: PASS\n";
    return 0;
}
