#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace coagentics::experiment {
struct TrialResult { std::uint64_t seed{}; double control_efficiency{}; double treatment_efficiency{}; double delta{}; };
struct Summary { std::size_t n{}; double mean_delta{}; double standard_error{}; double ci95_low{}; double ci95_high{}; };
Summary summarize(const std::vector<TrialResult>& trials);
}
