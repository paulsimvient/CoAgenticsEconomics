#pragma once
#include "coagentics/experiment/LlmBehavioralCampaign.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace coagentics::analysis {

struct EvidenceAuditResult {
    bool valid{false};
    std::size_t models_checked{};
    std::size_t evidence_records{};
    std::size_t complete_records{};
    std::vector<std::string> errors;
};

// Audits the campaign's traceability chain without interpreting the scientific claim.
EvidenceAuditResult audit_llm_behavioral_campaign(
    const experiment::LlmBehavioralCampaignReport& report);

// Produces one machine-readable evidence record per matched probe pair.
std::string evidence_jsonl(const experiment::LlmBehavioralCampaignReport& report);

// Produces a human-readable claim -> evidence navigation index.
std::string evidence_index_markdown(const experiment::LlmBehavioralCampaignReport& report);

} // namespace coagentics::analysis
