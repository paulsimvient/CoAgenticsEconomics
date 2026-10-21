#include "coagentics/domain/AdvancedContracts.hpp"
#include <cassert>
#include <string>

int main() {
    using namespace coagentics::domain;

    InformationEvent event{
        "news-001", "ASSET-A", 10, "Supply disruption reported", 0.82, 0.95,
        12, 100, std::nullopt, "public-news"
    };
    std::string why;
    assert(valid_information_event(event, &why));

    auto bad = event;
    bad.reliability = 1.1;
    assert(!valid_information_event(bad, &why));
    assert(why == "reliability_out_of_range");

    RunSpec run{"run-001", "exp-001", 42, "news-high-reliability", "discovery"};
    assert(valid_run_spec(run, &why));

    run.group = "heldout";
    assert(!valid_run_spec(run, &why));
    assert(why == "invalid_run_group");

    // The key architectural property: the observation is constructed from a
    // policy rather than exposing the whole world by default.
    Observation observation;
    observation.world = {"economic-market", "v1", 42, 10};
    observation.agent = {"agent-7", "buyer", "provider", "model-a", "v1"};
    observation.policy.allow_peer_actions = false;
    observation.public_fields["last_price"] = "101.25";
    assert(!observation.policy.allow_peer_actions);

    return 0;
}
