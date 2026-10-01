#pragma once
#include "coagentics/experiment/MarketScientist.hpp"
#include <cstdint>
#include <string>
#include <vector>
namespace coagentics::analysis {
struct CalibrationConfig {
  std::size_t train_trials{8};
  std::size_t test_trials{20};
  std::size_t replicates{8};
  int quote_events{1200};
  double decision_threshold{.80};
  std::uint64_t train_seed{170000};
  std::uint64_t test_seed{270000};
};
struct MechanismValidation {
  std::string mechanism;
  std::string expected_hypothesis;
  std::size_t trials{};
  std::size_t correct{};
  std::size_t separated{};
  double recovery_rate{};
  double mean_true_probability{};
  double brier_score{};
};
struct ScientistCalibrationReport {
  double calibrated_noise_sd{};
  double training_brier{};
  double heldout_brier{};
  double heldout_recovery_rate{};
  double null_false_discovery_rate{};
  std::size_t heldout_trials{};
  std::vector<MechanismValidation> mechanisms;
  bool calibration_pass{};
  bool null_control_pass{};
  bool recovery_pass{};
};
ScientistCalibrationReport validate_market_scientist(const CalibrationConfig& cfg=CalibrationConfig{});
std::string scientist_calibration_markdown(const ScientistCalibrationReport&);
std::string scientist_calibration_json(const ScientistCalibrationReport&);
}
