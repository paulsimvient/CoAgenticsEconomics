#include "coagentics/agents/PersistentMemory.hpp"
#include <algorithm>
#include <sstream>

namespace coagentics::agents {

const char* memory_mode_name(MemoryMode mode) {
    switch (mode) {
        case MemoryMode::None: return "none";
        case MemoryMode::Rolling: return "rolling";
        case MemoryMode::Episodic: return "episodic";
    }
    return "unknown";
}

PersistentMemory::PersistentMemory(MemoryPolicy policy) : policy_(policy) {
    if (policy_.mode == MemoryMode::Rolling && policy_.max_records == 0) policy_.max_records = 1;
    if (policy_.mode == MemoryMode::Episodic && policy_.episode_size == 0) policy_.episode_size = 1;
}

void PersistentMemory::remember(const AgentExperience& experience) {
    history_.push_back(experience);
    if (policy_.mode == MemoryMode::None) {
        history_.clear();
        episodes_.clear();
        return;
    }
    if (policy_.mode == MemoryMode::Rolling && history_.size() > policy_.max_records) {
        const auto excess = history_.size() - policy_.max_records;
        history_.erase(history_.begin(), history_.begin() + static_cast<std::ptrdiff_t>(excess));
    }
    rebuild_episodes();
}

void PersistentMemory::clear() {
    history_.clear();
    episodes_.clear();
}

std::vector<AgentExperience> PersistentMemory::visible_history() const {
    if (policy_.mode == MemoryMode::None) return {};
    return history_;
}

void PersistentMemory::rebuild_episodes() {
    episodes_.clear();
    if (history_.empty() || policy_.mode != MemoryMode::Episodic) return;
    for (std::size_t i = 0; i < history_.size(); i += policy_.episode_size) {
        const std::size_t end = std::min(history_.size(), i + policy_.episode_size);
        MemoryRecord r;
        r.first_tick = history_[i].tick;
        r.last_tick = history_[end - 1].tick;
        r.experience_count = end - i;
        std::ostringstream s;
        s << "ticks " << r.first_tick << "-" << r.last_tick << ": ";
        for (std::size_t j = i; j < end; ++j) {
            if (j != i) s << " | ";
            s << history_[j].action << " (" << (history_[j].action_valid ? "valid" : "invalid") << ")";
        }
        r.summary = s.str();
        episodes_.push_back(std::move(r));
    }
}

std::string PersistentMemory::context_text() const {
    std::ostringstream out;
    out << "memory_mode=" << memory_mode_name(policy_.mode);
    if (policy_.mode == MemoryMode::None) return out.str();
    if (policy_.mode == MemoryMode::Rolling) {
        out << "\nrecent_experiences=" << history_.size();
        for (const auto& e : history_) {
            out << "\ntick=" << e.tick
                << " obs=" << e.observation_fingerprint
                << " action=" << e.action
                << " valid=" << (e.action_valid ? "true" : "false")
                << " outcome=" << e.outcome;
        }
    } else {
        out << "\nepisodes=" << episodes_.size();
        for (const auto& e : episodes_) out << "\n" << e.summary;
    }
    return out.str();
}

} // namespace coagentics::agents
