#include "coagentics/domain/AgentBoundary.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace coagentics::domain;

static AgentInvocation invocation() {
    AgentInvocation i;
    i.request_id = "run-1:req-1";
    i.agent = {"agent-1","buyer","provider","model","v1"};
    i.request_seed = 424242;
    i.observation.world = {"market","v1",424242,7};
    i.observation.agent = i.agent;
    i.observation.public_fields["best_ask"] = "102";
    i.observation.public_fields["last_trade"] = "100";
    i.observation.policy.allow_public_news = true;
    return i;
}

int main() {
    ActionValidationPolicy p;
    p.allowed_action_types = {"BUY","SELL","HOLD","ABSTAIN"};
    p.max_abs_numeric_parameter = 1e6;
    ActionBoundary boundary(p);

    ReferenceAgentAdapter good("reference", [](const AgentInvocation&) {
        Action a; a.action_type = "BUY"; a.numeric_parameters["quantity"] = 2; a.numeric_parameters["price"] = 101;
        a.categorical_parameters["asset"] = "ASSET"; return a;
    });
    auto t1 = boundary.invoke("run-1", invocation(), good);
    assert(t1.status == InvocationStatus::Accepted);
    assert(!t1.observation_fingerprint.empty() && !t1.action_fingerprint.empty());

    ReferenceAgentAdapter bad("bad", [](const AgentInvocation&) {
        Action a; a.action_type = "HACK"; a.numeric_parameters["quantity"] = std::nan("1"); return a;
    });
    auto t2 = boundary.invoke("run-1", invocation(), bad);
    assert(t2.status == InvocationStatus::Rejected);
    assert(!t2.validation.errors.empty());

    ReferenceAgentAdapter hold("hold", [](const AgentInvocation&) {
        Action a; a.action_type = "HOLD"; return a;
    });
    auto t3 = boundary.invoke("run-1", invocation(), hold);
    assert(t3.status == InvocationStatus::Abstained);
    assert(t3.validation.abstained);

    ReferenceAgentAdapter throws("throws", [](const AgentInvocation&) -> Action { throw std::runtime_error("offline"); });
    auto t4 = boundary.invoke("run-1", invocation(), throws);
    assert(t4.status == InvocationStatus::ProviderFailure);
    assert(t4.failure_reason == "offline");

    std::cout << "accepted=" << invocation_status_name(t1.status)
              << " rejected=" << invocation_status_name(t2.status)
              << " abstained=" << invocation_status_name(t3.status)
              << " failure=" << invocation_status_name(t4.status) << "\n";
}
