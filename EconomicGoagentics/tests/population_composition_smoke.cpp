#include "coagentics/abm/PopulationComposition.hpp"
#include <cassert>
#include <iostream>
#include <set>

using namespace coagentics::abm;

int main() {
    PopulationCompositionSpec a;
    a.seed = 424242;
    a.seats = 12;
    a.models = {
        {"openai", "model-a", "v1"},
        {"anthropic", "model-b", "v2"},
        {"google", "model-c", "v3"},
        {"local", "model-d", "v4"}
    };
    a.roles = {
        {"buyer", "buyer"}, {"seller", "seller"}, {"buyer", "buyer"}, {"seller", "seller"}
    };
    a.reference_strategies = {
        ReferenceStrategy::FixedPrice,
        ReferenceStrategy::SignalResponsive,
        ReferenceStrategy::RiskSensitive,
        ReferenceStrategy::PeerResponsive
    };
    a.reservation_values = {90.0, 100.0, 110.0, 120.0};

    auto first = balanced_assignments(a);
    assert(first.size() == 12);
    assert(first[0].seat_id == "SEAT-0");
    assert(first[0].model == "model-a" || first[0].model == "model-b" || first[0].model == "model-c" || first[0].model == "model-d");

    std::set<std::string> models;
    for (const auto& x : first) models.insert(x.model);
    assert(models.size() == 4); // repeated seats preserve the full model catalog

    auto composed = compose_population(a);
    assert(composed.agents.size() == 12);
    assert(composed.owned_adapters.size() == 12);
    assert(!composed.assignment_fingerprint.empty());
    for (const auto& agent : composed.agents) assert(agent.adapter != nullptr);

    auto b = a;
    b.seed = 424243;
    auto second = balanced_assignments(b);
    bool model_assignment_changed = false;
    for (std::size_t i = 0; i < first.size(); ++i)
        if (first[i].model != second[i].model) { model_assignment_changed = true; break; }
    assert(model_assignment_changed);

    auto bad = a;
    bad.models.clear();
    bool rejected = false;
    try { (void)balanced_assignments(bad); } catch (...) { rejected = true; }
    assert(rejected);

    std::cout << "population_composition_smoke: PASS\n";
    return 0;
}
