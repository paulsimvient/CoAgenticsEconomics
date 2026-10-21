#include "coagentics/agents/PersistentMemoryAgent.hpp"
#include <utility>

namespace coagentics::agents {

PersistentMemoryAgent::PersistentMemoryAgent(MemoryPolicy policy, std::string agent_id)
    : agent_id_(std::move(agent_id)), memory_(policy) {}

std::string PersistentMemoryAgent::context_for_next_decision() const {
    return "agent_id=" + agent_id_ + "\n" + memory_.context_text();
}

void PersistentMemoryAgent::record_decision(std::uint64_t tick,
                                            const std::string& observation_fingerprint,
                                            const std::string& action,
                                            bool action_valid,
                                            const std::string& outcome) {
    memory_.remember(AgentExperience{tick, observation_fingerprint, action, action_valid, outcome});
}

} // namespace coagentics::agents
