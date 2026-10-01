#pragma once
#include "coagentics/analysis/Evidence.hpp"
#include "coagentics/experiment/Runner.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
namespace coagentics::experiment {
struct CampaignConfig {
 std::string id{"campaign-1"}; std::uint64_t first_seed{1000}; std::size_t trials{100};
 ReferenceAuctionConfig auction{}; std::uint64_t intervention_time{5}; double signal{-18.0}; double reliability{1.0};
 std::vector<std::string> targeted_agents{"B0"};
};
struct CampaignSummary {
 CampaignConfig config; double control_mean_efficiency{}; double treatment_mean_efficiency{}; double mean_efficiency_delta{};
 double ci95_low{}; double ci95_high{}; double targeted_mean_abs_shift{}; double non_target_mean_abs_shift{}; double mean_propagation_ratio{};
 std::size_t supports{}; std::size_t challenges{}; std::size_t inconclusive{};
 coagentics::analysis::EvidenceStore evidence;
};
CampaignSummary run_behavior_campaign(const CampaignConfig&);
std::string campaign_json(const CampaignSummary&);
std::string campaign_html(const CampaignSummary&);
}
