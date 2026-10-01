#pragma once
#include "coagentics/analysis/HumanReference.hpp"
#include <string>
#include <vector>
namespace coagentics::analysis {
enum class EvidenceOrigin { ScriptedControl, LiveProvider, Unknown };
struct ModelObservation {
 std::string metric, condition, model_id, run_id;
 double value{};
 EvidenceOrigin origin{EvidenceOrigin::Unknown};
};
struct HumanComparison {
 std::string metric, status, explanation, source_id;
 std::size_t human_units{}, model_runs{};
 double human_mean{}, model_mean{}, difference{}, empirical_percentile{-1};
};
struct HumanComparisonReport {
 std::vector<HumanComparison> comparisons;
 bool live_model_evidence{}, human_behavioral_coverage{}, darpa_claim_ready{};
};
// Compare only matching conditions and units. Scripted controls are NEVER described as LLM observations.
HumanComparisonReport compare_human_behavior(const HumanReferenceSet&,const std::vector<ModelObservation>&);
std::string human_comparison_markdown(const HumanComparisonReport&);
std::string human_comparison_json(const HumanComparisonReport&);
}
