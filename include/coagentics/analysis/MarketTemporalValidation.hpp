#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
namespace coagentics::analysis {
struct MarketTemporalConfig {std::size_t training_trials{12}, heldout_trials{30};std::uint64_t training_seed{419000},heldout_seed{919000};int quote_events{1500};double abstain_margin{0.015};};
struct MarketTemporalRow {std::string mechanism;std::size_t correct{},total{},abstained{},null_false_positive{};double recovery{};};
struct MarketTemporalReport {std::vector<MarketTemporalRow> rows;double heldout_recovery{},null_false_discovery{},abstention{};bool recovery_gate{},null_gate{};std::string protocol;};
// Five sequential market sessions, each with a paired control market and identical activation tape.
// A persistent target-agent state spans sessions; other traders reset between sessions.
std::array<double,5> market_temporal_trajectory(const std::string& mechanism,std::uint64_t seed,int quote_events=1500);
MarketTemporalReport validate_market_temporal(const MarketTemporalConfig& cfg={});
std::string market_temporal_markdown(const MarketTemporalReport&);
std::string market_temporal_json(const MarketTemporalReport&);
}
