#pragma once
#include "coagentics/experiment/LlmExperiment.hpp"
#include "coagentics/market/Mechanism.hpp"
#include <cstdint>
#include <string>
#include <vector>
namespace coagentics::experiment {

// Configurable market identity for Layer A qualification and Layer B AI runs.
struct MarketSpec {
 market::MechanismKind mechanism{market::MechanismKind::ContinuousDoubleAuction};
 std::string asset{"ASSET"};
 double fundamental{100.0};
 int buyers{4};
 int sellers{4};
 int periods{20};           // CDA rounds or sealed clearing periods
 double value_step{10.0};
 double starting_cash{5000.0};
 double efficiency_gate{90.0}; // DARPA market-fidelity gate (not LLM performance)
};

struct MarketQualificationTrial {
 std::uint64_t seed{};
 market::MechanismKind mechanism{};
 market::Metrics metrics{};
 market::UtilitySummary utility{};
 std::size_t trades{};
};

struct MarketQualificationReport {
 MarketSpec spec_template; // buyers/sellers/periods/gate (mechanism field unused; both run)
 std::vector<MarketQualificationTrial> trials;
 double mean_efficiency_cda{};
 double mean_efficiency_sealed{};
 bool cda_qualified{false};
 bool sealed_qualified{false};
 bool layer_a_pass{false};
 std::string claim_boundary{
  "Layer A qualifies the MARKET TEST ENVIRONMENT with programmed reference agents. "
  "This is NOT an LLM performance claim."};
 analysis::EvidenceStore evidence;
};

// Layer A: programmed/heuristic reference agents on CDA and sealed-bid.
// 100% efficiency = theoretical maximum surplus (existing Market::metrics definition).
MarketQualificationReport run_market_qualification(const MarketSpec& population,
 std::uint64_t first_seed, std::size_t n_trials);

struct PairedLlmDelta {
 double delta_efficiency{};
 double delta_surplus{};
 int delta_trades{};
 double delta_mean_price{};
};

struct PairedLlmExperimentResult {
 std::uint64_t seed{};
 MarketSpec market;
 // Control: programmed ZI/heuristic agents only (no LLM).
 market::Metrics control_metrics{};
 std::size_t control_trades{};
 double control_mean_price{};
 // Treatment: one LLM agent + programmed counterparties (Wave-1 path).
 RunResult treatment;
 PairedLlmDelta deltas{};
 analysis::EvidenceStore evidence;
 std::string claim_boundary{
  "Layer B paired comparison of observable market outcomes under shared seed/config. "
  "Does not claim LLM economic rationality or Phase-II behavioral constructs."};
};

// Shared seed + identical private-value assignment / counterparty limits.
// Control replaces the LLM slot with a programmed buyer; treatment uses transport.
PairedLlmExperimentResult run_paired_programmed_buyer_vs_llm(const ExperimentSpec& economics,
 const RunSpec& treatment_run,
 double control_buyer_limit_price);

std::string market_qualification_json(const MarketQualificationReport&);
std::string paired_llm_json(const PairedLlmExperimentResult&);
}
