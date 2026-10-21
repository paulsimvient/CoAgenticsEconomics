#include "coagentics/experiment/LlmExperiment.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
using namespace coagentics;
using namespace coagentics::experiment;

static ExperimentSpec base_exp(){
 ExperimentSpec e;
 e.deterministic_counterparty=true;
 e.counterparty_limit_price=90;
 e.counterparty_quantity=1;
 e.rounds=1;
 e.llm_private_value=120;
 e.starting_cash=10000;
 return e;
}

static RunSpec base_run(std::shared_ptr<agents::ModelTransport> t, std::uint64_t seed=424242){
 RunSpec r; r.seed=seed; r.model={"mock-provider","mock-model","test-v1"}; r.transport=std::move(t);
 r.adapter_version="coagentics-llm-adapter/0.1"; return r;
}

static void parsing_valid_action(){
 auto p=parse_canonical_market_action(R"({"action":"BUY","asset":"A","quantity":1,"price":82.0,"time":5})");
 assert(p.ok && p.action && p.action->action==ActionType::Buy);
 assert(p.action->asset=="A" && p.action->quantity==1 && p.action->time==5);
 assert(p.action->price && std::fabs(*p.action->price-82.0)<1e-12);
 auto h=parse_canonical_market_action(R"({"action":"HOLD","asset":"A","quantity":0,"price":null,"time":5})");
 assert(h.ok && h.action && h.action->action==ActionType::Hold && !h.action->price);
}

static void parsing_invalid_json(){
 const std::string raw="not-json-at-all";
 auto p=parse_canonical_market_action(raw);
 assert(!p.ok);
 assert(p.error=="invalid_json");
 assert(p.raw_retained==raw);
}

static void validation_wrong_asset(){
 auto p=parse_canonical_market_action(R"({"action":"BUY","asset":"WRONG","quantity":1,"price":95,"time":0})");
 assert(p.ok);
 DecisionContext ctx;
 ctx.market={0,"ASSET",100,0,std::nullopt,std::nullopt,std::nullopt};
 ctx.agent={"LLM-0",AgentRole::Buyer,10000,0,120,1,{},{},0};
 ctx.information.constraints=ActionConstraints::for_role(AgentRole::Buyer);
 auto v=validate_market_action(*p.action, ctx);
 assert(!v.valid);
 assert(std::find(v.errors.begin(),v.errors.end(),"asset_mismatch")!=v.errors.end());
}

static void validation_wrong_time(){
 auto p=parse_canonical_market_action(R"({"action":"BUY","asset":"ASSET","quantity":1,"price":95,"time":9})");
 assert(p.ok);
 DecisionContext ctx;
 ctx.market={0,"ASSET",100,0,{},{},{}};
 ctx.agent={"LLM-0",AgentRole::Buyer,10000,0,120,1,{},{},0};
 ctx.information.constraints=ActionConstraints::for_role(AgentRole::Buyer);
 auto v=validate_market_action(*p.action, ctx);
 assert(!v.valid);
 assert(std::find(v.errors.begin(),v.errors.end(),"time_mismatch")!=v.errors.end());
}

static void parsing_rejects_fractional_time(){
 auto p=parse_canonical_market_action(R"({"action":"BUY","asset":"A","quantity":1,"price":82.0,"time":0.5})");
 assert(!p.ok);
 assert(p.error=="time_not_integer");
}

static void insufficient_cash(){
 market::Market m({{{"ASSET",100.0}},{{"ASSET",1}}});
 m.add_account("b",{50,{}}); // cannot afford 1@100
 auto r=m.submit_detailed({0,"b","ASSET",1,100,market::Side::Buy});
 assert(!r.accepted);
 assert(r.rejection_reason=="insufficient_cash");
 assert(r.filled_quantity==0);
}

static void insufficient_inventory(){
 market::Market m({{{"ASSET",100.0}},{{"ASSET",1}}});
 m.add_account("s",{0,{}}); // no inventory
 auto r=m.submit_detailed({0,"s","ASSET",1,90,market::Side::Sell});
 assert(!r.accepted);
 assert(r.rejection_reason=="insufficient_inventory");
}

static void sealed_rejects_insufficient_cash_and_inventory(){
 market::MarketConfig cfg;
 cfg.fundamental_value["ASSET"]=100;
 cfg.total_supply["ASSET"]=1;
 auto mech=market::make_mechanism(market::MechanismKind::SealedBidDoubleAuction, cfg);
 mech->add_account("b",{50,{}});
 mech->add_account("s",{0,{}});
 auto buy=mech->submit_detailed({0,"b","ASSET",1,100,market::Side::Buy});
 assert(!buy.accepted && buy.rejection_reason=="insufficient_cash");
 auto sell=mech->submit_detailed({0,"s","ASSET",1,90,market::Side::Sell});
 assert(!sell.accepted && sell.rejection_reason=="insufficient_inventory");
}

static void agent_visible_payload_excludes_audit_metadata_and_includes_history(){
 DecisionContext ctx;
 ctx.market={2,"ASSET",100,0,{95.0},{97.0},{96.0}};
 ctx.agent={"LLM-0",AgentRole::Buyer,10000,0,120,1,{},{},0};
 ctx.information.history.push_back({1,95.0,94.0,96.0});
 ctx.information.history.push_back({2,96.0,95.0,97.0});
 ctx.information.information_condition="treatment_news_history";
 auto payload=canonical_decision_context_json(ctx,{"mock","model","v1"},"req-123",999);
 assert(payload.find("req-123")==std::string::npos);
 assert(payload.find("mock")==std::string::npos);
 assert(payload.find("999")==std::string::npos);
 assert(payload.find("treatment_news_history")==std::string::npos);
 assert(payload.find("\"history\":[")!=std::string::npos);
 assert(payload.find("\"time\":1")!=std::string::npos);
 assert(payload.find("\"time\":2")!=std::string::npos);
}

static void seller_role_executes_with_own_inventory(){
 auto transport=std::make_shared<RawJsonTransport>(std::vector<std::string>{
  R"({"action":"SELL","asset":"ASSET","quantity":1,"price":85,"time":0})"
 });
 auto exp=base_exp();
 exp.llm_role=AgentRole::Seller;
 exp.llm_private_value=80;
 exp.seller_cost=120;
 exp.counterparty_limit_price=90;
 auto result=run_llm_market_experiment(exp, base_run(transport));
 assert(result.turns.size()==1);
 assert(result.turns[0].action_validation.valid);
 assert(result.turns[0].submission.accepted);
 assert(result.turns[0].submission.filled_quantity==1);
 assert(result.ending_inventory==0);
 assert(result.turns[0].agent_state_before.role==AgentRole::Seller);
}

static void accepted_but_not_filled(){
 market::Market m({{{"ASSET",100.0}},{{"ASSET",1}}});
 m.add_account("b",{10000,{}});
 m.add_account("s",{0,{{"ASSET",1}}});
 // Bid below ask: accepted, rests, no trade.
 auto buy=m.submit_detailed({0,"b","ASSET",1,80,market::Side::Buy});
 assert(buy.accepted);
 assert(buy.filled_quantity==0);
 assert(buy.resting_quantity==1);
 assert(m.trades().empty());
 auto sell=m.submit_detailed({0,"s","ASSET",1,90,market::Side::Sell});
 assert(sell.accepted);
 assert(sell.filled_quantity==0);
 assert(sell.resting_quantity==1);
 assert(m.trades().empty());
}

static void guaranteed_trade(){
 // seller @90, buyer @95 must cross at mid 92.5
 market::MarketConfig cfg;
 cfg.fundamental_value["ASSET"]=100;
 cfg.total_supply["ASSET"]=1;
 cfg.private_values.buy_values["b\nASSET"]={120};
 cfg.private_values.sell_costs["s\nASSET"]={80};
 market::Market market(cfg);
 market.add_account("b",{10000,{}});
 market.add_account("s",{0,{{"ASSET",1}}});
 auto s=market.submit_detailed({0,"s","ASSET",1,90,market::Side::Sell});
 assert(s.accepted && s.filled_quantity==0 && s.resting_quantity==1);
 auto b=market.submit_detailed({0,"b","ASSET",1,95,market::Side::Buy});
 assert(b.accepted);
 assert(b.filled_quantity==1);
 assert(b.resting_quantity==0);
 assert(b.average_fill_price && std::fabs(*b.average_fill_price-92.5)<1e-12);
 assert(market.trades().size()==1);
 assert(market.accounts().at("b").inventory.at("ASSET")==1);
 assert(market.accounts().at("s").inventory.at("ASSET")==0);
}

static void private_information_boundary(){
 auto transport=std::make_shared<RawJsonTransport>(std::vector<std::string>{
  R"({"action":"BUY","asset":"ASSET","quantity":1,"price":95,"time":0})"
 });
 auto exp=base_exp();
 exp.seller_cost=77.7; // must NOT appear in LLM context
 auto run=base_run(transport);
 auto result=run_llm_market_experiment(exp, run);
 assert(result.turns.size()==1);
 const auto& t=result.turns[0];
 assert(t.agent_state_before.private_value==exp.llm_private_value);
 assert(t.canonical_request.find("\"private_value\":120")!=std::string::npos
     || t.canonical_request.find("\"private_value\":120.0")!=std::string::npos
     || t.canonical_request.find(std::to_string(exp.llm_private_value))!=std::string::npos);
 // Other agent's private cost / identity must not leak into the LLM request.
 assert(t.canonical_request.find("77.7")==std::string::npos);
 assert(t.canonical_request.find(run.seller_agent_id)==std::string::npos);
 assert(t.canonical_request.find("sell_costs")==std::string::npos);
 // Public market + own cash/inventory present.
 assert(t.canonical_request.find("\"cash\":")!=std::string::npos);
 assert(t.canonical_request.find("\"inventory\":")!=std::string::npos);
 assert(t.market_state.asset=="ASSET");
}

static void deterministic_replay(){
 auto make=[&]{
  auto transport=std::make_shared<RawJsonTransport>(std::vector<std::string>{
   R"({"action":"BUY","asset":"ASSET","quantity":1,"price":95,"time":0})"
  });
  return run_llm_market_experiment(base_exp(), base_run(transport, 999));
 };
 auto a=make(); auto b=make();
 assert(a.turns.size()==b.turns.size());
 assert(a.trades.size()==b.trades.size());
 assert(a.turns[0].submission.filled_quantity==b.turns[0].submission.filled_quantity);
 assert(a.turns[0].parsed_action->price==b.turns[0].parsed_action->price);
 assert(std::fabs(a.metrics.allocative_efficiency-b.metrics.allocative_efficiency)<1e-12);
}

static void sealed_mechanism_llm_slice(){
 auto transport=std::make_shared<RawJsonTransport>(std::vector<std::string>{
  R"({"action":"BUY","asset":"ASSET","quantity":1,"price":95,"time":0})"
 });
 auto exp=base_exp();
 exp.mechanism=market::MechanismKind::SealedBidDoubleAuction;
 auto run=base_run(transport);
 auto result=run_llm_market_experiment(exp, run);
 assert(result.turns.size()==1);
 assert(result.turns[0].parse.success);
 assert(result.turns[0].action_validation.valid);
 assert(result.evidence.records()[0].evidence.rationale.find("MarketMechanism")!=std::string::npos);
 assert(result.evidence.records()[0].evidence.rationale.find("mechanism=sealed")!=std::string::npos);
}

static void llm_market_slice_smoke(){
 std::string log="llm_market_slice_test.jsonl";
 std::filesystem::remove(log);
 auto make_transport=[]{
  return std::make_shared<RawJsonTransport>(std::vector<std::string>{
   R"({"action":"BUY","asset":"ASSET","quantity":1,"price":95,"time":0})"
  });
 };
 // Deterministic crossing: seller 90, LLM buy 95.
 auto exp=base_exp();
 auto run=base_run(make_transport());
 run.log_path=log;
 auto result=run_llm_market_experiment(exp, run);

 assert(result.turns.size()==1);
 const auto& t=result.turns[0];
 assert(!t.raw_provider_response.empty());
 assert(t.parse.success);
 assert(t.action_validation.valid);
 assert(t.submission.accepted);
 assert(t.submission.filled_quantity==1);
 assert(t.submission.resting_quantity==0);
 assert(t.submission.average_fill_price && std::fabs(*t.submission.average_fill_price-92.5)<1e-12);
 assert(result.trades.size()==1);
 assert(result.units_filled==1);
 assert(result.ending_inventory==1);
 assert(result.ending_cash < result.starting_cash);
 assert(t.agent_state_after.inventory==1);
 assert(t.agent_state_before.inventory==0);
 assert(t.agent_state_before.role==AgentRole::Buyer);
 assert(result.evidence.records().size()==1);
 assert(result.evidence.records()[0].evidence.rationale.find("Traceability-only")!=std::string::npos);
 assert(result.evidence.records()[0].evidence.rationale.find("MarketMechanism")!=std::string::npos);
 assert(std::filesystem::file_size(log)>0);
 // Step 9 evidence browser provenance: audit-only seed/config must be retained in the researcher log.
 { std::ifstream lf(log); std::string line; std::getline(lf,line);
   assert(line.find("\"seed\":")!=std::string::npos);
   assert(line.find("\"inference_config\":\"transport=RawJsonTransport\"")!=std::string::npos);
   assert(line.find("\"canonical_request\":")!=std::string::npos);
 }
 // Repeat with a fresh transport (RawJsonTransport is single-use / exhausted after one invoke).
 auto run2=base_run(make_transport());
 run2.log_path=log;
 auto again=run_llm_market_experiment(exp, run2);
 assert(again.trades.size()==1);
 assert(again.turns[0].submission.filled_quantity==1);
}

int main(){
 parsing_valid_action();
 parsing_invalid_json();
 parsing_rejects_fractional_time();
 validation_wrong_asset();
 validation_wrong_time();
 insufficient_cash();
 insufficient_inventory();
 accepted_but_not_filled();
 guaranteed_trade();
 private_information_boundary();
 deterministic_replay();
 sealed_mechanism_llm_slice();
 llm_market_slice_smoke();
 std::cout<<"llm_market_slice_smoke ok (A-K)\n";
}
