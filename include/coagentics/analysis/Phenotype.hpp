#pragma once
#include "coagentics/agents/Agents.hpp"
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
 double recovery{};                  // fraction of perturbation removed after signal withdrawal
 int abstentions{};
 int malformed_actions{};
 int provider_failures{};
};
struct ExplanationScore { std::string hypothesis; double support{}; std::string discriminator; };
struct PhenotypeResult { BehavioralFingerprint fingerprint; std::vector<ExplanationScore> explanations; std::string next_experiment; };
struct PhenotypeReport { std::string version{"v14"}; std::uint64_t seed{}; std::vector<PhenotypeResult> agents; bool live_llms_used{false}; std::string interpretation; };
PhenotypeResult phenotype_known_mechanism(const std::string& mechanism,std::uint64_t seed=26026);
PhenotypeReport run_behavioral_phenotyping(std::uint64_t seed=26026);
std::string phenotype_markdown(const PhenotypeReport&);
std::string phenotype_json(const PhenotypeReport&);
}
