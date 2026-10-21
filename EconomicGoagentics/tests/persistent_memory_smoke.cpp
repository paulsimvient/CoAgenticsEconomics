#include "coagentics/agents/PersistentMemory.hpp"
#include <cassert>
#include <iostream>
using namespace coagentics::agents;

int main() {
    {
        PersistentMemory m({MemoryMode::None, 2, 2});
        m.remember({1,"a","BUY",true,"filled"});
        assert(m.visible_history().empty());
    }
    {
        PersistentMemory m({MemoryMode::Rolling, 2, 2});
        m.remember({1,"a","BUY",true,"filled"});
        m.remember({2,"b","SELL",true,"filled"});
        m.remember({3,"c","HOLD",true,"none"});
        assert(m.visible_history().size() == 2);
        assert(m.visible_history().front().tick == 2);
        assert(m.context_text().find("tick=2") != std::string::npos);
    }
    {
        PersistentMemory m({MemoryMode::Episodic, 8, 2});
        for (std::uint64_t t=1;t<=5;++t) m.remember({t,"o","BUY",true,"ok"});
        assert(m.episodes().size() == 3);
        assert(m.episodes()[0].first_tick == 1 && m.episodes()[0].last_tick == 2);
        assert(m.episodes().back().first_tick == 5);
    }
    std::cout << "persistent_memory_smoke PASS\n";
}
