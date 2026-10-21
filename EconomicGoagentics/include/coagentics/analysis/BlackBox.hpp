#pragma once
#include "coagentics/agents/LlmHarness.hpp"
#include <array>
#include <memory>
#include <string>
#include <vector>
namespace coagentics::analysis {
// The inference API receives only harness records. Ground-truth labels are held by the validation caller.
struct BlackBoxFeatures { double news{}, peer{}, persistence{}, reversal{}, reliability{}; std::size_t valid{}, failures{}, abstentions{}; std::size_t stages_covered{}; std::size_t min_stage_samples{}; };
struct BlackBoxInference { std::string hypothesis; BlackBoxFeatures features; double separation{}; bool abstained{}; };
struct ProbePlan { std::string id; double signal{}, peer{}; };
std::vector<ProbePlan> preregistered_probes();
BlackBoxFeatures extract_black_box_features(const std::vector<coagentics::agents::HarnessRecord>&);
BlackBoxInference infer_black_box(const std::vector<coagentics::agents::HarnessRecord>&, double min_separation=0.15);
struct ValidationRow { std::string truth, predicted; bool abstained{}; };
struct ValidationReport { std::vector<ValidationRow> rows; double accuracy{}, abstention{}, null_false_discovery{}; bool leakage_test_passed{}; };
ValidationReport validate_black_box_controls();
std::string black_box_report(const ValidationReport&);
}
