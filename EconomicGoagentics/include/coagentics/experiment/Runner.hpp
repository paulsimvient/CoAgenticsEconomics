#pragma once
#include "coagentics/agents/Agents.hpp"
#include "coagentics/experiment/Experiment.hpp"
#include "coagentics/experiment/Intervention.hpp"
#include "coagentics/market/Market.hpp"
#include <cstdint>
#include <map>
#include <memory>
#include <vector>
namespace coagentics::experiment {
struct AuctionOutcome { std::uint64_t seed{}; coagentics::market::Metrics metrics; std::vector<coagentics::market::Bid> bids; std::vector<coagentics::market::Trade> trades; };
struct ReferenceAuctionConfig { int buyers{6}; int sellers{6}; int rounds{30}; double fundamental{100}; double value_step{5}; double starting_cash{1000}; };
AuctionOutcome run_reference_auction(std::uint64_t seed,const ReferenceAuctionConfig&,const InformationTimeline& timeline={},bool heuristic=false);
std::vector<TrialResult> run_paired_news_experiment(std::uint64_t first_seed,std::size_t n,const ReferenceAuctionConfig&,double signal,double reliability=1.0);
}
