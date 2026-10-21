#include "coagentics/abm/AdvancedPopulation.hpp"

#include <algorithm>
#include <numeric>
#include <random>
#include <stdexcept>

namespace coagentics::abm {

const char* activation_design_name(ActivationDesign design) {
    return design == ActivationDesign::FrozenSnapshot ? "frozen_snapshot" : "sequential_interaction";
}

PopulationRunner::PopulationRunner(domain::ActionBoundary boundary)
    : boundary_(std::move(boundary)) {}

PopulationRunResult PopulationRunner::run(domain::DomainEnvironment& environment,
                                           std::vector<PopulationAgent> agents,
                                           const PopulationRunSpec& spec) {
    std::string why;
    if (!domain::valid_run_spec(spec.run, &why))
        throw std::invalid_argument("invalid_population_run: " + why);
    if (spec.ticks == 0) throw std::invalid_argument("population_ticks_must_be_positive");
    if (agents.empty()) throw std::invalid_argument("population_must_not_be_empty");
    for (const auto& a : agents) {
        if (a.ref.agent_id.empty()) throw std::invalid_argument("population_agent_missing_id");
        if (!a.adapter) throw std::invalid_argument("population_agent_missing_adapter: " + a.ref.agent_id);
    }

    PopulationRunResult result;
    result.final_state.agents = agents;
    environment.reset(spec.run);

    std::mt19937_64 rng(spec.activation_seed == 0 ? spec.run.seed : spec.activation_seed);

    for (std::uint64_t local_tick = 0; local_tick < spec.ticks; ++local_tick) {
        std::vector<std::size_t> order(agents.size());
        std::iota(order.begin(), order.end(), 0);
        std::shuffle(order.begin(), order.end(), rng);

        if (spec.activation == ActivationDesign::FrozenSnapshot) {
            // All agents receive observations from the same world tick before any
            // action is submitted. The actions are then applied in the randomized
            // order. This separates common-state response from sequential feedback.
            struct Pending { std::size_t index; domain::AgentInvocation invocation; };
            std::vector<Pending> pending;
            pending.reserve(order.size());
            for (std::size_t index : order) {
                auto& a = agents[index];
                auto observation = environment.observe(a.ref, a.information_policy);
                domain::AgentInvocation invocation{
                    spec.run.run_id + ":t" + std::to_string(local_tick) + ":" + a.ref.agent_id,
                    a.ref, observation, spec.run.seed + local_tick, "population"};
                pending.push_back({index, std::move(invocation)});
            }
            for (auto& p : pending) {
                auto& a = agents[p.index];
                auto turn = boundary_.invoke(spec.run.run_id, p.invocation, *a.adapter);
                // The domain performs the authoritative validation because legality
                // depends on current world state (cash, inventory, market rules).
                auto authoritative = environment.validate(p.invocation, turn.proposed_action);
                auto transition = environment.step(p.invocation, authoritative);
                AgentHistoryEntry entry{std::move(turn), authoritative, transition};
                result.execution_order.push_back(a.ref.agent_id);
                result.final_state.history[a.ref.agent_id].push_back(entry);
                result.turns.push_back(entry);
            }
        } else {
            // Sequential interaction: each next agent observes the state after the
            // preceding agent's transition. This intentionally permits peer effects.
            for (std::size_t index : order) {
                auto& a = agents[index];
                auto observation = environment.observe(a.ref, a.information_policy);
                domain::AgentInvocation invocation{
                    spec.run.run_id + ":t" + std::to_string(local_tick) + ":" + a.ref.agent_id,
                    a.ref, observation, spec.run.seed + local_tick, "population"};
                auto turn = boundary_.invoke(spec.run.run_id, invocation, *a.adapter);
                auto authoritative = environment.validate(invocation, turn.proposed_action);
                auto transition = environment.step(invocation, authoritative);
                AgentHistoryEntry entry{std::move(turn), authoritative, transition};
                result.execution_order.push_back(a.ref.agent_id);
                result.final_state.history[a.ref.agent_id].push_back(entry);
                result.turns.push_back(entry);
            }
        }
        result.final_state.tick = environment.observe(agents.front().ref, agents.front().information_policy).world.tick;
    }
    return result;
}

} // namespace coagentics::abm
