#pragma once
#include <cstddef>
#include <string>
#include <vector>

namespace coagentics::science {

struct Hypothesis {
    std::string id;
    std::string description;
};

struct CandidateExperiment {
    std::string id;
    std::string description;
    double cost{1.0};
    bool feasible{true};
    // Predicted discrete outcome for each hypothesis, in hypothesis order.
    std::vector<int> predicted_outcomes;
};

struct AdaptiveSelectionConfig {
    double cost_weight{0.0};
    double minimum_score{0.0};
};

struct SelectionRationale {
    std::string selected_experiment_id;
    double discrimination_score{0.0};
    double cost{0.0};
    double utility{0.0};
    std::size_t distinguishable_hypothesis_pairs{0};
    std::string rationale;
};

struct AdaptiveSelectionResult {
    bool selected{false};
    SelectionRationale rationale;
};

AdaptiveSelectionResult select_next_experiment(
    const std::vector<Hypothesis>& hypotheses,
    const std::vector<CandidateExperiment>& candidates,
    const AdaptiveSelectionConfig& config = {});

} // namespace coagentics::science
