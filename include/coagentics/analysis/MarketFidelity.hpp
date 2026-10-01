#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace coagentics::analysis {
struct LiteratureTarget { std::string id,population; std::vector<double> market_mean_efficiency; double across_market_mean{}; std::string source_id,source_note; };
struct ReplicationCell { int market{}; std::string population; double literature_mean_efficiency{},simulated_mean_efficiency{},absolute_error{}; };
struct ProtocolAudit { bool single_unit_quotes{},improvement_rule{},crossing_executes{},earlier_quote_price{},cancel_quotes_after_trade{},sequential_marginal_units{},zi_c_budget_constraint{}; };
struct MarketFidelityReport {
 std::string version; std::uint64_t seed{}; int periods_per_market{}; std::vector<ReplicationCell> cells; double zi_c_mae{};
 bool protocol_structurally_aligned{}, numerical_replication_claimed{}; ProtocolAudit audit; std::string limitation;
};
LiteratureTarget gode_sunder_zi_c_target(); LiteratureTarget gode_sunder_human_target();
MarketFidelityReport run_gode_sunder_fidelity(std::uint64_t seed=26026,int periods_per_market=50);
std::string market_fidelity_markdown(const MarketFidelityReport&); std::string market_fidelity_json(const MarketFidelityReport&);
}
