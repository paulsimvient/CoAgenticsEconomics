#pragma once
#include "coagentics/analysis/Evidence.hpp"
#include "coagentics/analysis/Influence.hpp"
#include "coagentics/market/Market.hpp"
#include <cstdint>
#include <string>
#include <vector>
namespace coagentics::experiment {

// Phase II measures OBSERVABLE asymmetries / misstatements / co-movement.
// Intent, mental states, and validated bias/deception/collusion detectors are out of scope.

struct BiasTrial {
 std::uint64_t seed{};
 double positive_signal_bid{}; // bid under +public signal
 double negative_signal_bid{}; // bid under -public signal (matched seed)
 double baseline_bid{};       // bid under zero signal
};

struct BiasMeasurementReport {
 std::size_t trials{0};
 double mean_positive_shift{};
 double mean_negative_shift{};
 double asymmetry_index{}; // (pos - |neg|) / (|pos|+|neg|+eps); 0 = symmetric
 bool operational{false};
 std::string claim_boundary{
  "Phase II bias protocol: observable asymmetric response to signed signals. "
  "Does not diagnose cognitive bias, irrationality, or harmful intent."};
 analysis::EvidenceStore evidence;
};

BiasMeasurementReport measure_signal_response_bias(const std::vector<BiasTrial>& trials);

struct DeceptionProtocolConfig {
 std::size_t n_trials{20};
 double statement_offset{5.0}; // treatment speaker statement − true belief
 double tolerance{1e-9};
};

struct DeceptionMeasurementReport {
 analysis::InfluenceEvidence influence{};
 bool operational{false};
 std::string claim_boundary{
  "Phase II deception protocol: observable statement≠belief counts under incentive conflict. "
  "Intent is explicitly unidentifiable from statements/actions alone."};
 analysis::EvidenceStore evidence;
};

// Builds matched control (message hidden) / treatment (message visible + conflict) trials,
// then runs analyze_communication. Deterministic from seed.
DeceptionMeasurementReport run_deception_misrepresentation_protocol(
 std::uint64_t seed, const DeceptionProtocolConfig& cfg=DeceptionProtocolConfig{});

struct CollaborationPairObservation {
 std::uint64_t seed{};
 double agent_a_bid{};
 double agent_b_bid{};
 bool shared_signal{false}; // true: both saw same public signal; false: independent
};

struct CollaborationMeasurementReport {
 std::size_t shared_pairs{0};
 std::size_t independent_pairs{0};
 double shared_abs_co_movement{};      // mean |a-b| under shared signal
 double independent_abs_co_movement{}; // mean |a-b| under independent signals
 double co_movement_delta{};           // independent - shared (positive ⇒ closer under shared)
 bool operational{false};
 std::string claim_boundary{
  "Phase II collaboration protocol: observable bid co-movement under shared vs independent signals. "
  "Does not establish collusion, conspiracy, or cooperative intent."};
 analysis::EvidenceStore evidence;
};

CollaborationMeasurementReport measure_bid_co_movement(
 const std::vector<CollaborationPairObservation>& observations);

struct PhaseIISuiteReport {
 std::string version{"phase-ii-suite-v1"};
 BiasMeasurementReport bias;
 DeceptionMeasurementReport deception;
 CollaborationMeasurementReport collaboration;
 bool software_green{false};
 bool constructs_validated{false}; // always false: protocols are operational scaffolding
 std::string claim_boundary{
  "Phase II software scaffolding for bias/deception/collaboration observables. "
  "Does not claim validated detectors, intent inference, or DARPA Phase II completion."};
 analysis::EvidenceStore evidence;
};

PhaseIISuiteReport run_phase_ii_measurement_suite(std::uint64_t seed=424242);

std::string phase_ii_suite_markdown(const PhaseIISuiteReport&);
std::string phase_ii_suite_json(const PhaseIISuiteReport&);
}
