#include "coagentics/experiment/Dv026Wave3.hpp"
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
using namespace coagentics;
using namespace coagentics::experiment;

static void require(bool ok, const std::string& msg){
 if(!ok) throw std::runtime_error(msg);
}

static void test_ten_llm_catalog_and_interface(){
 auto catalog=dv026_distinct_llm_catalog();
 require(catalog.size()==10, "catalog must have 10 models");
 std::set<std::string> keys;
 for(const auto& m:catalog) keys.insert(m.provider+"|"+m.model+"|"+m.version);
 require(keys.size()==10, "all catalog identities must be distinct by provider|model|version");

 auto rep=run_ten_llm_interface_qualification(catalog, 424242);
 require(rep.distinct_models==10, "distinct_models");
 require(rep.cells.size()==10, "cells");
 require(rep.qualifies, "ten-LLM interface qualification");
 for(const auto& c:rep.cells){
  require(c.parse_ok && c.action_valid && c.provenance_ok, "cell provenance: "+c.model.model);
  require(!c.request_id.empty(), "request_id");
 }
 require(rep.claim_boundary.find("Does not claim economic performance")!=std::string::npos, "claim boundary");
 // Config-only duplicate must NOT inflate distinct count.
 auto dup=catalog; dup.push_back(catalog.front());
 auto rep2=run_ten_llm_interface_qualification(dup, 7);
 require(rep2.cells.size()==11, "duplicate adds a cell");
 require(rep2.distinct_models==10, "duplicate identity does not create new model");
 require(rep2.qualifies, "still qualifies via 10 distinct successful identities");
}

static void test_population_llm_plus_programmed(){
 PopulationSpec pop;
 pop.seed=99; pop.rounds=1;
 pop.market.asset="ASSET"; pop.market.fundamental=100;

 AgentSlot buyer_llm;
 buyer_llm.agent_id="LLM-B0";
 buyer_llm.kind=AgentKind::Llm;
 buyer_llm.side=market::Side::Buy;
 buyer_llm.private_value_or_cost=120;
 buyer_llm.cash=10000;
 buyer_llm.model={"mock","pop-model","v1"};
 buyer_llm.transport=std::make_shared<RawJsonTransport>(std::vector<std::string>{
  R"({"action":"BUY","asset":"ASSET","quantity":1,"price":95,"time":0})"
 });

 AgentSlot seller;
 seller.agent_id="S0";
 seller.kind=AgentKind::ProgrammedHeuristic;
 seller.side=market::Side::Sell;
 seller.private_value_or_cost=80;
 seller.inventory=1;
 seller.cash=0;

 pop.agents={buyer_llm, seller};
 // Ensure seller asks below 95 so trade can occur: heuristic sell shades up from 80 → ~82.4
 auto run=run_population_market(pop);
 require(run.agents.size()==2, "two agent outcomes");
 require(run.trades.size()>=1, "expected trade in mixed population");
 bool saw_llm=false;
 for(const auto& a:run.agents){
  if(a.kind==AgentKind::Llm){
   saw_llm=true;
   require(a.turns==1, "llm turn");
   require(a.llm_turns.size()==1, "llm turn record");
   require(a.llm_turns[0].canonical_request.find("private_value")!=std::string::npos, "own private state");
   require(a.llm_turns[0].canonical_request.find("S0")==std::string::npos, "no other agent id leak");
   require(a.fingerprint_available, "observed fingerprint available");
   require(a.fingerprint.observed, "fingerprint marked observed");
   require(a.fingerprint.action_validity==1.0, "valid action rate");
   require(a.fingerprint.reservation_value_violation_rate==0.0, "no reservation violation");
   require(a.fingerprint.trade_frequency>0.0, "settlement-aware trade frequency");
   require(a.fingerprint.response_consistency==1.0, "single valid response is consistent");
  }
 }
 require(saw_llm, "llm outcome missing");
 require(!run.classifier_available, "standalone population must not emit classifier");
 require(run.classifier_mode=="none", "standalone classifier mode none");
 require(run.classification.label.empty(), "standalone classifier label empty");
 require(!run.human_reference.empty(), "human reference comparison present");
 auto js=population_run_json(run);
 require(js.find("\"fingerprints\":[{")!=std::string::npos, "fingerprints serialized");
 require(js.find("\"action_validity\":1.0000")!=std::string::npos, "fingerprint validity serialized");
 require(run.claim_boundary.find("No Phase I classifier performance")!=std::string::npos, "classifier claim boundary");

 // RawJsonTransport is a deterministic fixture and must never be mislabeled as live human-comparison evidence.
 PopulationSpec all_llm_fixture=pop;
 AgentSlot seller_llm=seller; seller_llm.agent_id="LLM-S0"; seller_llm.kind=AgentKind::Llm; seller_llm.model={"mock","fixture-seller","v1"};
 seller_llm.transport=std::make_shared<RawJsonTransport>(std::vector<std::string>{R"({"action":"SELL","asset":"ASSET","quantity":1,"price":82,"time":0})"});
 all_llm_fixture.agents={buyer_llm,seller_llm};
 auto fixture=run_population_market(all_llm_fixture);
 require(fixture.human_comparison.behavioral_benchmarks.size()==3, "human behavioral benchmarks emitted");
 for(const auto& b:fixture.human_comparison.behavioral_benchmarks) require(b.status=="SCRIPTED_CONTROL_ONLY" || b.status=="NO_MATCHED_BEHAVIORAL_OBSERVATION", "fixture not live human evidence");
}

static void test_operational_classifier_no_perf_metrics(){
 std::vector<market::Bid> ctl={{0,"T","ASSET",1,100,market::Side::Buy}};
 std::vector<market::Bid> trt={{0,"T","ASSET",1,110,market::Side::Buy}};
 auto rep=run_operational_classifier(ctl, trt, 0, {"T"});
 require(!rep.classification.label.empty(), "label");
 require(rep.claim_boundary.find("No Phase I classifier accuracy")!=std::string::npos, "faq31 boundary");
}

static void test_heterogeneous_programmed_population(){
 PopulationSpec pop; pop.seed=3; pop.rounds=8;
 pop.market.fundamental=100; pop.market.asset="ASSET";
 for(int i=0;i<3;++i){
  AgentSlot b; b.agent_id="B"+std::to_string(i); b.kind=AgentKind::ProgrammedHeuristic;
  b.side=market::Side::Buy; b.private_value_or_cost=130-10*i; b.cash=5000;
  pop.agents.push_back(b);
 }
 for(int i=0;i<3;++i){
  AgentSlot s; s.agent_id="S"+std::to_string(i); s.kind=AgentKind::ProgrammedZI;
  s.side=market::Side::Sell; s.private_value_or_cost=70+10*i; s.inventory=1; s.cash=0;
  pop.agents.push_back(s);
 }
 auto run=run_population_market(pop);
 require(run.agents.size()==6, "six agents");
 require(run.metrics.allocative_efficiency>=0.0, "efficiency computed");
}

static PopulationSpec two_llm_buyers(ActivationDesign design){
 PopulationSpec pop; pop.seed=44; pop.rounds=1; pop.activation_design=design;
 pop.market.asset="ASSET"; pop.market.fundamental=100;
 for(int i=0;i<2;++i){
  AgentSlot b; b.agent_id="L"+std::to_string(i); b.kind=AgentKind::Llm; b.side=market::Side::Buy;
  b.private_value_or_cost=120+i; b.cash=5000; b.model={"mock","m"+std::to_string(i),"v1"};
  b.transport=std::make_shared<RawJsonTransport>(std::vector<std::string>{
   std::string("{\"action\":\"BUY\",\"asset\":\"ASSET\",\"quantity\":1,\"price\":")+(i?"106":"105")+",\"time\":0}"});
  pop.agents.push_back(b);
 }
 return pop;
}

static void test_dual_activation_designs(){
 auto frozen=run_population_market(two_llm_buyers(ActivationDesign::FrozenSnapshot));
 require(frozen.agents.size()==2, "frozen two agents");
 require(frozen.agents[0].llm_turns.size()==1 && frozen.agents[1].llm_turns.size()==1, "frozen turns");
 const auto& f0=frozen.agents[0].llm_turns[0].market_state;
 const auto& f1=frozen.agents[1].llm_turns[0].market_state;
 require(f0.best_bid==f1.best_bid && f0.best_ask==f1.best_ask && f0.last_trade==f1.last_trade,
  "frozen snapshot gives identical public market state");

 auto seq=run_population_market(two_llm_buyers(ActivationDesign::SequentialInteraction));
 require(seq.agents.size()==2, "sequential two agents");
 const auto& s0=seq.agents[0].llm_turns[0].market_state;
 const auto& s1=seq.agents[1].llm_turns[0].market_state;
 require(s0.best_bid!=s1.best_bid, "sequential design exposes earlier same-round market update");
}

int main(){
 try{
  test_ten_llm_catalog_and_interface();
  test_population_llm_plus_programmed();
  test_operational_classifier_no_perf_metrics();
  test_heterogeneous_programmed_population();
  test_dual_activation_designs();
  {
   auto suite=run_information_treatment_suite(7);
   require(suite.size()==3, "three independent information treatments");
   for(const auto& ic:suite){
    require(ic.matched_seed==7, "matched seed retained");
    require(ic.contrast.control.spec.seed==ic.contrast.treatment.spec.seed, "paired arms share seed");
    require(!ic.contrast.control.classifier_available, "control arm alone has no classifier");
    require(!ic.contrast.control.classifier_synthetic, "no synthetic classifier on control arm");
    require(ic.contrast.treatment.classifier_available, "paired treatment carries empirical classifier result");
    require(ic.contrast.treatment.classifier_mode=="observed_control_treatment", "empirical classifier mode");
    require(!ic.contrast.treatment.classifier_synthetic, "empirical contrast is not synthetic wiring");
    require(!ic.contrast.classifier.classification.label.empty(), "contrast label");
    require(!ic.manipulated_variable.empty(), "manipulated variable recorded");
    bool fp=false; for(const auto& a:ic.contrast.treatment.agents) if(a.agent_id=="LLM-B0"){
     require(a.fingerprint_available && a.fingerprint.observed, "treatment fingerprint observed");
     fp=true;
    }
    require(fp, "target fingerprint present");
   }
   auto news=suite[0];
   require(!news.contrast.control.spec.agents[0].information.news.present, "control news hidden");
   require(news.contrast.treatment.spec.agents[0].information.news.present, "treatment news visible");
   auto hist=suite[1];
   require(!hist.contrast.control.spec.agents[0].information.history_visible, "control history hidden");
   require(hist.contrast.treatment.spec.agents[0].information.history_visible, "treatment history visible");
   require(hist.contrast.control.agents[0].llm_turns.size()==2, "history contrast has second decision");
   require(hist.contrast.control.agents[0].llm_turns[1].canonical_request.find("\"history\":[") == std::string::npos, "control payload excludes history");
   require(hist.contrast.treatment.agents[0].llm_turns[1].canonical_request.find("\"history\":[") != std::string::npos, "treatment payload includes history");
   auto peer=suite[2];
   require(!peer.contrast.control.spec.agents[0].information.peer.peer_visibility, "control peer hidden");
   require(peer.contrast.treatment.spec.agents[0].information.peer.peer_visibility, "treatment peer visible");
   require(news.contrast.treatment.agents[0].fingerprint.information_sensitivity>0, "news sensitivity measured from paired behavior");
   require(peer.contrast.treatment.agents[0].fingerprint.peer_sensitivity>0, "peer sensitivity measured from paired behavior");
  }
  std::cout<<"dv026_wave3_smoke ok\n";
  return 0;
 }catch(const std::exception& e){
  std::cerr<<"dv026_wave3_smoke FAIL: "<<e.what()<<"\n";
  return 1;
 }
}
