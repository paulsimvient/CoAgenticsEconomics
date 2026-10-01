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
  }
 }
 require(saw_llm, "llm outcome missing");
 require(!run.classification.label.empty(), "operational classifier label");
 require(!run.human_reference.empty(), "human reference comparison present");
 require(run.claim_boundary.find("No Phase I classifier performance")!=std::string::npos, "classifier claim boundary");
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

int main(){
 try{
  test_ten_llm_catalog_and_interface();
  test_population_llm_plus_programmed();
  test_operational_classifier_no_perf_metrics();
  test_heterogeneous_programmed_population();
  {
   auto ic=run_information_contrast_experiment(7);
   require(ic.contrast.control.classifier_synthetic, "control synthetic wiring ok");
   require(!ic.contrast.treatment.classifier_synthetic, "treatment uses control/treatment path");
   require(!ic.contrast.classifier.classification.label.empty(), "contrast label");
   require(ic.claim_boundary.find("information contrast")!=std::string::npos, "info claim");
  }
  std::cout<<"dv026_wave3_smoke ok\n";
  return 0;
 }catch(const std::exception& e){
  std::cerr<<"dv026_wave3_smoke FAIL: "<<e.what()<<"\n";
  return 1;
 }
}
