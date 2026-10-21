#pragma once
#include "coagentics/experiment/Scientist.hpp"
#include "coagentics/analysis/Influence.hpp"
#include <vector>
#include <functional>
namespace coagentics::experiment {
// Actual 12-trader, sequential-unit continuous-double-auction domain.
// Every paired run reuses activation order and quote random variates.
struct MarketOutcome {double efficiency{}, surplus{}, mean_target_quote{}, mean_transaction_price{}; int trades{},target_quotes{};};
struct MarketPair {MarketOutcome control,treatment; std::uint64_t seed{};};
// Black-box-observable decision trace. `desired_price` is the agent's emitted
// action before the exchange applies the budget/standing-quote constraints.
// No access to model internals is required to populate these fields.
enum class QuoteDisposition { exhausted, no_legal_quote, rejected_standing, accepted, executed };
struct DecisionTrace {
 int event{}, actor{}, marginal_unit{};
 double private_limit{}, standing_bid{}, standing_ask{}, draw{}, desired_price{}, submitted_price{};
 double news_visible{}, peer_visible{}, transaction_price{};
 int counterparty{-1};
 bool buyer{}, legal{}, attempted{};
 QuoteDisposition disposition{QuoteDisposition::exhausted};
};
struct TracedMarketPair {
 MarketPair outcomes;
 std::vector<DecisionTrace> control,treatment;
};
class CdaMarketDomain final:public ExperimentalDomain {
public:
 explicit CdaMarketDomain(std::string target_policy="signal-responsive",int quote_events=1500):policy_(std::move(target_policy)),events_(quote_events){}
 PairedObservation run(const Probe&,std::uint64_t seed) const override;
 MarketPair run_market(const Probe&,std::uint64_t seed) const;
 TracedMarketPair run_traced(const Probe&,std::uint64_t seed) const;
 std::string name()const override{return "DV026 12-trader CDA (full market)";}
private:std::string policy_;int events_;
};
// A controlled, externally imposed network-exposure experiment. The network
// propagates an exogenous message, not endogenous trading decisions.
struct NetworkMarketResult {
 TracedMarketPair market;
 analysis::PropagationResult exposure;
 std::vector<double> attempted_quote_shift;
 std::vector<unsigned> matched_attempts;
};
NetworkMarketResult run_network_market(const std::vector<analysis::NetworkEdge>& edges,
 unsigned origin, double impulse, unsigned propagation_steps, std::uint64_t seed,
 int quote_events=1500);
// The callback receives each decision as the C++ market executes. It may
// update the six buyer exposures for subsequent activations.
TracedMarketPair run_live_network_market(std::uint64_t seed, int quote_events,
    std::vector<double>& exposures,
    const std::function<void(const DecisionTrace&,std::vector<double>&)>& on_decision);
struct MarketScientistResult {ScientistReport scientist;std::vector<MarketPair> selected_probe_market_pairs;};
MarketScientistResult run_market_scientist(const CdaMarketDomain&,const std::vector<Candidate>&,const std::vector<Probe>&,std::uint64_t seed,const ScientistConfig&);
std::string market_scientist_markdown(const MarketScientistResult&);
}
