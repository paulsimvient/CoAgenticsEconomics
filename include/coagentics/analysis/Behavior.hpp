#pragma once
#include "coagentics/market/Market.hpp"
#include <map>
#include <string>
#include <vector>
namespace coagentics::analysis {
struct AgentBehavior { int bids{}; int trades{}; double mean_bid_price{}; double mean_trade_price{}; double price_aggressiveness{}; };
struct PairedAgentResponse {
 std::string agent_id; bool targeted{}; int matched_post_bids{}; double mean_bid_shift{}; double mean_abs_bid_shift{};
 int reaction_latency{-1}; int recovery_latency{-1}; double peak_abs_shift{}; double conformity_change{};
};
struct PropagationSummary { std::uint64_t intervention_time{}; std::vector<PairedAgentResponse> agents; double targeted_mean_abs_shift{}; double non_target_mean_abs_shift{}; double propagation_ratio{}; };
PropagationSummary compare_paired_bids(const std::vector<coagentics::market::Bid>& control,const std::vector<coagentics::market::Bid>& treatment,std::uint64_t intervention_time,const std::vector<std::string>& targeted,double reaction_threshold=0.5,double recovery_threshold=0.5);
std::map<std::string,AgentBehavior> characterize(const std::vector<coagentics::market::Bid>& bids,const std::vector<coagentics::market::Trade>& trades,const std::map<std::string,double>& reference_values);
}
