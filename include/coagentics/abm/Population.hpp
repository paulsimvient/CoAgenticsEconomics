#pragma once
#include "coagentics/agents/Agents.hpp"
#include <functional>
#include <vector>
namespace coagentics::abm {
struct InteractionContext { std::uint64_t tick{}; std::vector<std::string> visible_neighbors; };
using NeighborhoodRule = std::function<std::vector<std::string>(const std::string&, std::uint64_t)>;
struct Population {
 std::vector<coagentics::agents::PopulationMember> members;
 NeighborhoodRule neighborhood;
};
// Deliberately small: supports heterogeneous agents/local interaction/emergence studies later,
// but DV026 v0 does not require a large artificial economy.
}
