#pragma once

#include "coagentics/domain/AgentBoundary.hpp"
#include "coagentics/domain/AdvancedContracts.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace coagentics::abm {

enum class ActivationDesign { FrozenSnapshot, SequentialInteraction };

struct PopulationAgent {
    domain::AgentRef ref;
    domain::InformationPolicy information_policy;
    domain::AgentAdapter* adapter{nullptr};
    bool active{true};
};

struct AgentHistoryEntry {
    domain::AgentTurn turn;
    domain::ActionResult result;
    domain::TransitionResult transition;
};

struct PopulationState {
    std::uint64_t tick{0};
    std::vector<PopulationAgent> agents;
    std::map<std::string, std::vector<AgentHistoryEntry>> history;
};

struct PopulationRunSpec {
    domain::RunSpec run;
    ActivationDesign activation{ActivationDesign::SequentialInteraction};
    std::uint64_t ticks{1};
    std::uint64_t activation_seed{0};
};

struct PopulationRunResult {
    PopulationState final_state;
    std::vector<std::string> execution_order;
    std::vector<AgentHistoryEntry> turns;
};

class PopulationRunner {
public:
    explicit PopulationRunner(domain::ActionBoundary boundary = domain::ActionBoundary{});

    PopulationRunResult run(domain::DomainEnvironment& environment,
                             std::vector<PopulationAgent> agents,
                             const PopulationRunSpec& spec);

private:
    domain::ActionBoundary boundary_;
};

const char* activation_design_name(ActivationDesign design);

} // namespace coagentics::abm
