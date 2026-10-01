#pragma once
#include "coagentics/analysis/Classifier.hpp"
#include "coagentics/analysis/HumanComparison.hpp"
#include "coagentics/analysis/HumanReference.hpp"
#include "coagentics/experiment/Dv026MarketLayers.hpp"
#include "coagentics/experiment/LlmExperiment.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
namespace coagentics::experiment {

enum class AgentKind { ProgrammedHeuristic, ProgrammedZI, Llm };

// One seat in a shared market. LLM slots carry model identity + transport.
struct AgentSlot {
 std::string agent_id;
 AgentKind kind{AgentKind::ProgrammedHeuristic};
 market::Side side{market::Side::Buy};
 double private_value_or_cost{100.0};
 double cash{5000.0};
 int inventory{0}; // sellers typically start with units
 agents::ModelIdentity model{}; // meaningful for Llm slots
 std::shared_ptr<agents::ModelTransport> transport; // required for Llm
 std::string information_condition{"public_book"}; // observable exposure label only
 InformationContext information{}; // per-seat controlled info (news/peer/history/constraints)
};

struct PopulationSpec {
 MarketSpec market{};
 std::vector<AgentSlot> agents;
 std::uint64_t seed{0};
 int rounds{1};
};

struct PopulationAgentOutcome {
 std::string agent_id;
 AgentKind kind{};
 agents::ModelIdentity model{};
 std::size_t turns{0};
 std::size_t fills{0};
 std::size_t parse_failures{0};
 std::size_t action_validation_failures{0};
 std::size_t market_rejections{0};
 std::vector<AgentTurnRecord> llm_turns; // empty for programmed agents
};

struct PopulationRunResult {
 PopulationSpec spec;
 market::Metrics metrics{};
 std::vector<market::Trade> trades;
 std::vector<market::Bid> bids;
 std::vector<PopulationAgentOutcome> agents;
 analysis::BehavioralFeatureVector features{};
 analysis::Classification classification{};
 std::string classifier_mode{"synthetic_wiring"}; // or "control_treatment"
 bool classifier_synthetic{true};
 std::vector<analysis::ReferenceComparison> human_reference;
 analysis::HumanComparisonReport human_comparison{};
 std::string claim_boundary{
  "Wave 3 population run: observable market outcomes + operational classifier label. "
  "No Phase I classifier performance metrics; no Phase II bias/deception claims."};
 analysis::EvidenceStore evidence;
};

// Multi-agent run via MarketMechanism (CDA or sealed). LLM agents use DecisionContext boundary.
PopulationRunResult run_population_market(const PopulationSpec& population);

// Operational classifier on observable features from a paired control/treatment bid set.
struct OperationalClassifierReport {
 analysis::BehavioralFeatureVector features;
 analysis::Classification classification;
 std::string claim_boundary{
  "Operational classifier demonstrated on observable features. "
  "No Phase I classifier accuracy/performance metrics (FAQ 31)."};
};

OperationalClassifierReport run_operational_classifier(
 const std::vector<market::Bid>& control_bids,
 const std::vector<market::Bid>& treatment_bids,
 std::uint64_t intervention_time,
 const std::vector<std::string>& targeted_agents);

// Control vs treatment population contrast for real behavioral measurement (not self-paired wiring).
struct PopulationBehavioralContrastReport {
 PopulationRunResult control;
 PopulationRunResult treatment;
 OperationalClassifierReport classifier;
 std::string claim_boundary{
  "Paired control/treatment population contrast on observable bids. "
  "Operational features only; no Phase I classifier accuracy claim (FAQ 31)."};
};

PopulationBehavioralContrastReport run_population_behavioral_contrast(
 const PopulationSpec& control,
 const PopulationSpec& treatment,
 std::uint64_t intervention_time=0,
 const std::vector<std::string>& targeted_agents={});

// Preregistered information contrast: identical economics; treatment adds news + peer visibility.
struct InformationContrastReport {
 PopulationBehavioralContrastReport contrast;
 std::string control_condition{"public_book"};
 std::string treatment_condition{"public_book+news+peer"};
 std::string claim_boundary{
  "Preregistered information contrast on observable bids under controlled news/peer visibility. "
  "No Phase I classifier accuracy; does not claim human-equivalent information response."};
};
InformationContrastReport run_information_contrast_experiment(std::uint64_t seed=424242);

// Ten distinct LLMs by provider/family/version (catalog identity). Config-only variants do not count.
std::vector<agents::ModelIdentity> dv026_distinct_llm_catalog();

struct TenLlmCell {
 agents::ModelIdentity model;
 bool parse_ok{false};
 bool action_valid{false};
 bool provenance_ok{false};
 std::string request_id;
 std::string error;
};

struct TenLlmQualificationReport {
 std::vector<TenLlmCell> cells;
 std::size_t distinct_models{0};
 bool qualifies{false}; // >=10 distinct identities with successful interface provenance
 std::string claim_boundary{
  "Interface/provenance qualification across distinct model identities. "
  "Does not claim economic performance of any model."};
 analysis::EvidenceStore evidence;
};

// CI uses ScriptedTransport per model (no network). Live transports plug in separately.
TenLlmQualificationReport run_ten_llm_interface_qualification(
 const std::vector<agents::ModelIdentity>& catalog,
 std::uint64_t seed=424242);

std::string ten_llm_json(const TenLlmQualificationReport&);
std::string population_run_json(const PopulationRunResult&);
}
