#pragma once
#include "coagentics/experiment/Scientist.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace coagentics::analysis {
struct IdentificationConfig {
  std::size_t development_replicates{12};
  std::size_t development_budget{4};
  std::size_t heldout_replicates{20};
  double noise_sd{0.2};
  double stop_posterior{0.80};
  double minimum_separation{0.05};
  std::uint64_t development_seed{410000};
  std::uint64_t heldout_seed{910000};
};

struct HeldoutScore {
  std::string hypothesis_id;
  double mse{0.0};
  double mean_abs_error{0.0};
};

struct BehavioralIdentificationReport {
  std::string hidden_agent_label;
  std::string frozen_hypothesis;
  std::string development_status;
  std::vector<experiment::ExperimentStep> development_steps;
  std::vector<HeldoutScore> heldout_scores;
  std::size_t heldout_trials{0};
  bool explanation_frozen{false};
  bool heldout_winner_matches_frozen{false};
  bool heldout_separates{false};
};

BehavioralIdentificationReport identify_and_validate(
    const std::string& hidden_mechanism,
    const std::vector<experiment::Candidate>& candidates,
    const std::vector<experiment::Probe>& development_probes,
    const std::vector<experiment::Probe>& heldout_probes,
    const IdentificationConfig& cfg = IdentificationConfig{});

std::string behavioral_identification_markdown(const BehavioralIdentificationReport&);
}
