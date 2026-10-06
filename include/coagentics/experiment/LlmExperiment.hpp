#pragma once
#include "coagentics/agents/LlmHarness.hpp"
#include "coagentics/analysis/Evidence.hpp"
#include "coagentics/market/Market.hpp"
#include "coagentics/market/Mechanism.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>
namespace coagentics::experiment {

enum class ActionType { Buy, Sell, Hold };
enum class AgentRole { Buyer, Seller };

// Canonical structured action for the LLM market slice.
struct MarketAction {
 ActionType action{ActionType::Hold};
 std::string asset;
 int quantity{0};
 std::optional<double> price;
 std::uint64_t time{0};
};

struct FillSummary {
 int filled_quantity{0};
 std::optional<double> average_fill_price;
 std::vector<market::Trade> trades;
};

// Public book / signal state only. Never includes private agent economics.
struct MarketState {
 std::uint64_t time{};
 std::string asset;
 double fundamental{};
 double public_signal{};
 std::optional<double> best_bid;
 std::optional<double> best_ask;
 std::optional<double> last_trade;
};

// Private economics for ONE agent. Must never include another agent's private fields.
struct AgentState {
 std::string agent_id;
 AgentRole role{AgentRole::Buyer};
 double cash{};
 int inventory{};
 double private_value{};
 int remaining_demand{};
 std::optional<MarketAction> previous_action;
 std::optional<FillSummary> previous_fill;
 double realized_payoff{};
};

// Controlled information visible to the deciding agent. Never other agents' private values/costs.
struct NewsState {
 bool present{false};
 std::string headline;
 double signal{0};
 double reliability{0};
};

struct MarketHistoryEntry {
 std::uint64_t time{};
 std::optional<double> last_trade;
 std::optional<double> best_bid;
 std::optional<double> best_ask;
};

struct PeerObservation {
 bool peer_visibility{false};
 std::vector<std::string> visible_agent_ids; // public identities only
 std::optional<double> peer_mid_quote;       // public aggregate quote, not private values
};

struct ActionConstraints {
 bool allow_buy{true};
 bool allow_sell{true};
 bool allow_hold{true};
 static ActionConstraints for_role(AgentRole role){
  ActionConstraints c;
  if(role==AgentRole::Buyer){ c.allow_buy=true; c.allow_sell=false; c.allow_hold=true; }
  else { c.allow_buy=false; c.allow_sell=true; c.allow_hold=true; }
  return c;
 }
};

struct InformationContext {
 NewsState news;
 bool history_visible{true}; // explicit experimental treatment gate
 std::vector<MarketHistoryEntry> history;
 PeerObservation peer;
 std::string information_condition{"public_book"};
 ActionConstraints constraints{};
};

struct DecisionContext {
 MarketState market;
 AgentState agent;
 InformationContext information{};
};

struct ParseStageResult {
 bool success{false};
 std::string error;
};

struct ActionValidationResult {
 bool valid{false};
 std::vector<std::string> errors;
};

using SubmissionResult = market::SubmitResult;

struct AgentTurnRecord {
 std::string run_id;
 std::string agent_id;
 std::uint64_t time{};

 MarketState market_state;
 AgentState agent_state_before;
 AgentState agent_state_after;

 agents::ModelIdentity model;
 std::string request_id;
 std::uint64_t seed{};
 std::string adapter_version;
 std::string inference_config;
 std::string canonical_request; // exact agent-visible decision payload offered to the model (no audit metadata)
 std::string raw_provider_response;

 std::optional<MarketAction> parsed_action;
 ParseStageResult parse;
 ActionValidationResult action_validation;
 SubmissionResult submission;

 std::uint64_t latency_ms{};
 std::uint64_t prompt_tokens{};
 std::uint64_t completion_tokens{};
 bool held{false};
};

struct ExperimentSpec {
 std::string experiment_id{"llm-market-slice-v1"};
 std::string asset{"ASSET"};
 double fundamental{100.0};
 double starting_cash{10000.0};
 double llm_private_value{120.0};
 double seller_cost{80.0};
 int rounds{1};
 // Deterministic counterparty for reproducible integration tests.
 bool deterministic_counterparty{true};
 double counterparty_limit_price{90.0}; // seller limit
 int counterparty_quantity{1};
 AgentRole llm_role{AgentRole::Buyer};
 InformationContext information{}; // controlled treatments (news/history/peer/constraints)
 // Mechanism under the same LLM interface (CDA continuous; sealed clears each round).
 market::MechanismKind mechanism{market::MechanismKind::ContinuousDoubleAuction};
};

struct RunSpec {
 std::uint64_t seed{0};
 agents::ModelIdentity model;
 std::shared_ptr<agents::ModelTransport> transport;
 std::string log_path;
 std::string adapter_version{"coagentics-llm-adapter/0.1"};
 std::string llm_agent_id{"LLM-0"};
 std::string seller_agent_id{"S0"};
 std::string run_id;
};

struct RunResult {
 std::string experiment_id;
 std::string run_id;
 std::uint64_t seed{};
 agents::ModelIdentity model;
 std::vector<AgentTurnRecord> turns;
 std::vector<market::Bid> accepted_bids;
 std::vector<market::Trade> trades;
 market::Metrics metrics{};
 analysis::EvidenceStore evidence;

 std::size_t parse_failures{0};
 std::size_t action_validation_failures{0};
 std::size_t market_rejections{0};
 std::size_t holds{0};
 std::size_t provider_failures{0};

 std::size_t submitted_actions{0};
 std::size_t accepted_actions{0};
 std::size_t rejected_actions{0};
 std::size_t units_filled{0};
 double starting_cash{};
 double ending_cash{};
 int starting_inventory{};
 int ending_inventory{};
 double llm_realized_payoff{};
};

MarketState build_market_state(const market::Market& market, std::uint64_t time,
 const std::string& asset, double fundamental, double public_signal=0);

AgentState build_agent_state(const market::Market& market, const std::string& agent_id,
 const std::string& asset, double private_value, int remaining_demand,
 AgentRole role=AgentRole::Buyer,
 const std::optional<MarketAction>& previous_action={},
 const std::optional<FillSummary>& previous_fill={},
 double realized_payoff=0);

// Overload for mechanism-backed population path (accounts only; book may be empty for sealed).
AgentState build_agent_state_from_accounts(const std::map<std::string,market::Account>& accounts,
 const std::string& agent_id, const std::string& asset, double private_value, int remaining_demand,
 AgentRole role,
 const std::optional<MarketAction>& previous_action={},
 const std::optional<FillSummary>& previous_fill={},
 double realized_payoff=0);

// Public projection retained for harness compatibility. Does NOT include AgentState.
agents::Observation to_public_observation(const MarketState& market);

std::string canonical_decision_context_json(const DecisionContext& ctx,
 const agents::ModelIdentity& model, const std::string& request_id, std::uint64_t seed);

std::string format_economic_decision_prompt(const DecisionContext& ctx);

/*
 Canonical MarketAction JSON grammar (no third-party JSON dependency):
   object := '{' ws fields ws '}'
   fields  := "action"  : "BUY"|"SELL"|"HOLD"
            , "asset"   : string  (exact market asset; validated later)
            , "time"    : INTEGER (no fractional part; exact market time later)
            , "quantity": INTEGER
            , "price"   : number | null
   HOLD requires quantity==0 (or omitted→0) and price==null (or omitted).
   BUY/SELL require quantity>0 and finite non-null price.
   Malformed values are rejected; no silent coercion of fractions to integers.
*/
struct MarketActionParseResult {
 bool ok{false};
 std::optional<MarketAction> action;
 std::string error;
 std::string raw_retained;
};
MarketActionParseResult parse_canonical_market_action(const std::string& raw_output);

ActionValidationResult validate_market_action(const MarketAction& action, const DecisionContext& ctx);

agents::LlmAction to_llm_action(const MarketAction& a);
std::optional<market::Bid> to_bid(const MarketAction& a, const std::string& agent_id);

class LlmAgentAdapter {
public:
 LlmAgentAdapter(agents::ModelIdentity identity,
  std::shared_ptr<agents::ModelTransport> transport,
  std::uint64_t seed=0,
  std::string adapter_version="coagentics-llm-adapter/0.1",
  std::string log_path={},
  std::string run_id={});
 // Invokes provider with DecisionContext (market + own agent state only).
 AgentTurnRecord decide(const DecisionContext& ctx);
 const std::vector<AgentTurnRecord>& turns() const { return turns_; }
 std::size_t parse_failures() const { return parse_failures_; }
 std::size_t action_validation_failures() const { return action_validation_failures_; }
 std::size_t provider_failures() const { return provider_failures_; }
 std::size_t holds() const { return holds_; }
 // Persist a fully completed turn (after submission + AgentStateAfter). Prefer this over
 // the pre-submit snapshot decide() may have deferred.
 void persist_turn(const AgentTurnRecord& turn) const { append_log(turn); }
private:
 agents::ModelIdentity identity_;
 std::shared_ptr<agents::ModelTransport> transport_;
 std::uint64_t seed_{};
 std::uint64_t seq_{0};
 std::string adapter_version_;
 std::string log_path_;
 std::string run_id_;
 std::vector<AgentTurnRecord> turns_;
 std::size_t parse_failures_{0}, action_validation_failures_{0}, provider_failures_{0}, holds_{0};
 void append_log(const AgentTurnRecord&) const;
};

class RawJsonTransport final : public agents::ModelTransport {
public:
 explicit RawJsonTransport(std::vector<std::string> payloads):payloads_(std::move(payloads)){}
 agents::ModelResponse invoke(const agents::ModelRequest&) override;
private:
 std::vector<std::string> payloads_;
 std::size_t next_{0};
};

bool live_llm_configured();
bool live_uses_ollama();
std::shared_ptr<agents::ModelTransport> make_live_openai_compatible_transport(const agents::ModelIdentity& identity);

RunResult run_llm_market_experiment(const ExperimentSpec& experiment, const RunSpec& run);
}
