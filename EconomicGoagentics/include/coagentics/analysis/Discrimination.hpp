#pragma once
#include "coagentics/analysis/Classifier.hpp"
#include "coagentics/analysis/Benchmark.hpp"
#include <cstdint>
#include <vector>
namespace coagentics::analysis {
struct DiscriminationCell { bool information{}; bool peer_visibility{}; bool heterogeneous{}; bool incentive_shift{}; BehavioralFeatureVector features; Classification classification; };
std::vector<DiscriminationCell> run_discrimination_matrix(std::uint64_t seed=1);
}
