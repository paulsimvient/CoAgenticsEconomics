#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
namespace coagentics::analysis {
struct TraceFeatures {
 std::array<double,5> response{};
 std::array<std::size_t,5> matched{}, control_legal{}, treatment_legal{};
 std::array<std::size_t,5> control_attempts{},treatment_attempts{};
};
struct TraceConfig {
 std::size_t training_trials{12},heldout_trials{30};
 std::uint64_t training_seed{419000},heldout_seed{919000};
 int quote_events{1500};double abstain_margin{.015};
 std::size_t min_matched{1};
};
struct TraceRow {std::string mechanism;std::size_t correct{},total{},abstained{},false_positive{};double recovery{};};
struct TraceReport {
 std::vector<TraceRow> rows;
 double recovery{},null_false_discovery{},abstention{},mean_matched_coverage{};
 bool recovery_gate{},null_gate{};
 std::string protocol;
};
TraceFeatures decision_trace_trajectory(const std::string&,std::uint64_t seed,int quote_events=1500);
TraceReport validate_decision_traces(const TraceConfig& cfg={});
std::string trace_validation_markdown(const TraceReport&);
std::string trace_validation_json(const TraceReport&);
}
