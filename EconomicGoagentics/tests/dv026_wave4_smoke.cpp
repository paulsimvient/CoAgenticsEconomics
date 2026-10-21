#include "coagentics/experiment/Dv026Wave4.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
using namespace coagentics;
using namespace coagentics::experiment;

static void require(bool ok, const std::string& msg){
 if(!ok) throw std::runtime_error(msg);
}

static void test_adaptive_discovery_confirmation(){
 std::vector<Candidate> h={
  {"blind",0,0,0,.25},{"news",1,0,0,.25},{"peer",0,.6,0,.25},{"mixed",1,.6,0,.25}
 };
 std::vector<Probe> p={{"news-only",10,0,1},{"peer-only",0,20,1},{"both",10,20,1}};
 ScientistConfig cfg; cfg.noise_sd=.25; cfg.stop_posterior=.95;
 ConfirmationConfig conf; conf.heldout_seeds=3; conf.min_posterior=.80;

 AgentProbeDomain domain("signal-responsive");
 auto rep=run_adaptive_discovery_confirmation(domain, h, p, 26026, cfg, conf);
 require(rep.discovery_separated, "discovery should separate signal-responsive");
 require(rep.leading_hypothesis=="news", "leading should be news");
 require(rep.confirmed, "held-out confirmation should agree");
 require(rep.confirmation_runs.size()==3, "three confirmation runs");
 require(rep.confirmation_agree==3, "all confirmations agree");
 require(rep.evidence.records().size()>=2, "discovery+confirmation evidence");
 require(rep.claim_boundary.find("Does not claim Phase I")!=std::string::npos, "claim boundary");

 // Null: identical candidates → unresolved discovery → confirmation skipped
 std::vector<Candidate> identical={{"a",0,0,0,.5},{"b",0,0,0,.5}};
 AgentProbeDomain blind("information-blind");
 auto unresolved=run_adaptive_discovery_confirmation(blind, identical, p, 1, cfg, conf);
 require(!unresolved.discovery_separated, "null unresolved");
 require(!unresolved.confirmed, "null not confirmed");
 require(unresolved.status.find("CONFIRMATION_SKIPPED")!=std::string::npos, "skip confirmation");
}

static void test_historical_replay_cross_event(){
 ExperimentSpec exp;
 exp.rounds=2;
 exp.deterministic_counterparty=true;
 exp.counterparty_limit_price=90.0;

 auto payloads=std::vector<std::string>{
  R"({"action":"BUY","asset":"ASSET","quantity":1,"price":95,"time":0})",
  R"({"action":"HOLD","asset":"ASSET","quantity":0,"time":1})"
 };
 auto transport=std::make_shared<RawJsonTransport>(payloads);
 RunSpec run;
 run.seed=4242;
 run.model={"mock","wave4-replay","v1"};
 run.transport=transport;
 run.run_id="capture-run";

 auto original=run_llm_market_experiment(exp, run);
 require(original.turns.size()==2, "two turns captured");

 auto log=capture_historical_replay_log(original, exp, run);
 require(log.events.size()==2, "two events in log");
 require(log.events[0].raw_provider_response.find("BUY")!=std::string::npos, "buy payload");
 require(log.events[1].raw_provider_response.find("HOLD")!=std::string::npos, "hold payload");

 auto replay=replay_and_validate(log);
 require(replay.byte_identical_raw, "raw payloads identical");
 require(replay.cross_event_valid, "cross-event validation");
 require(replay.outcome_match, "outcome match");
 require(replay.events.size()==2, "two validations");
 require(replay.events[0].fill_match && replay.events[1].fill_match, "fills match");
 require(replay.claim_boundary.find("Cross-event validation")!=std::string::npos, "replay claim");
 require(!replay.evidence.records().empty(), "replay evidence");

 // Tamper: wrong payload must fail validation
 auto bad=log;
 bad.events[0].raw_provider_response=
  R"({"action":"BUY","asset":"ASSET","quantity":1,"price":80,"time":0})";
 // Keep original_filled_quantity from capture so fill/parse mismatch surfaces
 auto fail=replay_and_validate(bad);
 require(!fail.cross_event_valid, "tampered transcript must fail cross-event");
}

int main(){
 try{
  test_adaptive_discovery_confirmation();
  test_historical_replay_cross_event();
  std::cout<<"dv026_wave4_smoke ok\n";
  return 0;
 }catch(const std::exception& e){
  std::cerr<<"dv026_wave4_smoke FAIL: "<<e.what()<<"\n";
  return 1;
 }
}
