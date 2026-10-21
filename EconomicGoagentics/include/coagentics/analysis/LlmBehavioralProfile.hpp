#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace coagentics::analysis {

enum class ClaimStatus { Supports, Challenges, Inconclusive };

// One matched treatment/control probe. treatment_only and control_only are the
// discordant matched pairs: control=0,treatment=1 and control=1,treatment=0.
// Together with matched_n they are sufficient for a paired binary-response estimator.
struct ProbeObservation {
    std::string probe_id;
    double treatment_rate{};
    double control_rate{};
    std::size_t treatment_n{};
    std::size_t control_n{};
    double expected_delta{};
    std::size_t treatment_only{};
    std::size_t control_only{};
    bool paired{false};
};

struct BehavioralDimension {
    std::string name;
    std::string probe_id;
    double effect{};          // mean paired difference: treatment - control
    double standard_error{};  // paired binary-response standard error
    double lower_95{};
    double upper_95{};
    bool positive_signal{};
    bool supported{};
    ClaimStatus status{ClaimStatus::Inconclusive};
    double adjusted_alpha{};
};

struct LlmBehavioralProfile {
    std::string provider;
    std::string model;
    std::string version;
    std::vector<BehavioralDimension> dimensions;
    std::size_t probes_used{};
    std::size_t total_observations{};
    bool sufficient_evidence{false};
    bool paired_analysis{false};
    std::string claim_boundary{
        "Observable response profile from controlled probes; does not infer hidden beliefs, intent, or psychological traits."};
};

// Builds a profile from matched treatment/control binary responses. Claims require
// paired observations. The family-wise confidence level is controlled across probes
// using a Bonferroni-adjusted alpha.
LlmBehavioralProfile build_llm_behavioral_profile(
    std::string provider,
    std::string model,
    std::string version,
    const std::vector<ProbeObservation>& probes,
    double minimum_effect = 0.10,
    double alpha = 0.05);

std::string claim_status_string(ClaimStatus status);
std::string llm_behavioral_profile_markdown(const LlmBehavioralProfile& profile);

} // namespace coagentics::analysis
