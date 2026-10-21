#pragma once

#include "coagentics/agents/PersistentMemory.hpp"
#include <cstdint>
#include <string>

namespace coagentics::agents {

// Section 16 integration boundary: one explicit memory policy per agent.
class PersistentMemoryAgent {
public:
    explicit PersistentMemoryAgent(MemoryPolicy policy = {}, std::string agent_id = {});

    const std::string& agent_id() const noexcept { return agent_id_; }
    const PersistentMemory& memory() const noexcept { return memory_; }
    PersistentMemory& memory() noexcept { return memory_; }

    // Text that may be appended to the canonical model observation.
    std::string context_for_next_decision() const;

    // Called only after the proposed action has been parsed, validated, and executed.
    void record_decision(std::uint64_t tick,
                         const std::string& observation_fingerprint,
                         const std::string& action,
                         bool action_valid,
                         const std::string& outcome);

    void clear() { memory_.clear(); }

private:
    std::string agent_id_;
    PersistentMemory memory_;
};

} // namespace coagentics::agents
