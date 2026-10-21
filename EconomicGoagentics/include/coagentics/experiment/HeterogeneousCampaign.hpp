#pragma once
#include "coagentics/experiment/Dv026LiveCampaign.hpp"
#include <cstdint>
#include <string>
#include <vector>
namespace coagentics::experiment {

struct BalancedSeatAssignment {
 std::string agent_id;
 agents::ModelIdentity model;
 market::Side side{market::Side::Buy};
 double private_value_or_cost{};
 std::size_t activation_position{}; // preregistered base position before seed-controlled shuffle
};

struct HeterogeneousCampaignSpec {
 std::string campaign_id{"dv026-heterogeneous-10-model-v1"};
 std::uint64_t base_seed{424242};
 std::size_t n_seeds{3};
 int rounds{2};
 std::size_t max_llm_seats{10};
 std::size_t inference_repeats{2};
 std::size_t minimum_distinct_models{10};
 bool require_minimum_distinct_models{true};
 bool smoke{false};
 bool use_live_ollama{true};
 bool run_frozen_snapshot{true};
 bool run_sequential_interaction{true};
 bool run_counterfactual_substitution{true};
 std::string results_dir{"results/dv026_heterogeneous_campaign"};
 std::string protocol_id;
 std::string protocol_sha256;
 std::string scenario_id;
 std::string trial;
 std::string condition;
 std::string required_model;
};

struct HeterogeneousCampaignCell {
 std::uint64_t seed{};
 std::size_t repeat{};
 ActivationDesign activation_design{ActivationDesign::SequentialInteraction};
 std::vector<BalancedSeatAssignment> assignment;
 PopulationRunResult population;
};

struct CounterfactualModelDecision {
 agents::ModelIdentity model;
 std::size_t repeat{};
 AgentTurnRecord turn;
};
struct CounterfactualSubstitutionReport {
 std::uint64_t seed{};
 DecisionContext fixed_context;
 std::vector<CounterfactualModelDecision> decisions;
 std::string claim_boundary{
  "Counterfactual model substitution: identical decision context across model identities; descriptive black-box behavior only."};
};

struct ModelCampaignSummary {
 agents::ModelIdentity model;
 std::size_t assignment_count{};
 std::size_t turn_count{};
 std::size_t buy_count{};
 std::size_t sell_count{};
 std::size_t hold_or_abstain_count{};
 std::size_t accepted_count{};
 std::size_t rejected_count{};
};

struct HeterogeneousCampaignReport {
 HeterogeneousCampaignSpec spec;
 OllamaPreflight preflight;
 std::vector<HeterogeneousCampaignCell> cells;
 std::vector<CounterfactualSubstitutionReport> counterfactuals;
 std::vector<ModelCampaignSummary> model_summaries;
 std::size_t distinct_models_used{};
 bool minimum_model_requirement_met{false};
 bool balanced_role_value_rotation{true};
 bool repeated_inference{true};
 std::string research_expectations_sha256;
 std::string research_expectations_json;
 std::string claim_boundary{
  "Multi-seed heterogeneous shared-market campaign with deterministic role/value rotation, activation-design contrasts, repeated inference, and fixed-context model substitution. Observable behavior only."};
};

std::vector<BalancedSeatAssignment> balanced_assignment_for_seed(
 const std::vector<agents::ModelIdentity>& models, std::uint64_t seed, std::uint64_t base_seed);
HeterogeneousCampaignReport run_heterogeneous_campaign(const HeterogeneousCampaignSpec& spec=HeterogeneousCampaignSpec{});
std::string heterogeneous_campaign_json(const HeterogeneousCampaignReport&);
}
