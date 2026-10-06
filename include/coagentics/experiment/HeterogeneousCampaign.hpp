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
 std::uint64_t base_seed{424242};
 std::size_t n_seeds{3};
 int rounds{2};
 std::size_t max_llm_seats{4};
 std::size_t inference_repeats{2};
 bool smoke{false};
 bool use_live_ollama{true};
 bool run_frozen_snapshot{true};
 bool run_sequential_interaction{true};
 bool run_counterfactual_substitution{true};
 std::string results_dir{"results/dv026_heterogeneous_campaign"};
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

struct HeterogeneousCampaignReport {
 HeterogeneousCampaignSpec spec;
 OllamaPreflight preflight;
 std::vector<HeterogeneousCampaignCell> cells;
 std::vector<CounterfactualSubstitutionReport> counterfactuals;
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
