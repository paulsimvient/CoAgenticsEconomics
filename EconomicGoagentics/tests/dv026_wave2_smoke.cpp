#include "coagentics/experiment/Dv026MarketLayers.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace coagentics;
using namespace coagentics::experiment;

static void require(bool ok, const std::string& msg){
 if(!ok) throw std::runtime_error(msg);
}

static void layer_a_market_qualification(){
 MarketSpec spec;
 spec.buyers=4; spec.sellers=4; spec.periods=25;
 spec.value_step=10; spec.fundamental=100; spec.starting_cash=5000;
 spec.efficiency_gate=90.0;
 auto rep=run_market_qualification(spec, 424242, 5);
 require(rep.trials.size()==10, "expected 10 trials");
 require(rep.mean_efficiency_cda>=90.0, "CDA mean efficiency < 90");
 require(rep.mean_efficiency_sealed>=90.0, "sealed mean efficiency < 90");
 require(rep.cda_qualified && rep.sealed_qualified && rep.layer_a_pass, "layer A gate failed");
 require(rep.evidence.records().size()==1, "missing layer A evidence");
 require(rep.evidence.records()[0].evidence.hypothesis_id=="H_MARKET_ENVIRONMENT_QUALIFIED", "wrong hypothesis");
 require(rep.claim_boundary.find("NOT an LLM")!=std::string::npos, "missing claim boundary");
 auto again=run_market_qualification(spec, 424242, 5);
 require(std::fabs(again.mean_efficiency_cda-rep.mean_efficiency_cda)<1e-12, "CDA not reproducible");
 require(std::fabs(again.mean_efficiency_sealed-rep.mean_efficiency_sealed)<1e-12, "sealed not reproducible");
 std::cout<<"layer_a mean_cda="<<rep.mean_efficiency_cda
  <<" mean_sealed="<<rep.mean_efficiency_sealed<<"\n";
}

static void layer_b_paired_zi_vs_llm(){
 ExperimentSpec econ;
 econ.deterministic_counterparty=true;
 econ.counterparty_limit_price=90;
 econ.counterparty_quantity=1;
 econ.rounds=1;
 econ.llm_private_value=120;
 econ.seller_cost=80;
 econ.starting_cash=10000;

 auto make_treat=[&]{
  auto transport=std::make_shared<RawJsonTransport>(std::vector<std::string>{
   R"({"action":"BUY","asset":"ASSET","quantity":1,"price":95,"time":0})"
  });
  RunSpec treat;
  treat.seed=424242;
  treat.model={"mock","paired","v1"};
  treat.transport=transport;
  treat.adapter_version="coagentics-llm-adapter/0.1";
  return treat;
 };

 auto paired=run_paired_programmed_buyer_vs_llm(econ, make_treat(), /*control_buyer_limit_price=*/95.0);
 require(paired.treatment.turns.size()==1, "missing treatment turn");
 require(paired.treatment.turns[0].parse.success, "parse failed");
 require(paired.treatment.turns[0].submission.filled_quantity==1, "treatment not filled");
 require(paired.control_trades==1, "control should trade at 95");
 require(paired.treatment.trades.size()==1, "treatment should trade");
 require(std::fabs(paired.deltas.delta_efficiency)<1e-6, "matched fixture efficiency delta");
 require(paired.deltas.delta_trades==0, "matched fixture trade delta");
 require(paired.evidence.records()[0].evidence.hypothesis_id=="H_PAIRED_OBSERVABLE_DELTA", "wrong paired hypothesis");
 require(paired.claim_boundary.find("Does not claim LLM economic rationality")!=std::string::npos, "missing paired boundary");

 auto paired2=run_paired_programmed_buyer_vs_llm(econ, make_treat(), /*control_buyer_limit_price=*/85.0);
 require(paired2.control_trades==0, "control @85 must not cross seller @90");
 require(paired2.treatment.trades.size()==1, "treatment must still trade");
 require(paired2.deltas.delta_trades==1, "expected delta_trades==1");
 std::cout<<"layer_b delta_trades_cross="<<paired2.deltas.delta_trades<<"\n";
}

static void layers_remain_separate(){
 auto a=run_market_qualification({}, 1, 1);
 require(a.evidence.records()[0].evidence.rationale.find("Not an LLM claim")!=std::string::npos,
  "layer A must not claim LLM performance");
}

int main(){
 try{
  layer_a_market_qualification();
  layer_b_paired_zi_vs_llm();
  layers_remain_separate();
  std::cout<<"dv026_wave2_smoke ok\n";
  return 0;
 }catch(const std::exception& e){
  std::cerr<<"dv026_wave2_smoke FAIL: "<<e.what()<<"\n";
  return 1;
 }
}
