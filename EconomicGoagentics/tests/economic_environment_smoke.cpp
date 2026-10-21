#include "coagentics/domain/EconomicEnvironment.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>

using namespace coagentics::domain;
using namespace coagentics::market;

static std::unique_ptr<EconomicEnvironment> make_env(MechanismKind kind = MechanismKind::ContinuousDoubleAuction) {
    EconomicEnvironment::Config c;
    c.scenario_id = "section4-market";
    c.scenario_version = "1";
    c.default_asset = "ASSET";
    c.mechanism = kind;
    c.market.fundamental_value["ASSET"] = 100.0;
    c.market.total_supply["ASSET"] = 10;
    auto env = std::make_unique<EconomicEnvironment>(c);
    env->add_account("buyer", {1000.0, {}});
    env->add_account("seller", {0.0, {{"ASSET", 5}}});
    return env;
}

int main() {
    auto env = make_env();
    RunSpec run{"run-s4", "exp-s4", 7, "control", "development"};
    auto world = env->reset(run);
    assert(world.tick == 0);

    env->publish_information({"news-1", "ASSET", 0, "supply disruption", 0.9, 1.0, std::nullopt, std::nullopt, std::nullopt, "public-news"});

    AgentRef buyer{"buyer", "buyer", "ref", "reference", "1"};
    InformationPolicy policy;
    policy.allow_public_news = true;
    policy.allow_price_history = true;
    policy.allow_private_state = true;
    auto obs = env->observe(buyer, policy);
    assert(obs.information.size() == 1);
    assert(obs.private_fields.at("cash") == "1000");

    AgentInvocation inv{"run-s4:req-1", buyer, obs, 7, "cfg"};
    Action buy;
    buy.action_id = "a1";
    buy.action_type = "BUY";
    buy.numeric_parameters["quantity"] = 2;
    buy.numeric_parameters["price"] = 101;
    buy.categorical_parameters["asset"] = "ASSET";

    auto valid = env->validate(inv, buy);
    assert(valid.accepted && !valid.abstained);
    auto tr = env->step(inv, valid);
    assert(tr.outcomes.at("accepted") == 1.0);
    assert(tr.outcomes.at("filled_quantity") == 0.0 || tr.outcomes.at("filled_quantity") == 2.0);
    assert(tr.next_world.tick == 1);

    // Invalid domain action: buyer cannot afford this order.
    Action too_expensive = buy;
    too_expensive.numeric_parameters["quantity"] = 20;
    too_expensive.numeric_parameters["price"] = 100;
    auto invalid = env->validate(inv, too_expensive);
    assert(!invalid.accepted);
    assert(invalid.rejection_reason == "insufficient_cash");
    auto rejected = env->step(inv, invalid);
    assert(rejected.outcomes.at("accepted") == 0.0);
    assert(rejected.outcomes.at("filled_quantity") == 0.0);

    // HOLD is an explicit abstention and advances the world without execution.
    Action hold; hold.action_type = "HOLD";
    auto held = env->validate(inv, hold);
    assert(held.accepted && held.abstained);
    auto hold_transition = env->step(inv, held);
    assert(hold_transition.outcomes.at("abstained") == 1.0);

    auto sealed = make_env(MechanismKind::SealedBidDoubleAuction);
    sealed->reset(run);
    auto sobs = sealed->observe(buyer, policy);
    AgentInvocation sinv{"run-s4:req-2", buyer, sobs, 7, "cfg"};
    auto sv = sealed->validate(sinv, buy);
    assert(sv.accepted);
    auto st = sealed->step(sinv, sv);
    assert(st.next_world.tick == 1);

    std::cout << "section4 economic environment PASS"
              << " trades=" << env->trades().size()
              << " efficiency=" << env->mechanism().metrics().allocative_efficiency << "\n";
}
