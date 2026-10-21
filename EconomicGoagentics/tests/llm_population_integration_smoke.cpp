#include "coagentics/abm/AdvancedPopulation.hpp"
#include "coagentics/domain/EconomicEnvironment.hpp"
#include "coagentics/domain/LlmAgentAdapterV2.hpp"
#include "coagentics/llm/LlmHarnessV2.hpp"

#include <cassert>
#include <iostream>
#include <memory>

using namespace coagentics::domain;
using namespace coagentics::market;

static std::unique_ptr<EconomicEnvironment> make_env() {
    EconomicEnvironment::Config c;
    c.scenario_id = "section8-market";
    c.scenario_version = "1";
    c.default_asset = "ASSET";
    c.market.fundamental_value["ASSET"] = 100.0;
    c.market.total_supply["ASSET"] = 10;
    auto env = std::make_unique<EconomicEnvironment>(c);
    env->add_account("llm-buyer", {1000.0, {}});
    env->add_account("reference-seller", {0.0, {{"ASSET", 5}}});
    return env;
}

int main() {
    coagentics::llm::ModelRegistry registry;
    registry.register_model({"mock-provider", "research-model", "2026.10", "mock"});
    registry.set_status("mock-provider", "research-model", "2026.10",
                        coagentics::llm::Availability::Available, "smoke-test");
    coagentics::llm::LlmHarness harness(registry);
    harness.bind_provider("mock-provider", [](const coagentics::llm::LlmRequest&) {
        coagentics::llm::LlmResponse r;
        r.status = coagentics::llm::InvocationStatus::Executed;
        r.raw_text = R"({"action":"BUY","time":0,"asset":"ASSET","quantity":1,"price":101})";
        r.input_tokens = 10;
        r.output_tokens = 8;
        r.estimated_cost_usd = 0.001;
        return r;
    });
    harness.set_budget({20, 0, 1.0, 0.0});

    auto llm = std::make_unique<LlmAgentAdapterV2>(
        harness,
        coagentics::llm::ModelIdentity{"mock-provider", "research-model", "2026.10", "mock"});

    ReferenceAgentAdapter seller("reference-seller", [](const AgentInvocation& inv) {
        Action a;
        a.action_type = "SELL";
        a.numeric_parameters["time"] = static_cast<double>(inv.observation.world.tick);
        a.numeric_parameters["quantity"] = 1;
        a.numeric_parameters["price"] = 99;
        a.categorical_parameters["asset"] = "ASSET";
        return a;
    });

    AgentRef buyer_ref{"llm-buyer", "buyer", "mock-provider", "research-model", "2026.10"};
    AgentRef seller_ref{"reference-seller", "seller", "reference", "rule-agent", "1"};
    InformationPolicy buyer_policy;
    buyer_policy.allow_public_news = true;
    buyer_policy.allow_price_history = true;
    buyer_policy.allow_peer_actions = false;
    buyer_policy.allow_private_state = true;
    InformationPolicy seller_policy = buyer_policy;

    std::vector<coagentics::abm::PopulationAgent> agents;
    agents.push_back({buyer_ref, buyer_policy, llm.get(), true});
    agents.push_back({seller_ref, seller_policy, &seller, true});

    auto env = make_env();
    coagentics::domain::ActionValidationPolicy p;
    p.allowed_action_types = {"BUY", "SELL", "HOLD", "ABSTAIN"};
    coagentics::abm::PopulationRunner runner{coagentics::domain::ActionBoundary{p}};
    coagentics::abm::PopulationRunSpec spec;
    spec.run = {"run-s8", "exp-s8", 424242, "mixed", "development"};
    spec.activation = coagentics::abm::ActivationDesign::SequentialInteraction;
    spec.ticks = 2;
    spec.activation_seed = 17;

    auto result = runner.run(*env, std::move(agents), spec);
    assert(result.turns.size() == 4);
    assert(!harness.records().empty());
    assert(harness.records().front().response.status == coagentics::llm::InvocationStatus::Executed);
    assert(harness.records().front().response.raw_text.find("BUY") != std::string::npos);

    bool saw_llm_action = false;
    bool saw_reference_action = false;
    for (const auto& t : result.turns) {
        if (t.turn.agent.agent_id == "llm-buyer") {
            saw_llm_action = true;
            assert(t.turn.proposed_action.action_type == "BUY");
            assert(t.result.accepted);
        }
        if (t.turn.agent.agent_id == "reference-seller") {
            saw_reference_action = true;
            assert(t.turn.proposed_action.action_type == "SELL");
            assert(t.result.accepted);
        }
    }
    assert(saw_llm_action && saw_reference_action);

    std::cout << "section8 llm population integration PASS"
              << " turns=" << result.turns.size()
              << " llm_invocations=" << harness.records().size()
              << " trades=" << env->trades().size() << "\n";
}
