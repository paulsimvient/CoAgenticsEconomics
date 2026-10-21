#include "coagentics/agents/PersistentMemoryAgent.hpp"
#include <cassert>
#include <iostream>

using namespace coagentics::agents;

int main() {
    PersistentMemoryAgent agent({MemoryMode::Rolling, 2, 2}, "LLM-0");
    assert(agent.context_for_next_decision().find("memory_mode=rolling") != std::string::npos);

    agent.record_decision(1, "obs-a", "BUY 80", true, "filled");
    agent.record_decision(2, "obs-b", "BUY 82", true, "filled");
    agent.record_decision(3, "obs-c", "HOLD", true, "none");

    const auto visible = agent.memory().visible_history();
    assert(visible.size() == 2);
    assert(visible.front().tick == 2);
    assert(visible.back().tick == 3);

    PersistentMemoryAgent none({MemoryMode::None, 8, 4}, "LLM-1");
    none.record_decision(1, "obs", "BUY 80", true, "filled");
    assert(none.memory().visible_history().empty());

    PersistentMemoryAgent episodic({MemoryMode::Episodic, 8, 2}, "LLM-2");
    for (std::uint64_t t = 1; t <= 4; ++t)
        episodic.record_decision(t, "obs", t % 2 ? "BUY" : "SELL", true, "none");
    assert(episodic.memory().episodes().size() == 2);
    assert(episodic.memory().episodes()[0].experience_count == 2);
    assert(episodic.memory().episodes()[1].experience_count == 2);

    std::cout << "persistent_memory_agent_smoke PASS\n";
}
