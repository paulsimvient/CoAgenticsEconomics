#include "coagentics/agents/PersistentMemoryAgent.hpp"
#include <sstream>
#include <utility>

namespace coagentics::agents {
PersistentMemoryAgent::PersistentMemoryAgent(MemoryPolicy policy, std::string agent_id)
    : agent_id_(std::move(agent_id)), memory_(policy) {}

std::string PersistentMemoryAgent::context_for_next_decision() const {
    std::ostringstream out;
    out << "agent_id=" << agent_id_ << "\n" << memory_.context_text();
    return out.str();
}

void PersistentMemoryAgent::record_decision(std::uint64_t tick,
                                            const std::string& observation_fingerprint,
                                            const std::string& action,
                                            bool action_valid,
                                            const std::string& outcome) {
    memory_.remember({tick, observation_fingerprint, action, action_valid, outcome});
}
} // namespace coagentics::agents
