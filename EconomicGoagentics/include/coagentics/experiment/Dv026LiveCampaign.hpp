#pragma once
#include "coagentics/analysis/Evidence.hpp"
#include "coagentics/analysis/HumanComparison.hpp"
#include "coagentics/analysis/HumanReference.hpp"
#include "coagentics/experiment/Dv026MarketLayers.hpp"
#include "coagentics/experiment/Dv026Wave3.hpp"
#include "coagentics/experiment/Dv026Wave4.hpp"
#include "coagentics/experiment/LlmExperiment.hpp"
#include <cstdint>
#include <map>
#include <string>
#include <vector>
namespace coagentics::experiment {

std::vector<agents::ModelIdentity> dv026_ollama_live_catalog();
std::vector<std::string> ollama_installed_model_tags(const std::string& base_url="http://127.0.0.1:11434");

struct OllamaPreflight {
 std::vector<agents::ModelIdentity> catalog;
 std::vector<std::string> installed;
 std::vector<agents::ModelIdentity> available;
 std::vector<agents::ModelIdentity> missing;
 bool ollama_reachable{false};
};

OllamaPreflight preflight_ollama_live_catalog(const std::string& base_url="http://127.0.0.1:11434");

enum class RepeatabilityStatus { NotMeasured, Exact, Variable, Failed };

struct ReproducibilityGrade {
 bool simulator_seed_controlled{true};
 bool inference_seed_requested{false};
 std::string inference_seed_support{"unknown"}; // supported | unsupported | unknown
 std::string provider;
 std::string model;
 std::string version;
 std::string inference_config;
 std::size_t repeat_trials{0};
 std::size_t exact_repeats{0};
 RepeatabilityStatus repeatability{RepeatabilityStatus::NotMeasured};
 std::string grade{"B"}; // A=empirically exact, B=controlled/requested but unmeasured, C=seed unsupported/unknown, F=failed
 std::string interpretation;
};

ReproducibilityGrade grade_reproducibility(const agents::ModelIdentity& model,
 const std::string& inference_config, bool inference_seed_requested,
 const std::string& seed_support, const std::vector<std::string>& repeated_outputs={});
std::string reproducibility_grade_json(const ReproducibilityGrade&);

struct LiveCampaignCell {
 agents::ModelIdentity model;
 std::uint64_t seed{};
 bool skipped{false};
 std::string skip_reason;
 // Separated readiness dimensions (HOLD-only must not count as economic success).
 bool interface_ok{false};       // parse + action_valid
 bool market_action_ok{false};   // non-HOLD structured action
 bool market_accepted{false};    // submit accepted by market
 bool execution_observed{false}; // filled_quantity > 0
 bool replay_ok{false};          // captured-provider transcript cross-event valid
 bool parse_ok{false};
 bool action_valid{false};
 bool cross_event_valid{false};
 // Economic cell success: interface + market action + accepted + replay (fill preferred but accepted rest ok for participation)
 bool cell_ok{false};
 int units_filled{0};
 double delta_efficiency{};
 double delta_surplus{};
 int delta_trades{};
 double delta_mean_price{};
 double treatment_efficiency{};
 std::uint64_t latency_ms{};
 std::string error;
 ReproducibilityGrade reproducibility{};
};

struct ReadinessCheck {
 std::string id;
 bool pass{false};
 std::string detail;
};

struct DarpaPhaseIReadiness {
 bool darpa_claim_ready{false};
 std::string scope;
 std::vector<ReadinessCheck> checks;
 std::string claim_boundary{
  "Even when darpa_claim_ready=true under scope=local_ollama_poc: no Phase II construct validation, "
  "no classifier accuracy claim (FAQ 31), no commercial multi-provider matrix claim."};
};

struct LiveCampaignSpec {
 MarketSpec market{};
 std::uint64_t base_seed{424242};
 std::size_t n_seeds{20};
 std::size_t min_models{10};
 std::size_t layer_a_trials{5}; // DARPA FAQ: >90% across multiple trials
 std::string implementation_revision{"2026-10-04-pass3-step22h-i-matched-provenance"};
 double min_provenance_rate{0.95};
 double min_interface_rate{0.90};
 double min_market_action_rate{0.80}; // fraction of attempted cells with non-HOLD action
 double min_market_acceptance_rate{0.80}; // fraction of non-HOLD actions accepted by the market
 std::string results_dir{"results/dv026_ollama_campaign"};
 bool smoke{false};
};

struct LiveCampaignReport {
 LiveCampaignSpec spec;
 OllamaPreflight preflight;
 bool layer_a_pass{false};
 std::size_t layer_a_trials{0};
 double mean_efficiency_cda{};
 double mean_efficiency_sealed{};
 std::vector<LiveCampaignCell> cells;
 std::size_t cells_attempted{0};
 std::size_t cells_ok{0};
 std::size_t distinct_live_models_ok{0};
 bool human_ref_gate{false};
 analysis::HumanComparisonReport human_comparison{};
 DarpaPhaseIReadiness readiness;
 bool live_llm{true};
 std::string manifest_sha256;
 std::string manifest_path;
 analysis::EvidenceStore evidence;
};

LiveCampaignReport run_ollama_layer_b_campaign(const LiveCampaignSpec& spec=LiveCampaignSpec{});
DarpaPhaseIReadiness evaluate_darpa_phase_i_readiness(const LiveCampaignReport& campaign);

// Heterogeneous multi-LLM shared market: N live identities co-present (not sequential cells).
struct HeterogeneousPopulationSpec {
 std::uint64_t seed{424242};
 int rounds{2};
 std::size_t max_llm_seats{4}; // co-present LLM buyers + sellers; smoke uses 2
 bool smoke{false};
 bool use_live_ollama{true}; // false → RawJsonTransport for CI
 ActivationDesign activation_design{ActivationDesign::SequentialInteraction};
 std::string results_dir{"results/dv026_hetero_population"};
};

struct HeterogeneousPopulationReport {
 HeterogeneousPopulationSpec spec;
 OllamaPreflight preflight;
 PopulationRunResult population;
 std::size_t llm_seats{0};
 std::size_t llm_buyers{0};
 std::size_t llm_sellers{0};
 bool shared_market{true};
 std::string activation_design{"sequential_interaction"};
 std::string claim_boundary{
  "Heterogeneous multi-LLM shared-market population: co-present seats under one MarketMechanism. "
  "Observable outcomes only; not a commercial multi-provider claim."};
};

HeterogeneousPopulationReport run_heterogeneous_ollama_population(
 const HeterogeneousPopulationSpec& spec=HeterogeneousPopulationSpec{});

std::string live_campaign_json(const LiveCampaignReport&);
std::string live_campaign_markdown(const LiveCampaignReport&);
std::string heterogeneous_population_json(const HeterogeneousPopulationReport&);
}
