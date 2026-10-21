#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace coagentics::analysis {
enum class TraderPolicy { ZIC, ValueShading, SignalBiased, PeerAnchored };
struct AttributionCell {
 std::string model_id, policy, condition;
 double mean_efficiency{}, institution_baseline{}, agent_delta{}, intervention_delta{};
 int periods{};
};
struct AttributionReport {
 std::string version{"v13"}; std::uint64_t seed{}; int periods{};
 std::vector<AttributionCell> cells;
 double institution_floor{};
 bool live_llms_used{false};
 std::string interpretation;
};
AttributionReport run_institutional_attribution(std::uint64_t seed=26026,int periods=40);
std::string institutional_attribution_markdown(const AttributionReport&);
std::string institutional_attribution_json(const AttributionReport&);
}
