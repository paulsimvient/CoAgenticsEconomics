#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace coagentics::agents {

enum class MemoryMode { None, Rolling, Episodic };
const char* memory_mode_name(MemoryMode);

struct MemoryPolicy {
    MemoryMode mode{MemoryMode::Rolling};
    std::size_t max_records{8};
    std::size_t episode_size{4};
};

struct AgentExperience {
    std::uint64_t tick{};
    std::string observation_fingerprint;
    std::string action;
    bool action_valid{false};
    std::string outcome;
};

struct MemoryRecord {
    std::uint64_t first_tick{};
    std::uint64_t last_tick{};
    std::size_t experience_count{};
    std::string summary;
};

class PersistentMemory {
public:
    explicit PersistentMemory(MemoryPolicy policy = {});

    void remember(const AgentExperience& experience);
    void clear();

    const MemoryPolicy& policy() const noexcept { return policy_; }
    const std::vector<AgentExperience>& raw_history() const noexcept { return history_; }
    const std::vector<MemoryRecord>& episodes() const noexcept { return episodes_; }

    std::vector<AgentExperience> visible_history() const;
    std::string context_text() const;

private:
    MemoryPolicy policy_;
    std::vector<AgentExperience> history_;
    std::vector<MemoryRecord> episodes_;

    void rebuild_episodes();
};

} // namespace coagentics::agents
