#pragma once
#include "coagentics/agents/LlmHarness.hpp"
#include "coagentics/analysis/HumanReference.hpp"
#include "coagentics/market/Mechanism.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
namespace coagentics::experiment {
struct ModelCampaignCell {
 coagentics::agents::ModelIdentity model; coagentics::market::MechanismKind mechanism{}; std::uint64_t seed{}; bool intervention{};
 double efficiency{}; double total_utility{}; std::size_t failures{}; std::size_t abstentions{};
};
struct CrossModelSummary {
 std::vector<ModelCampaignCell> cells; std::size_t distinct_models{}; std::size_t seeds{}; std::size_t mechanisms{};
 double mean_efficiency{}; double min_efficiency{}; double max_efficiency{};
 std::vector<coagentics::analysis::ReferenceComparison> human_reference_comparisons;
};
CrossModelSummary run_cross_model_qualification(const std::vector<coagentics::agents::ModelIdentity>& models,std::uint64_t first_seed,std::size_t seed_count,const coagentics::analysis::HumanReferenceSet& refs);
std::string cross_model_json(const CrossModelSummary&);
}
