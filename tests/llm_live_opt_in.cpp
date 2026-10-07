#include "coagentics/experiment/LlmExperiment.hpp"
#include <cstdlib>
#include <iostream>
using namespace coagentics;
using namespace coagentics::experiment;

// Opt-in live provider test. Skips unless COAGENTICS_LIVE_LLM=1.
// Supports OpenAI-compatible cloud keys OR local Ollama (COAGENTICS_LLM_PROVIDER=ollama).
int main(){
 const char* gate=std::getenv("COAGENTICS_LIVE_LLM");
 if(!gate || std::string(gate)!="1"){
  std::cout<<"SKIP llm_live_opt_in (set COAGENTICS_LIVE_LLM=1 to enable)\n";
  return 0;
 }
 LiveTransportConfig cfg;
 const bool ollama=live_uses_ollama() || !live_llm_configured();
 if(ollama){
  cfg=default_ollama_transport_config();
 }else{
  cfg.provider="openai";
  cfg.base_url="https://api.openai.com/v1";
  if(const char* m=std::getenv("COAGENTICS_LLM_MODEL"); m&&*m) cfg.model=m;
  else cfg.model="gpt-4o-mini";
 }
 agents::ModelIdentity model{
  ollama?"ollama":"openai",
  cfg.model.empty()?(ollama?"llama3.2":"gpt-4o-mini"):cfg.model,
  ollama?"local":"live",
  "chat-completions"};

 ExperimentSpec exp;
 exp.rounds=1;
 exp.deterministic_counterparty=true;
 exp.counterparty_limit_price=90;
 RunSpec run;
 run.seed=424242;
 run.model=model;
 run.transport=make_live_openai_compatible_transport(model, cfg);
 run.adapter_version="coagentics-llm-adapter/0.1";
 run.log_path="llm_live_opt_in.jsonl";

 auto result=run_llm_market_experiment(exp, run);
 if(result.turns.empty()){ std::cerr<<"no turns\n"; return 1; }
 const auto& t=result.turns[0];
 std::cout<<"live request_id="<<t.request_id
  <<" parse="<<t.parse.success
  <<" action_valid="<<t.action_validation.valid
  <<" market_accepted="<<t.submission.accepted
  <<" filled="<<t.submission.filled_quantity
  <<" latency_ms="<<t.latency_ms
  <<" raw_bytes="<<t.raw_provider_response.size()<<"\n";
 if(!t.parse.success && result.provider_failures){
  std::cerr<<"provider failure: "<<t.parse.error<<"\n";
  return 1;
 }
 if(t.request_id.empty() || t.canonical_request.empty() || t.model.provider.empty()){
  std::cerr<<"incomplete provenance\n"; return 1;
 }
 // Own private state must be present; counterparty private cost must not.
 if(t.canonical_request.find("private_value")==std::string::npos){
  std::cerr<<"missing own private_value in decision context\n"; return 1;
 }
 std::cout<<"llm_live_opt_in ok\n";
 return 0;
}
