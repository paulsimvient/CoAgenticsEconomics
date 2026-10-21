#pragma once
#include "coagentics/analysis/LlmBehavioralProfile.hpp"
#include "coagentics/experiment/Dv026LiveCampaign.hpp"
#include "coagentics/experiment/LlmExperiment.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace coagentics::experiment {

enum class BehavioralProbeKind { Information, RecentPrice, Peer };
const char* behavioral_probe_name(BehavioralProbeKind);

struct BehavioralProbeSpec {
    BehavioralProbeKind kind{BehavioralProbeKind::Information};
    std::string probe_id{"news"};
    double expected_delta{0.20};
};

struct LlmBehavioralCampaignSpec {
    std::string campaign_id{"dv026-live-llm-behavior-v1"};
    std::uint64_t base_seed{700000};
    std::size_t paired_trials_per_probe{20};
    std::size_t minimum_distinct_models{10};
    bool require_minimum_models{true};
    bool use_live_ollama{true};
    bool smoke{false};
    double minimum_effect{0.10};
    double alpha{0.05};
    int rounds{1};
    std::string results_dir{"results/dv026_live_llm_behavior"};
    std::vector<BehavioralProbeSpec> probes{
        {BehavioralProbeKind::Information, "news", 0.20},
        {BehavioralProbeKind::RecentPrice, "price", 0.20},
        {BehavioralProbeKind::Peer, "peer", 0.20}
    };
};

struct EvidenceTrace {
    std::string evidence_id;
    std::string probe_id;
    std::uint64_t seed{};
    std::string control_run_id;
    std::string treatment_run_id;
    std::string control_observation_fingerprint;
    std::string treatment_observation_fingerprint;
    std::string control_action_fingerprint;
    std::string treatment_action_fingerprint;
    std::string control_outcome_fingerprint;
    std::string treatment_outcome_fingerprint;
    std::string control_canonical_request;
    std::string treatment_canonical_request;
    std::string control_raw_response;
    std::string treatment_raw_response;
    std::string control_action;
    std::string treatment_action;
    std::string control_validation;
    std::string treatment_validation;
    std::string claim_status{"inconclusive"};
};

struct ProbeRunPair {
    std::uint64_t seed{};
    bool control_response{};
    bool treatment_response{};
    bool control_valid{false};
    bool treatment_valid{false};
    bool paired{false};
    std::string control_status;
    std::string treatment_status;
    EvidenceTrace evidence;
};

struct LlmModelBehaviorResult {
    agents::ModelIdentity model;
    std::vector<analysis::ProbeObservation> probes;
    analysis::LlmBehavioralProfile profile;
    std::size_t valid_pairs{};
    std::size_t invalid_pairs{};
    std::vector<EvidenceTrace> evidence;
};

struct LlmBehavioralCampaignReport {
    LlmBehavioralCampaignSpec spec;
    OllamaPreflight preflight;
    std::vector<LlmModelBehaviorResult> models;
    std::size_t distinct_models_available{};
    bool minimum_model_requirement_met{false};
    bool executed_live{false};
    bool campaign_ready{false};
    std::string claim_boundary{
        "Observable model response profiles from controlled matched probes; no inference of hidden beliefs, intent, psychology, or causal claims beyond the preregistered probes."};
    std::string limitation{
        "A live campaign is only considered executed when the required model catalog is available. Missing/offline models are not replaced by mocks."};
};

LlmBehavioralCampaignReport run_llm_behavioral_campaign(
    const LlmBehavioralCampaignSpec& spec = LlmBehavioralCampaignSpec{});
std::string llm_behavioral_campaign_json(const LlmBehavioralCampaignReport&);
std::string llm_behavioral_campaign_markdown(const LlmBehavioralCampaignReport&);

} // namespace coagentics::experiment
