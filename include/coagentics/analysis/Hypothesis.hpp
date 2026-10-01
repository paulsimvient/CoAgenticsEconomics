#pragma once
#include "coagentics/analysis/Behavior.hpp"
#include <string>
#include <vector>
namespace coagentics::analysis {
enum class EvidenceDirection { Supports, Challenges, Inconclusive };
struct Discriminator {
  std::string id;
  std::string description;
  double targeted_min_shift{0.0};
  double propagation_min_ratio{0.0};
  double propagation_max_ratio{1e9};
};
struct BehavioralHypothesis {
  std::string id;
  std::string statement;
  std::vector<Discriminator> discriminators;
};
struct EvidenceRecord {
  std::string hypothesis_id;
  std::string discriminator_id;
  EvidenceDirection direction{EvidenceDirection::Inconclusive};
  double targeted_mean_abs_shift{};
  double non_target_mean_abs_shift{};
  double propagation_ratio{};
  std::string rationale;
};
EvidenceRecord evaluate(const BehavioralHypothesis&, const Discriminator&, const PropagationSummary&);
std::string to_string(EvidenceDirection);
}
