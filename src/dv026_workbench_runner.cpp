#include "coagentics/experiment/Dv026MarketLayers.hpp"
#include "coagentics/experiment/Dv026LiveCampaign.hpp"
#include "coagentics/experiment/HeterogeneousCampaign.hpp"
#include "coagentics/experiment/Dv026PhaseI.hpp"
#include "coagentics/experiment/Dv026PhaseII.hpp"
#include "coagentics/experiment/Dv026Wave3.hpp"
#include "coagentics/experiment/Dv026Wave4.hpp"
#include "coagentics/experiment/Scientist.hpp"
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
using namespace coagentics;
using namespace coagentics::experiment;

namespace {
std::string esc(const std::string& s){
 std::string o; o.reserve(s.size());
 for(char c:s){
  if(c=='\\'||c=='"') o.push_back('\\');
  if(c=='\n'){ o+="\\n"; continue; }
  o.push_back(c);
 }
 return o;
}

std::string wrap(const std::string& command, std::uint64_t seed, const std::string& body,
 bool live_llm=false, bool darpa_claim_ready=false, const std::string& scope=""){
 std::ostringstream o;
 o<<"{\"command\":\""<<esc(command)<<"\",\"seed\":"<<seed
  <<",\"live_llm\":"<<(live_llm?"true":"false")
  <<",\"darpa_claim_ready\":"<<(darpa_claim_ready?"true":"false")
  <<",\"scope\":\""<<esc(scope)<<"\""
  <<",\"constructs_validated\":false"
  <<",\"claim_boundary\":\""<<(live_llm
   ?(darpa_claim_ready
     ?"Local Ollama PoC readiness gate passed (scope=local_ollama_poc). No Phase II constructs; no commercial multi-provider claim."
     :"Live Ollama market path: observable outcomes under local models. DARPA readiness requires full campaign gate.")
   :"DV026 workbench software evidence. Scripted/mock transports only. Does not claim live multi-provider economics, Phase I classifier accuracy, or Phase II construct validation.")
  <<"\""
  <<","<<body.substr(1); // body is {...}; drop leading { and merge
 return o.str();
}

std::string layer_a_json(std::uint64_t seed){
 MarketSpec spec; spec.buyers=4; spec.sellers=4; spec.periods=5; spec.efficiency_gate=90;
 auto r=run_market_qualification(spec, seed, 5);
 return market_qualification_json(r);
}

std::string ten_llm_rich(std::uint64_t seed){
 auto catalog=dv026_distinct_llm_catalog();
 auto r=run_ten_llm_interface_qualification(catalog, seed);
 std::ostringstream o;
 o<<"{\"distinct_models\":"<<r.distinct_models
  <<",\"cells\":"<<r.cells.size()
  <<",\"qualifies\":"<<(r.qualifies?"true":"false")
  <<",\"claim_boundary\":\""<<esc(r.claim_boundary)<<"\"}";
 return o.str();
}

std::string llm_slice_json(std::uint64_t seed){
 ExperimentSpec exp; exp.deterministic_counterparty=true; exp.counterparty_limit_price=90;
 auto transport=std::make_shared<RawJsonTransport>(std::vector<std::string>{
  R"({"action":"BUY","asset":"ASSET","quantity":1,"price":95,"time":0})"
 });
 RunSpec run; run.seed=seed; run.model={"mock","workbench","v1"};
 run.transport=transport; run.run_id="workbench-llm-slice";
 auto original=run_llm_market_experiment(exp, run);
 auto log=capture_historical_replay_log(original, exp, run);
 auto replay=replay_and_validate(log);
 bool boundary=!original.turns.empty()
  && original.turns[0].canonical_request.find("private_value")!=std::string::npos
  && original.turns[0].canonical_request.find(run.seller_agent_id)==std::string::npos;
 bool turn_ok=!original.turns.empty() && original.turns[0].parse.success
  && original.turns[0].action_validation.valid && original.turns[0].submission.accepted;
 std::ostringstream o;
 o<<std::boolalpha
  <<"{\"turns\":"<<original.turns.size()
  <<",\"units_filled\":"<<original.units_filled
  <<",\"parse_validate_submit\":"<<turn_ok
  <<",\"private_state_boundary\":"<<boundary
  <<",\"cross_event_valid\":"<<replay.cross_event_valid
  <<",\"outcome_match\":"<<replay.outcome_match
  <<",\"byte_identical_raw\":"<<replay.byte_identical_raw<<"}";
 return o.str();
}

std::string population_rich(std::uint64_t seed){
 PopulationSpec pop; pop.seed=seed; pop.rounds=1;
 pop.market.asset="ASSET"; pop.market.fundamental=100;
 AgentSlot buyer_llm;
 buyer_llm.agent_id="LLM-B0"; buyer_llm.kind=AgentKind::Llm; buyer_llm.side=market::Side::Buy;
 buyer_llm.private_value_or_cost=120; buyer_llm.cash=10000;
 buyer_llm.model={"mock","workbench-pop","v1"};
 buyer_llm.transport=std::make_shared<RawJsonTransport>(std::vector<std::string>{
  R"({"action":"BUY","asset":"ASSET","quantity":1,"price":95,"time":0})"
 });
 AgentSlot seller;
 seller.agent_id="S0"; seller.kind=AgentKind::ProgrammedHeuristic; seller.side=market::Side::Sell;
 seller.private_value_or_cost=80; seller.inventory=1; seller.cash=0;
 pop.agents={buyer_llm, seller};
 auto run=run_population_market(pop);
 return population_run_json(run);
}

std::string adaptive_json(std::uint64_t seed){
 std::vector<Candidate> h={
  {"blind",0,0,0,.25},{"news",1,0,0,.25},{"peer",0,.6,0,.25},{"mixed",1,.6,0,.25}
 };
 std::vector<Probe> p={{"news-only",10,0,1},{"peer-only",0,20,1},{"both",10,20,1}};
 ScientistConfig cfg; cfg.noise_sd=.25; cfg.stop_posterior=.95;
 ConfirmationConfig conf; conf.heldout_seeds=2; conf.min_posterior=.80;
 AgentProbeDomain domain("signal-responsive");
 auto r=run_adaptive_discovery_confirmation(domain, h, p, seed, cfg, conf);
 return adaptive_discovery_json(r);
}

std::string ollama_preflight_json(){
 auto r=preflight_ollama_live_catalog();
 std::ostringstream o;
 o<<std::boolalpha<<"{\"ollama_reachable\":"<<r.ollama_reachable
  <<",\"catalog_size\":"<<r.catalog.size()
  <<",\"installed_count\":"<<r.installed.size()
  <<",\"available_models\":[";
 for(std::size_t i=0;i<r.available.size();++i){
  if(i) o<<",";
  const auto& m=r.available[i];
  o<<"{\"provider\":\""<<esc(m.provider)<<"\",\"model\":\""<<esc(m.model)
   <<"\",\"version\":\""<<esc(m.version)<<"\",\"transport\":\""<<esc(m.endpoint_class)<<"\"}";
 }
 o<<"],\"missing_models\":[";
 for(std::size_t i=0;i<r.missing.size();++i){
  if(i) o<<",";
  const auto& m=r.missing[i];
  o<<"{\"provider\":\""<<esc(m.provider)<<"\",\"model\":\""<<esc(m.model)
   <<"\",\"version\":\""<<esc(m.version)<<"\",\"transport\":\""<<esc(m.endpoint_class)<<"\"}";
 }
 o<<"]}";
 return o.str();
}

std::string ollama_slice_json(std::uint64_t seed){
 // Ensure Ollama defaults for this command even if the parent only set LIVE_LLM.
 if(std::getenv("COAGENTICS_LLM_PROVIDER")==nullptr)
  setenv("COAGENTICS_LLM_PROVIDER","ollama",1);
 if(std::getenv("COAGENTICS_LLM_BASE_URL")==nullptr)
  setenv("COAGENTICS_LLM_BASE_URL","http://127.0.0.1:11434/v1",1);
 if(!live_llm_configured()){
  throw std::runtime_error(
   "Ollama not configured. Ensure ollama serve is running on :11434 "
   "(optional COAGENTICS_LLM_MODEL, default llama3.2)");
 }
 std::string model_name= []{
  const char* m=std::getenv("COAGENTICS_LLM_MODEL");
  return (m&&*m)? std::string(m) : std::string("llama3.2");
 }();
 agents::ModelIdentity model{"ollama", model_name, "local", "chat-completions"};
 ExperimentSpec exp; exp.deterministic_counterparty=true; exp.counterparty_limit_price=90;
 RunSpec run; run.seed=seed; run.model=model;
 run.transport=make_live_openai_compatible_transport(model);
 run.run_id="ollama-slice:"+std::to_string(seed);
 run.log_path="ollama_slice.jsonl";
 auto original=run_llm_market_experiment(exp, run);
 auto log=capture_historical_replay_log(original, exp, run);
 auto replay=replay_and_validate(log);
 bool turn_ok=!original.turns.empty() && original.turns[0].parse.success
  && original.turns[0].action_validation.valid;
 std::ostringstream o;
 o<<std::boolalpha
  <<"{\"provider\":\"ollama\",\"model\":\""<<esc(model_name)<<"\""
  <<",\"turns\":"<<original.turns.size()
  <<",\"units_filled\":"<<original.units_filled
  <<",\"parse_ok\":"<<(original.turns.empty()?false:original.turns[0].parse.success)
  <<",\"action_valid\":"<<(original.turns.empty()?false:original.turns[0].action_validation.valid)
  <<",\"market_accepted\":"<<(original.turns.empty()?false:original.turns[0].submission.accepted)
  <<",\"turn_ok\":"<<turn_ok
  <<",\"cross_event_valid\":"<<replay.cross_event_valid
  <<",\"latency_ms\":"<<(original.turns.empty()?0:original.turns[0].latency_ms)
  <<",\"efficiency\":"<<original.metrics.allocative_efficiency
  <<",\"raw_bytes\":"<<(original.turns.empty()?0:original.turns[0].raw_provider_response.size())
  <<"}";
 return o.str();
}

std::string ollama_paired_json(std::uint64_t seed){
 if(std::getenv("COAGENTICS_LLM_PROVIDER")==nullptr)
  setenv("COAGENTICS_LLM_PROVIDER","ollama",1);
 if(std::getenv("COAGENTICS_LLM_BASE_URL")==nullptr)
  setenv("COAGENTICS_LLM_BASE_URL","http://127.0.0.1:11434/v1",1);
 if(!live_llm_configured()){
  throw std::runtime_error("Ollama not configured for Layer B paired run");
 }
 std::string model_name= []{
  const char* m=std::getenv("COAGENTICS_LLM_MODEL");
  return (m&&*m)? std::string(m) : std::string("llama3.2");
 }();
 agents::ModelIdentity model{"ollama", model_name, "local", "chat-completions"};
 ExperimentSpec exp; exp.deterministic_counterparty=true; exp.counterparty_limit_price=90;
 RunSpec run; run.seed=seed; run.model=model;
 run.transport=make_live_openai_compatible_transport(model);
 run.run_id="ollama-paired:"+std::to_string(seed);
 auto paired=run_paired_programmed_buyer_vs_llm(exp, run, /*control_buyer_limit_price=*/95.0);
 return paired_llm_json(paired);
}

std::string ollama_campaign_json(std::uint64_t seed, std::size_t n_seeds, bool smoke){
 LiveCampaignSpec spec;
 spec.base_seed=seed;
 spec.n_seeds=n_seeds;
 spec.smoke=smoke;
 // Keep min_models=10 for the readiness gate; smoke never flips darpa_claim_ready.
 auto r=run_ollama_layer_b_campaign(spec);
 return live_campaign_json(r);
}

std::string run_command(const std::string& cmd, std::uint64_t seed, std::size_t n_seeds, bool smoke){
 if(cmd=="phase-i"){
  auto r=run_phase_i_qualification_suite(seed);
  return wrap(cmd, seed, phase_i_suite_json(r), false, r.darpa_claim_ready,
   r.darpa_claim_ready?"local_ollama_poc":"");
 }
 if(cmd=="phase-ii"){
  auto r=run_phase_ii_measurement_suite(seed);
  return wrap(cmd, seed, phase_ii_suite_json(r));
 }
 if(cmd=="layer-a"){
  return wrap(cmd, seed, "{\"layer_a\":"+layer_a_json(seed)+"}");
 }
 if(cmd=="ten-llm"){
  return wrap(cmd, seed, "{\"ten_llm\":"+ten_llm_rich(seed)+"}");
 }
 if(cmd=="llm-slice"){
  return wrap(cmd, seed, "{\"llm_slice\":"+llm_slice_json(seed)+"}");
 }
 if(cmd=="population"){
  return wrap(cmd, seed, "{\"population\":"+population_rich(seed)+"}");
 }
 if(cmd=="adaptive"){
  return wrap(cmd, seed, "{\"adaptive\":"+adaptive_json(seed)+"}");
 }
 if(cmd=="ollama-preflight"){
  return wrap(cmd, seed, "{\"preflight\":"+ollama_preflight_json()+"}", true);
 }
 if(cmd=="ollama-slice"){
  return wrap(cmd, seed, "{\"ollama_slice\":"+ollama_slice_json(seed)+"}", true);
 }
 if(cmd=="ollama-paired"){
  return wrap(cmd, seed, "{\"paired\":"+ollama_paired_json(seed)+"}", true);
 }
 if(cmd=="ollama-campaign"){
  auto body=ollama_campaign_json(seed, n_seeds, smoke);
  // re-parse readiness from body for wrap flags
  bool ready=body.find("\"darpa_claim_ready\":true")!=std::string::npos;
  std::string scope;
  auto sk=body.find("\"scope\":\"");
  if(sk!=std::string::npos){
   auto q1=sk+9; auto q2=body.find('"', q1);
   if(q2!=std::string::npos) scope=body.substr(q1, q2-q1);
  }
  return wrap(cmd, seed, "{\"campaign\":"+body+"}", true, ready, scope);
 }
 if(cmd=="hetero-campaign"){
  HeterogeneousCampaignSpec hs; hs.base_seed=seed; hs.n_seeds=n_seeds; hs.smoke=smoke; hs.use_live_ollama=!smoke; hs.inference_repeats=2; if(const char* rr=std::getenv("COAGENTICS_INFERENCE_REPEATS")){try{auto v=std::stoull(rr); if(v>=1 && v<=20) hs.inference_repeats=v;}catch(...){}} if(const char* ad=std::getenv("COAGENTICS_ACTIVATION_DESIGN")){std::string a=ad; if(a=="frozen_snapshot"){hs.run_frozen_snapshot=true;hs.run_sequential_interaction=false;} else if(a=="sequential_interaction"){hs.run_frozen_snapshot=false;hs.run_sequential_interaction=true;} else if(a=="both"){hs.run_frozen_snapshot=true;hs.run_sequential_interaction=true;}} if(smoke){hs.use_live_ollama=false; hs.n_seeds=std::min<std::size_t>(hs.n_seeds,2); hs.rounds=1; hs.max_llm_seats=2;}
  auto r=run_heterogeneous_campaign(hs);
  return wrap(cmd, seed, "{\"heterogeneous_campaign\":"+heterogeneous_campaign_json(r)+"}", hs.use_live_ollama);
 }
 if(cmd=="hetero-pop"){
  HeterogeneousPopulationSpec hs; hs.seed=seed; hs.smoke=smoke; hs.use_live_ollama=!smoke; const char* ad=std::getenv("COAGENTICS_ACTIVATION_DESIGN"); if(ad&&std::string(ad)=="frozen_snapshot") hs.activation_design=ActivationDesign::FrozenSnapshot;
  if(smoke) hs.use_live_ollama=false;
  auto r=run_heterogeneous_ollama_population(hs);
  return wrap(cmd, seed, "{\"hetero\":"+heterogeneous_population_json(r)+"}", hs.use_live_ollama);
 }
 if(cmd=="info-contrast" || cmd=="info-contrast-live"){
  auto rs=(cmd=="info-contrast-live")?run_information_treatment_suite_live(seed):run_information_treatment_suite(seed);
  std::ostringstream body; body<<"{\"information_treatments\":[";
  for(size_t i=0;i<rs.size();++i){ auto& r=rs[i]; if(i)body<<",";
   body<<"{\"matched_seed\":"<<r.matched_seed
    <<",\"manipulated_variable\":\""<<esc(r.manipulated_variable)<<"\""
    <<",\"control_condition\":\""<<esc(r.control_condition)<<"\""
    <<",\"treatment_condition\":\""<<esc(r.treatment_condition)<<"\""
    <<",\"classifier\":\""<<esc(r.contrast.classifier.classification.label)<<"\""
    <<",\"classifier_mode\":\""<<esc(r.contrast.treatment.classifier_mode)<<"\""
    <<",\"classifier_available\":"<<(r.contrast.treatment.classifier_available?"true":"false")
    <<",\"classifier_synthetic_treatment\":"<<(r.contrast.treatment.classifier_synthetic?"true":"false")<<"}";
  }
  body<<"],\"claim_boundary\":\"matched-seed, one-variable-at-a-time information treatments; scripted fixture qualifies plumbing, live transports provide empirical behavior\"}";
  return wrap(cmd, seed, body.str());
 }
 if(cmd=="bundle"){
  auto pi=run_phase_i_qualification_suite(seed);
  auto pii=run_phase_ii_measurement_suite(seed);
  std::ostringstream body;
  body<<"{"
   <<"\"phase_i\":"<<phase_i_suite_json(pi)
   <<",\"phase_ii\":"<<phase_ii_suite_json(pii)
   <<",\"layer_a\":"<<layer_a_json(seed)
   <<",\"ten_llm\":"<<ten_llm_rich(seed)
   <<",\"llm_slice\":"<<llm_slice_json(seed)
   <<",\"population\":"<<population_rich(seed)
   <<",\"adaptive\":"<<adaptive_json(seed)
   <<"}";
  return wrap(cmd, seed, body.str(), false, pi.darpa_claim_ready,
   pi.darpa_claim_ready?"local_ollama_poc":"");
 }
 throw std::invalid_argument(
  "unknown command (use: bundle|phase-i|phase-ii|layer-a|ten-llm|llm-slice|population|adaptive|"
  "ollama-preflight|ollama-slice|ollama-paired|ollama-campaign|hetero-pop|hetero-campaign|info-contrast|info-contrast-live)");
}
}

int main(int argc, char** argv){
 try{
  if(argc<2){
   std::cerr<<"usage: dv026-workbench-runner <command> [seed] [n_seeds] [--smoke]\n";
   return 2;
  }
  std::string cmd=argv[1];
  std::uint64_t seed=424242;
  std::size_t n_seeds=20;
  bool smoke=false;
  if(argc>=3) seed=std::stoull(argv[2]);
  if(argc>=4){
   std::string a3=argv[3];
   if(a3=="--smoke"||a3=="smoke") smoke=true;
   else n_seeds=std::stoull(a3);
  }
  if(argc>=5){
   std::string a4=argv[4];
   if(a4=="--smoke"||a4=="smoke") smoke=true;
  }
  if(std::getenv("COAGENTICS_OLLAMA_CAMPAIGN_SMOKE")!=nullptr) smoke=true;
  std::cout<<run_command(cmd, seed, n_seeds, smoke)<<"\n";
  return 0;
 }catch(const std::exception& e){
  std::cerr<<"dv026-workbench-runner error: "<<e.what()<<"\n";
  return 1;
 }
}
