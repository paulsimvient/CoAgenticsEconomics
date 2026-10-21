#pragma once
#include "coagentics/analysis/BlackBox.hpp"
#include <string>
#include <vector>
namespace coagentics::analysis {
struct RobustnessCase {std::string name, expected, observed; bool pass{};};
struct RobustnessReport {std::vector<RobustnessCase> cases; std::size_t passed{}, total{}; bool all_passed{};};
RobustnessReport run_robustness_audit();
std::string robustness_markdown(const RobustnessReport&);
}
