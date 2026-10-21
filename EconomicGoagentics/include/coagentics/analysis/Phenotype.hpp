#pragma once
#include "coagentics/agents/Agents.hpp"
#include "coagentics/experiment/LlmExperiment.hpp"
#include <cstdint>
#include <string>
#include <vector>
namespace coagentics::analysis {
struct BehavioralFingerprint {
 std::string agent_id, mechanism;
 double value_shading{};             // signed distance from reservation value, normalized by reservation
 double news_sensitivity{};          // price delta per unit public signal
 int response_latency{-1};           // steps to cross a material response threshold
 double peer_susceptibility{};       // price delta per unit peer-price displacement
 double price_influence{};           // treatment-vs-control market effect proxy
 double utility_capture{};           // normalized surplus-capture proxy in controlled probe
 double adaptation{};                // persistence/change after repeated evidence
 double withholding{};               // fraction of probe opportunities with no executable action
 double recovery{};
 // Observed live/population descriptors. These are measurements, not classifier accuracy metrics.
 double aggressiveness{};             // normalized willingness to quote toward/through reservation value
 double reservation_value_violation_rate{}; // buyer above value or seller below cost
 double information_sensitivity{};    // paired treatment price displacement; 0 unless measured
 double peer_sensitivity{};           // paired peer-treatment displacement; 0 unless measured
 double price_improvement{};          // mean normalized improvement vs visible opposite quote
 double trade_frequency{};            // filled turns / decision turns
 double surplus_capture{};            // realized payoff normalized by private-value/fundamental opportunity
 double action_validity{};             // schema+economic-valid turns / turns
 double response_consistency{};        // 1 - normalized dispersion of valid action prices
 bool observed{false};                  // fraction of perturbation removed after signal withdrawal
 int abstentions{};
 int malformed_actions{};
 int provider_failures{};
};
struct ExplanationScore { std::string hypothesis; double support{}; std::string discriminator; };
struct PhenotypeResult { BehavioralFingerprint fingerprint; std::vector<ExplanationScore> explanations; std::string next_experiment; };
struct PhenotypeReport { std::string version{"v14"}; std::uint64_t seed{}; std::vector<PhenotypeResult> agents; bool live_llms_used{false}; std::string interpretation; };
BehavioralFingerprint fingerprint_observed_llm(
 const std::string& agent_id,
 const std::string& mechanism,
 double private_value_or_cost,
 coagentics::experiment::AgentRole role,
 const std::vector<coagentics::experiment::AgentTurnRecord>& turns);
PhenotypeResult phenotype_known_mechanism(const std::string& mechanism,std::uint64_t seed=26026);
PhenotypeReport run_behavioral_phenotyping(std::uint64_t seed=26026);
std::string phenotype_markdown(const PhenotypeReport&);
std::string phenotype_json(const PhenotypeReport&);
}
