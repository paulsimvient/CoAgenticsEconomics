#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
namespace coagentics::analysis {
struct TemporalConfig {std::size_t train_trials{12}, heldout_trials{30};std::uint64_t train_seed{318000},heldout_seed{918000};double abstain_margin{0.15};};
struct TemporalMechanism {std::string name;std::size_t correct{},total{},abstained{};double recovery{};};
struct TemporalReport {std::vector<TemporalMechanism> mechanisms;double recovery{},null_false_discovery{},abstention{};bool recovery_gate{},null_gate{};std::string protocol;};
TemporalReport validate_temporal_scientist(const TemporalConfig& cfg={});
std::string temporal_markdown(const TemporalReport&);
std::string temporal_json(const TemporalReport&);
}
