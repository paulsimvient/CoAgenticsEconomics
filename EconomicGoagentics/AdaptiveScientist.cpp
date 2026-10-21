#include "AdaptiveScientist.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace coagentics::science {
namespace {
std::size_t pair_count(const std::vector<int>& x) {
    std::size_t n = 0;
    for (std::size_t i=0;i<x.size();++i)
        for (std::size_t j=i+1;j<x.size();++j)
            if (x[i] != x[j]) ++n;
    return n;
}
}

AdaptiveSelectionResult select_next_experiment(
    const std::vector<Hypothesis>& hypotheses,
    const std::vector<CandidateExperiment>& candidates,
    const AdaptiveSelectionConfig& config) {
    if (hypotheses.empty()) throw std::invalid_argument("adaptive_selection_requires_hypotheses");
    const double total_pairs = static_cast<double>(hypotheses.size() * (hypotheses.size()-1) / 2);
    if (total_pairs == 0.0) throw std::invalid_argument("adaptive_selection_requires_competing_hypotheses");

    AdaptiveSelectionResult best;
    double best_utility = -std::numeric_limits<double>::infinity();
    for (const auto& c : candidates) {
        if (!c.feasible || c.cost < 0.0) continue;
        if (c.predicted_outcomes.size() != hypotheses.size()) continue;
        const std::size_t pairs = pair_count(c.predicted_outcomes);
        const double discrimination = static_cast<double>(pairs) / total_pairs;
        const double utility = discrimination - config.cost_weight * c.cost;
        if (utility > best_utility) {
            best_utility = utility;
            best.selected = utility >= config.minimum_score && pairs > 0;
            best.rationale.selected_experiment_id = c.id;
            best.rationale.discrimination_score = discrimination;
            best.rationale.cost = c.cost;
            best.rationale.utility = utility;
            best.rationale.distinguishable_hypothesis_pairs = pairs;
            std::ostringstream r;
            r << "Selected " << c.id << " because it distinguishes " << pairs
              << " of " << static_cast<std::size_t>(total_pairs)
              << " competing hypothesis pairs; utility=" << utility
              << " (discrimination=" << discrimination
              << ", cost_weight=" << config.cost_weight << ").";
            best.rationale.rationale = r.str();
        }
    }
    if (!best.selected) {
        best.rationale.selected_experiment_id.clear();
        best.rationale.rationale = "No feasible candidate met the discrimination threshold; no experiment selected.";
    }
    return best;
}

} // namespace coagentics::science
