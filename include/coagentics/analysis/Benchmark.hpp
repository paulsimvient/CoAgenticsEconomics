#pragma once
#include "coagentics/agents/Agents.hpp"
#include <cstdint>
#include <string>
#include <vector>
namespace coagentics::analysis {
enum class Mechanism { BlindZI, SignalResponsive, RiskSensitive, PeerResponsive, Adaptive };
struct MechanismResult { Mechanism mechanism; double information_sensitivity{}; double peer_sensitivity{}; bool recovered{}; };
std::string name(Mechanism);
MechanismResult probe_mechanism(Mechanism,std::uint64_t seed=1);
std::vector<MechanismResult> run_benchmark_battery(std::uint64_t seed=1);
}
