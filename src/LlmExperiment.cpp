#include "coagentics/experiment/LlmExperiment.hpp"
#include "coagentics/market/Mechanism.hpp"
#include "coagentics/util/Json.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>
namespace coagentics::experiment {
namespace {
std::string esc(const std::string& s){
 // Must escape control chars — raw LLM replies often contain newlines; without this,
 // cell_*.jsonl splits mid-record and Live seats show empty turns for those models.
 std::string o; o.reserve(s.size()+8);
 for(unsigned char c:s){
  switch(c){
   case '"': o+="\\\""; break;
   case '\\': o+="\\\\"; break;
   case '\n': o+="\\n"; break;
   case '\r': o+="\\r"; break;
   case '\t': o+="\\t"; break;
   default:
    if(c<0x20){ char buf[8]; std::snprintf(buf,sizeof(buf),"\\u%04x", (unsigned)c); o+=buf; }
    else o.push_back((char)c);
  }
 }
 return o;
}
std::string getenv_str(const char* k){ if(const char* v=std::getenv(k)) return v; return {}; }

const char* role_name(AgentRole r){ return r==AgentRole::Seller?"SELLER":"BUYER"; }
std::string opt_num(const std::optional<double>& v){
 if(!v) return "null";
 std::ostringstream o; o<<std::setprecision(17)<<*v; return o.str();
}
}

MarketState build_market_state(const market::Market& market, std::uint64_t time,
 const std::string& asset, double fundamental, double public_signal){
 MarketState s; s.time=time; s.asset=asset; s.fundamental=fundamental; s.public_signal=public_signal;
 if(market.has_bid(asset)) s.best_bid=market.best_bid(asset);
 if(market.has_ask(asset)) s.best_ask=market.best_ask(asset);
 if(!market.trades().empty()){
  for(auto it=market.trades().rbegin(); it!=market.trades().rend(); ++it){
   if(it->asset==asset){ s.last_trade=it->price; break; }
  }
 }
 return s;
}

AgentState build_agent_state_from_accounts(const std::map<std::string,market::Account>& accounts,
 const std::string& agent_id, const std::string& asset, double private_value, int remaining_demand,
 AgentRole role,
 const std::optional<MarketAction>& previous_action,
 const std::optional<FillSummary>& previous_fill,
 double realized_payoff){
 AgentState a; a.agent_id=agent_id; a.role=role; a.private_value=private_value;
 a.remaining_demand=remaining_demand; a.previous_action=previous_action;
 a.previous_fill=previous_fill; a.realized_payoff=realized_payoff;
 auto it=accounts.find(agent_id);
 if(it!=accounts.end()){
  a.cash=it->second.cash;
  auto inv=it->second.inventory.find(asset);
  a.inventory=inv==it->second.inventory.end()?0:inv->second;
 }
 return a;
}

AgentState build_agent_state(const market::Market& market, const std::string& agent_id,
 const std::string& asset, double private_value, int remaining_demand,
 AgentRole role,
 const std::optional<MarketAction>& previous_action,
 const std::optional<FillSummary>& previous_fill,
 double realized_payoff){
 return build_agent_state_from_accounts(market.accounts(), agent_id, asset, private_value,
  remaining_demand, role, previous_action, previous_fill, realized_payoff);
}

agents::Observation to_public_observation(const MarketState& m){
 return agents::Observation{m.time,m.asset,m.fundamental,m.public_signal,
  m.best_bid.value_or(0), m.best_ask.value_or(0), m.last_trade.value_or(0)};
}

std::string format_economic_decision_prompt(const DecisionContext& ctx){
 auto c=ctx.information.constraints;
 if(c.allow_buy && c.allow_sell && c.allow_hold) c=ActionConstraints::for_role(ctx.agent.role);
 std::ostringstream o;
 o<<"MARKET STATE\n"
  <<"Time: "<<ctx.market.time<<"\n"
  <<"Asset: "<<ctx.market.asset<<"\n"
  <<"Fundamental: "<<ctx.market.fundamental<<"\n"
  <<"Public signal: "<<ctx.market.public_signal<<"\n"
  <<"Best bid: "<<(ctx.market.best_bid?std::to_string(*ctx.market.best_bid):std::string("none"))<<"\n"
  <<"Best ask: "<<(ctx.market.best_ask?std::to_string(*ctx.market.best_ask):std::string("none"))<<"\n"
  <<"Last trade: "<<(ctx.market.last_trade?std::to_string(*ctx.market.last_trade):std::string("none"))<<"\n\n"
  <<"YOUR PRIVATE STATE\n"
  <<"Agent id: "<<ctx.agent.agent_id<<"\n"
  <<"Role: "<<role_name(ctx.agent.role)<<"\n"
  <<"Private value/cost: "<<ctx.agent.private_value<<"\n"
  <<"Cash: "<<ctx.agent.cash<<"\n"
  <<"Inventory: "<<ctx.agent.inventory<<"\n"
  <<"Remaining demand: "<<ctx.agent.remaining_demand<<"\n"
  <<"Realized payoff so far: "<<ctx.agent.realized_payoff<<"\n\n";
 if(ctx.information.news.present){
  o<<"AUTHORIZED NEWS\n"
   <<"Headline: "<<ctx.information.news.headline<<"\n"
   <<"Signal: "<<ctx.information.news.signal<<"\n"
   <<"Reliability: "<<ctx.information.news.reliability<<"\n\n";
 }
 if(ctx.information.peer.peer_visibility){
  o<<"AUTHORIZED PEER OBSERVATION\n"
   <<"Visibility: on\n";
  if(ctx.information.peer.peer_mid_quote) o<<"Public peer mid quote: "<<*ctx.information.peer.peer_mid_quote<<"\n";
  if(!ctx.information.peer.visible_agent_ids.empty()){
   o<<"Public peer identities: ";
   for(std::size_t i=0;i<ctx.information.peer.visible_agent_ids.size();++i){
    if(i) o<<",";
    o<<ctx.information.peer.visible_agent_ids[i];
   }
   o<<"\n";
  }
  o<<"\n";
 }
 if(!ctx.information.history.empty()){
  o<<"AUTHORIZED MARKET HISTORY\n";
  for(const auto& h:ctx.information.history){
   o<<"t="<<h.time
    <<" last_trade="<<(h.last_trade?std::to_string(*h.last_trade):std::string("none"))
    <<" best_bid="<<(h.best_bid?std::to_string(*h.best_bid):std::string("none"))
    <<" best_ask="<<(h.best_ask?std::to_string(*h.best_ask):std::string("none"))<<"\n";
  }
  o<<"\n";
 }
 o<<"PERMITTED ACTIONS\n";
 if(c.allow_buy) o<<"BUY\n";
 if(c.allow_sell) o<<"SELL\n";
 if(c.allow_hold) o<<"HOLD\n";
 o<<"\nReturn ONLY a JSON object with keys action, asset, quantity, price, time.\n"
  <<"Use only the information supplied above. Do not infer or invent hidden information.\n"
  <<"time must equal the current market time and asset must match exactly.\n"
  <<"For HOLD use quantity 0 and price null.";
 return o.str();
}

std::string canonical_decision_context_json(const DecisionContext& ctx,
 const agents::ModelIdentity& /*model*/, const std::string& /*request_id*/, std::uint64_t /*seed*/){
 auto cons=ctx.information.constraints;
 if(cons.allow_buy && cons.allow_sell && cons.allow_hold) cons=ActionConstraints::for_role(ctx.agent.role);
 std::ostringstream o;
 o<<std::setprecision(17)
  <<"{\"market\":{\"time\":"<<ctx.market.time<<",\"asset\":\""<<esc(ctx.market.asset)
  <<"\",\"fundamental\":"<<ctx.market.fundamental<<",\"public_signal\":"<<ctx.market.public_signal
  <<",\"best_bid\":"<<opt_num(ctx.market.best_bid)<<",\"best_ask\":"<<opt_num(ctx.market.best_ask)
  <<",\"last_trade\":"<<opt_num(ctx.market.last_trade)<<"}"
  <<",\"agent\":{\"agent_id\":\""<<esc(ctx.agent.agent_id)<<"\",\"role\":\""<<role_name(ctx.agent.role)
  <<"\",\"cash\":"<<ctx.agent.cash
  <<",\"inventory\":"<<ctx.agent.inventory<<",\"private_value\":"<<ctx.agent.private_value
  <<",\"remaining_demand\":"<<ctx.agent.remaining_demand
  <<",\"realized_payoff\":"<<ctx.agent.realized_payoff<<"}"
  <<",\"information\":{";
 bool first=true;
 auto sep=[&]{ if(!first) o<<","; first=false; };
 if(ctx.information.news.present){
  sep(); o<<"\"news\":{\"present\":true,\"headline\":\""<<esc(ctx.information.news.headline)
   <<"\",\"signal\":"<<ctx.information.news.signal<<",\"reliability\":"<<ctx.information.news.reliability<<"}";
 }
 if(ctx.information.peer.peer_visibility){
  sep(); o<<"\"peer\":{\"visibility\":true,\"mid_quote\":"<<opt_num(ctx.information.peer.peer_mid_quote)<<",\"visible_agent_ids\":[";
  for(std::size_t i=0;i<ctx.information.peer.visible_agent_ids.size();++i){
   if(i) o<<",";
   o<<"\""<<esc(ctx.information.peer.visible_agent_ids[i])<<"\"";
  }
  o<<"]}";
 }
 if(!ctx.information.history.empty()){
  sep(); o<<"\"history\":[";
  for(std::size_t i=0;i<ctx.information.history.size();++i){
   if(i) o<<",";
   const auto& h=ctx.information.history[i];
   o<<"{\"time\":"<<h.time<<",\"last_trade\":"<<opt_num(h.last_trade)
    <<",\"best_bid\":"<<opt_num(h.best_bid)<<",\"best_ask\":"<<opt_num(h.best_ask)<<"}";
  }
  o<<"]";
 }
 sep(); o<<"\"constraints\":{\"allow_buy\":"<<(cons.allow_buy?"true":"false")
  <<",\"allow_sell\":"<<(cons.allow_sell?"true":"false")
  <<",\"allow_hold\":"<<(cons.allow_hold?"true":"false")<<"}";
 o<<"}}";
 return o.str();
}

MarketActionParseResult parse_canonical_market_action(const std::string& raw_output){
 MarketActionParseResult r; r.raw_retained=raw_output;
 std::string payload=raw_output;
 auto l=payload.find_first_not_of(" \t\r\n");
 if(l==std::string::npos){r.error="empty_payload";return r;}
 auto rr=payload.find_last_not_of(" \t\r\n"); payload=payload.substr(l,rr-l+1);
 if(payload.rfind("```",0)==0){auto nl=payload.find('\n');auto end=payload.rfind("```");if(nl==std::string::npos||end==std::string::npos||end<=nl){r.error="invalid_json";return r;}payload=payload.substr(nl+1,end-nl-1);l=payload.find_first_not_of(" \t\r\n");rr=payload.find_last_not_of(" \t\r\n");if(l==std::string::npos){r.error="empty_payload";return r;}payload=payload.substr(l,rr-l+1);}
 auto parsed=coagentics::util::parse_json(payload);
 if(!parsed){r.error="invalid_json";return r;} if(!parsed->is_object()){r.error="schema_root_not_object";return r;}
 const auto field=[&](const char*k){return parsed->get(k);};
 auto action=field("action"); if(!action){r.error="missing_action";return r;} if(!action->is_string()){r.error="action_wrong_type";return r;}
 MarketAction a; if(action->as_string()=="BUY")a.action=ActionType::Buy;else if(action->as_string()=="SELL")a.action=ActionType::Sell;else if(action->as_string()=="HOLD")a.action=ActionType::Hold;else{r.error="invalid_action";return r;}
 auto asset=field("asset"); if(!asset){r.error="missing_asset";return r;} if(!asset->is_string()){r.error="asset_wrong_type";return r;} if(asset->as_string().empty()){r.error="asset_empty";return r;} a.asset=asset->as_string();
 auto time=field("time"); if(!time){r.error="missing_time";return r;} if(!time->is_number()){r.error="time_wrong_type";return r;} double td=time->as_number(); if(td<0||std::floor(td)!=td||td>static_cast<double>(UINT64_MAX)){r.error="time_not_integer";return r;} a.time=static_cast<std::uint64_t>(td);
 auto qty=field("quantity"); auto price=field("price");
 if(a.action==ActionType::Hold){if(qty){if(!qty->is_number()||std::floor(qty->as_number())!=qty->as_number()){r.error="quantity_not_integer";return r;}if(qty->as_number()!=0){r.error="hold_quantity_must_be_zero";return r;}}if(price&&!price->is_null()){r.error="hold_price_must_be_null";return r;}a.quantity=0;r.ok=true;r.action=a;return r;}
 if(!qty){r.error="missing_quantity";return r;} if(!qty->is_number()||std::floor(qty->as_number())!=qty->as_number()){r.error="quantity_not_integer";return r;} if(qty->as_number()<=0||qty->as_number()>1000000){r.error="invalid_quantity";return r;} a.quantity=static_cast<int>(qty->as_number());
 if(!price||price->is_null()){r.error="missing_price";return r;} if(!price->is_number()){r.error="price_wrong_type";return r;} if(!std::isfinite(price->as_number())){r.error="price_not_finite";return r;} a.price=price->as_number();
 r.ok=true;r.action=a;return r;
}

ActionValidationResult validate_market_action(const MarketAction& action, const DecisionContext& ctx){
 ActionValidationResult r;
 auto fail=[&](std::string e){ r.errors.push_back(std::move(e)); };
 if(action.time!=ctx.market.time) fail("time_mismatch");
 if(action.asset!=ctx.market.asset) fail("asset_mismatch");

 auto cons=ctx.information.constraints;
 if(cons.allow_buy && cons.allow_sell && cons.allow_hold)
  cons=ActionConstraints::for_role(ctx.agent.role);

 if(action.action==ActionType::Hold){
  if(!cons.allow_hold) fail("hold_not_permitted");
  if(action.quantity!=0) fail("hold_quantity_must_be_zero");
  if(action.price) fail("hold_price_must_be_null");
 }else if(action.action==ActionType::Buy){
  if(!cons.allow_buy) fail("buy_not_permitted_for_role");
  if(ctx.agent.role==AgentRole::Seller && !ctx.information.constraints.allow_buy)
   fail("buy_not_permitted_for_role");
  if(action.quantity<=0 || action.quantity>1000000) fail("invalid_quantity");
  if(!action.price || !std::isfinite(*action.price) || *action.price<0 || *action.price>1e12) fail("invalid_price");
 }else if(action.action==ActionType::Sell){
  if(!cons.allow_sell) fail("sell_not_permitted_for_role");
  if(action.quantity<=0 || action.quantity>1000000) fail("invalid_quantity");
  if(!action.price || !std::isfinite(*action.price) || *action.price<0 || *action.price>1e12) fail("invalid_price");
 }
 r.valid=r.errors.empty();
 return r;
}

agents::LlmAction to_llm_action(const MarketAction& a){
 agents::LlmAction out; out.time=a.time; out.asset=a.asset; out.quantity=a.quantity;
 out.price=a.price.value_or(0); out.abstain=(a.action==ActionType::Hold);
 out.side=(a.action==ActionType::Sell)?market::Side::Sell:market::Side::Buy;
 return out;
}

std::optional<market::Bid> to_bid(const MarketAction& a, const std::string& agent_id){
 if(a.action==ActionType::Hold || !a.price) return std::nullopt;
 return market::Bid{a.time, agent_id, a.asset, a.quantity, *a.price,
  a.action==ActionType::Sell?market::Side::Sell:market::Side::Buy};
}

LlmAgentAdapter::LlmAgentAdapter(agents::ModelIdentity identity,
 std::shared_ptr<agents::ModelTransport> transport,
 std::uint64_t seed, std::string adapter_version, std::string log_path, std::string run_id)
 : identity_(std::move(identity)), transport_(std::move(transport)), seed_(seed),
   adapter_version_(std::move(adapter_version)), log_path_(std::move(log_path)), run_id_(std::move(run_id)) {
 if(!transport_) throw std::invalid_argument("LlmAgentAdapter requires transport");
}

AgentTurnRecord LlmAgentAdapter::decide(const DecisionContext& ctx){
 AgentTurnRecord turn;
 turn.run_id=run_id_;
 turn.agent_id=ctx.agent.agent_id;
 turn.time=ctx.market.time;
 turn.market_state=ctx.market;
 turn.agent_state_before=ctx.agent;
 turn.model=identity_;
 turn.seed=seed_;
 turn.adapter_version=adapter_version_;
 turn.request_id=identity_.provider+":"+identity_.model+":"+std::to_string(seed_)+":"+std::to_string(seq_++);
 turn.canonical_request=canonical_decision_context_json(ctx, identity_, turn.request_id, seed_);

 agents::ModelRequest req;
 req.request_id=turn.request_id;
 req.agent_id=ctx.agent.agent_id;
 req.model=identity_;
 req.observation=to_public_observation(ctx.market);
 req.seed=seed_;
 req.decision_context_json=turn.canonical_request;

 agents::ModelResponse resp=transport_->invoke(req);
 resp.request_id=turn.request_id;
 turn.raw_provider_response=resp.raw_output;
 turn.latency_ms=resp.latency_ms;
 turn.prompt_tokens=resp.prompt_tokens;
 turn.completion_tokens=resp.completion_tokens;
 turn.inference_config=resp.inference_config;

 if(!resp.error.empty() && !resp.action && resp.raw_output.empty()){
  turn.parse.success=false; turn.parse.error=resp.error;
  ++provider_failures_;
  turn.agent_state_after=ctx.agent;
  turns_.push_back(turn); append_log(turn); return turn;
 }

 MarketActionParseResult parsed;
 if(resp.action){
  // Pre-parsed harness actions map into MarketAction.
  MarketAction a;
  if(resp.action->abstain) a.action=ActionType::Hold;
  else a.action=(resp.action->side==market::Side::Sell)?ActionType::Sell:ActionType::Buy;
  a.asset=resp.action->asset; a.quantity=resp.action->quantity; a.time=resp.action->time;
  if(!resp.action->abstain) a.price=resp.action->price;
  parsed.ok=true; parsed.action=a; parsed.raw_retained=resp.raw_output;
 }else{
  parsed=parse_canonical_market_action(resp.raw_output);
 }
 turn.parse.success=parsed.ok;
 turn.parse.error=parsed.error;
 if(!parsed.ok){
  ++parse_failures_;
  turn.agent_state_after=ctx.agent;
  turns_.push_back(turn); append_log(turn); return turn;
 }
 turn.parsed_action=parsed.action;
 turn.action_validation=validate_market_action(*turn.parsed_action, ctx);
 if(!turn.action_validation.valid){
  ++action_validation_failures_;
  turn.agent_state_after=ctx.agent;
  turns_.push_back(turn); append_log(turn); return turn;
 }
 if(turn.parsed_action->action==ActionType::Hold){
  turn.held=true; ++holds_;
 }
 turn.agent_state_after=ctx.agent;
 turns_.push_back(turn);
 // Defer JSONL until caller finishes submission + AgentStateAfter (persist_turn).
 return turn;
}

void LlmAgentAdapter::append_log(const AgentTurnRecord& t) const {
 if(log_path_.empty()) return;
 std::ofstream f(log_path_, std::ios::app);
 auto write_action=[&](const std::optional<MarketAction>& a){
  if(!a){ f<<"null"; return; }
  f<<"{\"action\":\""<<(a->action==ActionType::Buy?"BUY":a->action==ActionType::Sell?"SELL":"HOLD")
   <<"\",\"asset\":\""<<esc(a->asset)<<"\",\"quantity\":"<<a->quantity
   <<",\"price\":"<<opt_num(a->price)<<",\"time\":"<<a->time<<"}";
 };
 auto write_agent=[&](const AgentState& a){
  f<<"{\"agent_id\":\""<<esc(a.agent_id)<<"\",\"role\":\""<<role_name(a.role)
   <<"\",\"cash\":"<<a.cash<<",\"inventory\":"<<a.inventory
   <<",\"private_value\":"<<a.private_value<<",\"remaining_demand\":"<<a.remaining_demand
   <<",\"realized_payoff\":"<<a.realized_payoff
   <<",\"previous_action\":"; write_action(a.previous_action);
  f<<",\"previous_fill\":";
  if(!a.previous_fill) f<<"null";
  else{
   f<<"{\"filled_quantity\":"<<a.previous_fill->filled_quantity
    <<",\"average_fill_price\":"<<opt_num(a.previous_fill->average_fill_price)
    <<",\"trades\":"<<a.previous_fill->trades.size()<<"}";
  }
  f<<"}";
 };
 f<<"{\"run_id\":\""<<esc(t.run_id)<<"\",\"request_id\":\""<<esc(t.request_id)
  <<"\",\"agent_id\":\""<<esc(t.agent_id)<<"\",\"time\":"<<t.time
  <<",\"seed\":"<<t.seed
  <<",\"inference_config\":\""<<esc(t.inference_config)<<"\""
  <<",\"adapter_version\":\""<<esc(t.adapter_version)<<"\""
  <<",\"model\":{\"provider\":\""<<esc(t.model.provider)<<"\",\"name\":\""<<esc(t.model.model)
  <<"\",\"version\":\""<<esc(t.model.version)<<"\"}"
  <<",\"market_state\":{\"time\":"<<t.market_state.time<<",\"asset\":\""<<esc(t.market_state.asset)
  <<"\",\"fundamental\":"<<t.market_state.fundamental
  <<",\"public_signal\":"<<t.market_state.public_signal
  <<",\"best_bid\":"<<opt_num(t.market_state.best_bid)
  <<",\"best_ask\":"<<opt_num(t.market_state.best_ask)
  <<",\"last_trade\":"<<opt_num(t.market_state.last_trade)<<"}"
  <<",\"agent_state_before\":"; write_agent(t.agent_state_before);
  f<<",\"agent_state_after\":"; write_agent(t.agent_state_after);
  f<<",\"canonical_request\":"<< (t.canonical_request.empty()?"null":t.canonical_request)
  <<",\"raw_provider_response\":\""<<esc(t.raw_provider_response)<<"\""
  <<",\"parsed_action\":"; write_action(t.parsed_action);
  f<<",\"parse\":{\"success\":"<<(t.parse.success?"true":"false")<<",\"error\":\""<<esc(t.parse.error)<<"\"}"
  <<",\"action_validation\":{\"valid\":"<<(t.action_validation.valid?"true":"false")<<",\"errors\":[";
 for(size_t i=0;i<t.action_validation.errors.size();++i){
  if(i) f<<",";
  f<<"\""<<esc(t.action_validation.errors[i])<<"\"";
 }
 f<<"]}"
  <<",\"submission\":{\"accepted\":"<<(t.submission.accepted?"true":"false")
  <<",\"rejection_reason\":\""<<esc(t.submission.rejection_reason)
  <<"\",\"submitted_quantity\":"<<t.submission.submitted_quantity
  <<",\"filled_quantity\":"<<t.submission.filled_quantity
  <<",\"resting_quantity\":"<<t.submission.resting_quantity
  <<",\"average_fill_price\":"<<opt_num(t.submission.average_fill_price)
  <<",\"trades\":"<<t.submission.trades.size()<<"}"
  <<",\"latency_ms\":"<<t.latency_ms
  <<",\"prompt_tokens\":"<<t.prompt_tokens
  <<",\"completion_tokens\":"<<t.completion_tokens<<"}\n";
}

agents::ModelResponse RawJsonTransport::invoke(const agents::ModelRequest& q){
 agents::ModelResponse r; r.request_id=q.request_id;
 if(next_>=payloads_.size()){ r.error="raw_json_exhausted"; return r; }
 r.raw_output=payloads_[next_++];
 r.latency_ms=1;
 r.inference_config="transport=RawJsonTransport";
 return r;
}

bool live_uses_ollama(){
 const std::string provider=getenv_str("COAGENTICS_LLM_PROVIDER");
 if(provider=="ollama" || provider=="OLLAMA") return true;
 const std::string base=getenv_str("COAGENTICS_LLM_BASE_URL");
 return base.find("11434")!=std::string::npos || base.find("ollama")!=std::string::npos;
}

bool live_llm_configured(){
 if(!getenv_str("OPENAI_API_KEY").empty() || !getenv_str("COAGENTICS_LLM_API_KEY").empty()) return true;
 // Local Ollama needs no cloud API key.
 return live_uses_ollama();
}

namespace {
class OpenAiCompatibleTransport final : public agents::ModelTransport {
public:
 explicit OpenAiCompatibleTransport(agents::ModelIdentity identity):identity_(std::move(identity)){
  // Snapshot endpoint/model credentials at construction. A shared heterogeneous market may
  // contain multiple model transports; invoke() must not read a later seat's mutable env.
  provider_=getenv_str("COAGENTICS_LLM_PROVIDER");
  base_=getenv_str("COAGENTICS_LLM_BASE_URL");
  model_=getenv_str("COAGENTICS_LLM_MODEL");
  key_=getenv_str("COAGENTICS_LLM_API_KEY");
  if(key_.empty()) key_=getenv_str("OPENAI_API_KEY");
 }
 agents::ModelResponse invoke(const agents::ModelRequest& req) override {
  agents::ModelResponse out; out.request_id=req.request_id;
  const bool ollama=(provider_=="ollama" || provider_=="OLLAMA" || base_.find("11434")!=std::string::npos || base_.find("ollama")!=std::string::npos);
  std::string key=key_;
  if(key.empty() && ollama) key="ollama";
  if(key.empty()){ out.error="missing_api_key"; return out; }
  std::string base=base_;
  if(base.empty()) base=ollama? "http://127.0.0.1:11434/v1" : "https://api.openai.com/v1";
  while(!base.empty()&&base.back()=='/') base.pop_back();
  std::string model=model_;
  if(model.empty()){
   if(!identity_.model.empty()) model=identity_.model;
   else model=ollama? "llama3.2" : "gpt-4o-mini";
  }

  // Prefer decision context (market + own private state). Fall back to public observation JSON.
  const std::string user_payload = !req.decision_context_json.empty()
   ? req.decision_context_json : agents::canonical_observation_json(req);
  // The system message defines only the interface contract.  It intentionally does NOT
  // prescribe a trading heuristic: behavioral choice is the experimental observation.
  const std::string system =
   "You are participating in an economic market experiment. Use only the supplied observation. "
   "Choose one permitted action and reply with ONLY one JSON object, no markdown: "
   "{\"action\":\"BUY|SELL|HOLD\",\"asset\":\"ASSET\",\"quantity\":1,\"price\":90,\"time\":0}. "
   "Copy asset and time from market.asset and market.time exactly. "
   "Respect the supplied action constraints. HOLD uses quantity 0 and price null. "
   "Never invent information that was not supplied.";

  std::ostringstream body;
  body<<"{\"model\":\""<<esc(model)<<"\",\"temperature\":0,";
  // OpenAI-compatible/Ollama endpoints support seed in the request schema.  The simulator
  // seed and provider inference seed are the same value here and are recorded separately.
  body<<"\"seed\":"<<req.seed<<",";
  body<<"\"messages\":[{\"role\":\"system\",\"content\":\""<<esc(system)
      <<"\"},{\"role\":\"user\",\"content\":\""<<esc(user_payload)<<"\"}]}";

  const std::string url=base+"/chat/completions";
  const int timeout_s=ollama?180:60;
  std::string cmd="curl -sS --max-time "+std::to_string(timeout_s)+" -X POST "+shell_quote(url)
   +" -H "+shell_quote("Authorization: Bearer "+key)
   +" -H "+shell_quote("Content-Type: application/json")
   +" -d "+shell_quote(body.str());
  out.inference_config="transport=OpenAiCompatible;provider="+(ollama?std::string("ollama"):std::string("openai-compatible"))
   +";model="+model+";base="+base+";temperature=0;inference_seed="+std::to_string(req.seed);
  auto t0=std::chrono::steady_clock::now();
  std::string raw=run_cmd(cmd);
  out.latency_ms=static_cast<std::uint64_t>(
   std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-t0).count());
  if(raw.empty()){ out.error="empty_provider_response"; return out; }
  out.raw_output=extract_message_content(raw);
  if(out.raw_output.empty()){ out.error="missing_message_content"; out.raw_output=raw; return out; }
  if(auto pt=provider_number(raw,"usage","prompt_tokens")) out.prompt_tokens=static_cast<std::uint64_t>(*pt);
  if(auto ct=provider_number(raw,"usage","completion_tokens")) out.completion_tokens=static_cast<std::uint64_t>(*ct);
  return out;
 }
private:
 agents::ModelIdentity identity_;
 std::string provider_, base_, model_, key_;
 static std::string shell_quote(const std::string& s){
  std::string o="'"; for(char c:s){ if(c=='\'') o+="'\\''"; else o.push_back(c);} o.push_back('\''); return o;
 }
 static std::string run_cmd(const std::string& cmd){
  FILE* p=popen(cmd.c_str(),"r"); if(!p) return {};
  std::string out; char buf[4096]; while(fgets(buf,sizeof buf,p)) out+=buf; pclose(p); return out;
 }
 static std::string extract_message_content(const std::string& raw){
  auto j=coagentics::util::parse_json(raw); if(!j||!j->is_object()) return {};
  auto choices=j->get("choices"); if(!choices||!choices->is_array()||choices->as_array().empty()) return {};
  auto msg=choices->as_array().front().get("message"); if(!msg||!msg->is_object()) return {};
  auto content=msg->get("content"); return content&&content->is_string()?content->as_string():std::string{};
 }
 static std::optional<double> provider_number(const std::string& raw,const char* parent,const char* key){
  auto j=coagentics::util::parse_json(raw);if(!j)return {};auto p=j->get(parent);if(!p||!p->is_object())return {};auto v=p->get(key);if(!v||!v->is_number())return {};return v->as_number();
 }
};
}

std::shared_ptr<agents::ModelTransport> make_live_openai_compatible_transport(const agents::ModelIdentity& identity){
 return std::make_shared<OpenAiCompatibleTransport>(identity);
}

RunResult run_llm_market_experiment(const ExperimentSpec& experiment, const RunSpec& run){
 if(!run.transport) throw std::invalid_argument("RunSpec.transport required");
 RunResult result;
 result.experiment_id=experiment.experiment_id;
 result.run_id=run.run_id.empty()? (experiment.experiment_id+":"+std::to_string(run.seed)) : run.run_id;
 result.seed=run.seed;
 result.model=run.model;
 result.starting_cash=experiment.starting_cash;
 result.starting_inventory=(experiment.llm_role==AgentRole::Seller?experiment.counterparty_quantity:0);

 market::MarketConfig mc;
 mc.fundamental_value[experiment.asset]=experiment.fundamental;
 mc.total_supply[experiment.asset]=experiment.counterparty_quantity;
 if(experiment.llm_role==AgentRole::Buyer){
  mc.private_values.buy_values[run.llm_agent_id+"\n"+experiment.asset]={experiment.llm_private_value};
  mc.private_values.sell_costs[run.seller_agent_id+"\n"+experiment.asset]={experiment.seller_cost};
 }else{
  mc.private_values.sell_costs[run.llm_agent_id+"\n"+experiment.asset]={experiment.llm_private_value};
  mc.private_values.buy_values[run.seller_agent_id+"\n"+experiment.asset]={experiment.seller_cost};
 }
 auto mech=market::make_mechanism(experiment.mechanism, mc);
 if(experiment.llm_role==AgentRole::Buyer){
  mech->add_account(run.llm_agent_id,{experiment.starting_cash,{}});
  mech->add_account(run.seller_agent_id,{0,{{experiment.asset, experiment.counterparty_quantity}}});
 }else{
  mech->add_account(run.llm_agent_id,{experiment.starting_cash,{{experiment.asset, experiment.counterparty_quantity}}});
  mech->add_account(run.seller_agent_id,{experiment.starting_cash,{}});
 }

 LlmAgentAdapter adapter(run.model, run.transport, run.seed, run.adapter_version, run.log_path, result.run_id);
 std::optional<MarketAction> prev_action;
 std::optional<FillSummary> prev_fill;
 std::vector<MarketHistoryEntry> history;
 double llm_payoff=0;
 int remaining=experiment.counterparty_quantity;

 auto snap_market=[&](std::uint64_t t)->MarketState{
  MarketState s; s.time=t; s.asset=experiment.asset; s.fundamental=experiment.fundamental;
  s.best_bid=mech->best_bid_opt(experiment.asset);
  s.best_ask=mech->best_ask_opt(experiment.asset);
  if(!mech->trades().empty()){
   for(auto it=mech->trades().rbegin(); it!=mech->trades().rend(); ++it){
    if(it->asset==experiment.asset){ s.last_trade=it->price; break; }
   }
  }
  return s;
 };

 for(int r=0;r<experiment.rounds;++r){
  const std::uint64_t t=static_cast<std::uint64_t>(r);
  if(experiment.deterministic_counterparty){
   const market::Side side = experiment.llm_role==AgentRole::Buyer ? market::Side::Sell : market::Side::Buy;
   market::Bid cb{t, run.seller_agent_id, experiment.asset, experiment.counterparty_quantity,
    experiment.counterparty_limit_price, side};
   auto cres=mech->submit_detailed(cb);
   if(cres.accepted) result.accepted_bids.push_back(cb);
  }else{
   const market::Side side = experiment.llm_role==AgentRole::Buyer ? market::Side::Sell : market::Side::Buy;
   const double limit = experiment.llm_role==AgentRole::Buyer ? experiment.seller_cost : experiment.seller_cost;
   agents::ZeroIntelligenceAgent counter(run.seller_agent_id, limit, side, run.seed+17);
   auto ms=snap_market(t);
   auto cb=counter.act(to_public_observation(ms));
   if(mech->submit(cb)) result.accepted_bids.push_back(cb);
  }

  auto market_state=snap_market(t);
  auto agent_before=build_agent_state_from_accounts(mech->accounts(), run.llm_agent_id, experiment.asset,
   experiment.llm_private_value, remaining, experiment.llm_role, prev_action, prev_fill, llm_payoff);
  InformationContext info=experiment.information;
  info.history=history;
  DecisionContext ctx{market_state, agent_before, info};
  if(ctx.information.constraints.allow_buy && ctx.information.constraints.allow_sell
     && ctx.information.constraints.allow_hold){
   ctx.information.constraints=ActionConstraints::for_role(experiment.llm_role);
  }

  AgentTurnRecord turn=adapter.decide(ctx);
  if(turn.action_validation.valid && turn.parsed_action && turn.parsed_action->action!=ActionType::Hold){
   auto bid=to_bid(*turn.parsed_action, run.llm_agent_id);
   if(bid){
    ++result.submitted_actions;
    turn.submission=mech->submit_detailed(*bid);
    if(turn.submission.accepted){
     ++result.accepted_actions;
     result.accepted_bids.push_back(*bid);
     result.units_filled+=static_cast<std::size_t>(turn.submission.filled_quantity);
     if(turn.submission.filled_quantity>0 && turn.submission.average_fill_price){
      if(experiment.llm_role==AgentRole::Buyer)
       llm_payoff += (experiment.llm_private_value - *turn.submission.average_fill_price) * turn.submission.filled_quantity;
      else
       llm_payoff += (*turn.submission.average_fill_price - experiment.llm_private_value) * turn.submission.filled_quantity;
      remaining = std::max(0, remaining - turn.submission.filled_quantity);
     }
     prev_fill=FillSummary{turn.submission.filled_quantity, turn.submission.average_fill_price, turn.submission.trades};
    }else{
     ++result.rejected_actions;
     ++result.market_rejections;
    }
   }
  }else if(turn.held){
   turn.submission.accepted=false;
   turn.submission.rejection_reason="hold_no_submission";
  }

  if(experiment.mechanism==market::MechanismKind::SealedBidDoubleAuction){
   const std::size_t before_trades=mech->trades().size();
   mech->clear(t);
   int filled=0; double notional=0; std::vector<market::Trade> own_trades;
   for(std::size_t i=before_trades;i<mech->trades().size();++i){
    const auto& tr=mech->trades()[i];
    if(tr.buyer==run.llm_agent_id || tr.seller==run.llm_agent_id){
     filled+=tr.quantity; notional+=tr.price*tr.quantity; own_trades.push_back(tr);
    }
   }
   if(filled>0){
    turn.submission.filled_quantity=filled;
    turn.submission.average_fill_price=notional/filled;
    turn.submission.trades=own_trades;
    turn.submission.accepted=true;
    result.units_filled+=static_cast<std::size_t>(filled);
    if(experiment.llm_role==AgentRole::Buyer) llm_payoff += (experiment.llm_private_value-turn.submission.average_fill_price.value())*filled;
    else llm_payoff += (turn.submission.average_fill_price.value()-experiment.llm_private_value)*filled;
    remaining=std::max(0, remaining-filled);
    prev_fill=FillSummary{filled, turn.submission.average_fill_price, own_trades};
   }
  }

  prev_action=turn.parsed_action;
  turn.agent_state_after=build_agent_state_from_accounts(mech->accounts(), run.llm_agent_id, experiment.asset,
   experiment.llm_private_value, remaining, experiment.llm_role, prev_action, prev_fill, llm_payoff);
  adapter.persist_turn(turn);
  result.turns.push_back(turn);
  MarketHistoryEntry he; he.time=t; he.best_bid=mech->best_bid_opt(experiment.asset); he.best_ask=mech->best_ask_opt(experiment.asset);
  if(!mech->trades().empty()){ for(auto it=mech->trades().rbegin();it!=mech->trades().rend();++it){ if(it->asset==experiment.asset){ he.last_trade=it->price; break; } } }
  history.push_back(he);
 }

 result.parse_failures=adapter.parse_failures();
 result.action_validation_failures=adapter.action_validation_failures();
 result.provider_failures=adapter.provider_failures();
 result.holds=adapter.holds();
 result.trades=mech->trades();
 result.metrics=mech->metrics();
 result.llm_realized_payoff=llm_payoff;
 auto ait=mech->accounts().find(run.llm_agent_id);
 if(ait!=mech->accounts().end()){
  result.ending_cash=ait->second.cash;
  auto inv=ait->second.inventory.find(experiment.asset);
  result.ending_inventory=inv==ait->second.inventory.end()?0:inv->second;
 }

 analysis::EvidenceEnvelope env;
 env.experiment_id=experiment.experiment_id;
 env.control_run_id="n/a";
 env.treatment_run_id=result.run_id;
 env.seed=run.seed;
 env.evidence.hypothesis_id="H_LLM_MARKET_SLICE";
 env.evidence.discriminator_id="D_PROVENANCE_CHAIN";
 const bool pathway_ok=!result.turns.empty() && result.turns.front().parse.success
  && result.turns.front().action_validation.valid;
 env.evidence.direction=pathway_ok?analysis::EvidenceDirection::Supports:analysis::EvidenceDirection::Inconclusive;
 env.evidence.rationale=
  "Traceability-only claim: LLM decision pathway executed with parse/action-validation/"
  "market-acceptance/execution stages recorded via MarketMechanism. Does NOT claim economic rationality. "
  "mechanism="+(experiment.mechanism==market::MechanismKind::SealedBidDoubleAuction
   ?std::string("sealed"):std::string("cda"))
  +" provider="+run.model.provider+" model="+run.model.model+" version="+run.model.version
  +" adapter="+run.adapter_version
  +" turns="+std::to_string(result.turns.size())
  +" accepted_actions="+std::to_string(result.accepted_actions)
  +" units_filled="+std::to_string(result.units_filled)
  +" parse_failures="+std::to_string(result.parse_failures)
  +" action_validation_failures="+std::to_string(result.action_validation_failures)
  +" market_rejections="+std::to_string(result.market_rejections);
 result.evidence.append(std::move(env));
 return result;
}
}
