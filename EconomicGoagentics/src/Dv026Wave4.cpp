#include "coagentics/experiment/Dv026Wave4.hpp"
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
namespace coagentics::experiment {
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
}

AdaptiveDiscoveryConfirmationReport run_adaptive_discovery_confirmation(
 const ExperimentalDomain& domain,
 const std::vector<Candidate>& candidates,
 const std::vector<Probe>& probes,
 std::uint64_t discovery_seed,
 const ScientistConfig& discovery_cfg,
 const ConfirmationConfig& confirm_cfg){
 if(confirm_cfg.heldout_seeds==0) throw std::invalid_argument("heldout_seeds must be >0");

 AdaptiveDiscoveryConfirmationReport out;
 out.discovery=run_scientist(domain, candidates, probes, discovery_seed, discovery_cfg);
 out.discovery_separated=out.discovery.status.find("SEPARATED")!=std::string::npos;
 out.leading_hypothesis=out.discovery.leading_hypothesis;

 analysis::EvidenceEnvelope disc;
 disc.experiment_id="dv026-wave4-discovery";
 disc.seed=discovery_seed;
 disc.evidence.hypothesis_id="H_ADAPTIVE_DISCOVERY";
 disc.evidence.discriminator_id="D_PROBE_SELECTION";
 disc.evidence.direction=out.discovery_separated
  ?analysis::EvidenceDirection::Supports:analysis::EvidenceDirection::Challenges;
 disc.evidence.rationale=
  "status="+out.discovery.status+
  " leading="+out.leading_hypothesis+
  " steps="+std::to_string(out.discovery.steps.size())+
  ". Model-conditional only.";
 out.evidence.append(std::move(disc));

 if(!out.discovery_separated || out.leading_hypothesis.empty()){
  out.status="CONFIRMATION_SKIPPED: discovery unresolved";
  out.confirmed=false;
  return out;
 }

 // Confirmation: re-run scientist on held-out seeds; require same leading hypothesis
 // with posterior at/above confirmation threshold on the final step.
 for(std::size_t i=0;i<confirm_cfg.heldout_seeds;++i){
  const std::uint64_t seed=discovery_seed+1000003u*(i+1);
  auto rep=run_scientist(domain, candidates, probes, seed, discovery_cfg);
  bool agree=rep.leading_hypothesis==out.leading_hypothesis;
  double lead_post=0;
  if(!rep.steps.empty()){
   const auto& post=rep.steps.back().posterior;
   std::size_t lead_idx=0;
   for(std::size_t j=0;j<candidates.size();++j){
    if(candidates[j].id==out.leading_hypothesis){ lead_idx=j; break; }
   }
   if(lead_idx<post.size()) lead_post=post[lead_idx];
  }
  if(agree && lead_post>=confirm_cfg.min_posterior) ++out.confirmation_agree;
  out.confirmation_runs.push_back(std::move(rep));
 }

 out.confirmed=(out.confirmation_agree==confirm_cfg.heldout_seeds);
 out.status=out.confirmed
  ?"CONFIRMED: discovery leading hypothesis held out"
  :"UNCONFIRMED: held-out disagreement or weak posterior";

 analysis::EvidenceEnvelope conf;
 conf.experiment_id="dv026-wave4-confirmation";
 conf.seed=discovery_seed;
 conf.evidence.hypothesis_id="H_DISCOVERY_CONFIRMATION";
 conf.evidence.discriminator_id="D_HELDOUT_AGREEMENT";
 conf.evidence.direction=out.confirmed
  ?analysis::EvidenceDirection::Supports:analysis::EvidenceDirection::Challenges;
 conf.evidence.rationale=
  "leading="+out.leading_hypothesis+
  " agree="+std::to_string(out.confirmation_agree)+
  "/"+std::to_string(confirm_cfg.heldout_seeds)+
  ". Confirmation of adaptive selection only; not Phase I/II claim.";
 out.evidence.append(std::move(conf));
 return out;
}

CapturedProviderTranscriptLog capture_provider_transcript_log(const RunResult& original,
 const ExperimentSpec& experiment, const RunSpec& run){
 CapturedProviderTranscriptLog log;
 log.experiment=experiment;
 log.run_template=run;
 log.run_template.transport.reset(); // replay builds its own
 log.run_template.log_path.clear();
 for(const auto& t:original.turns){
  CapturedTurnEvent ev;
  ev.time=t.time;
  ev.agent_id=t.agent_id;
  ev.request_id=t.request_id;
  ev.raw_provider_response=t.raw_provider_response;
  ev.original_parsed_action=t.parsed_action;
  ev.original_filled_quantity=t.submission.filled_quantity;
  ev.original_parse_ok=t.parse.success;
  ev.original_action_valid=t.action_validation.valid;
  log.events.push_back(std::move(ev));
 }
 return log;
}

CapturedProviderReplayReport replay_and_validate(const CapturedProviderTranscriptLog& log){
 if(log.events.empty()) throw std::invalid_argument("CapturedProviderTranscriptLog has no events");
 CapturedProviderReplayReport out; out.log=log;

 std::vector<agents::ModelResponse> responses;
 responses.reserve(log.events.size());
 for(const auto& ev:log.events){
  agents::ModelResponse r;
  r.request_id=ev.request_id;
  r.raw_output=ev.raw_provider_response;
  responses.push_back(std::move(r));
 }
 auto transport=std::make_shared<agents::ReplayTransport>(std::move(responses));
 RunSpec run=log.run_template;
 run.transport=transport;
 if(run.run_id.empty()) run.run_id="replay:"+std::to_string(run.seed);

 out.replayed=run_llm_market_experiment(log.experiment, run);

 const auto& turns=out.replayed.turns;
 out.byte_identical_raw=(turns.size()==log.events.size());
 out.cross_event_valid=true;
 for(std::size_t i=0;i<log.events.size();++i){
  EventValidation v;
  v.time=log.events[i].time;
  v.agent_id=log.events[i].agent_id;
  if(i>=turns.size()){
   v.detail="missing replay turn";
   out.cross_event_valid=false;
   out.byte_identical_raw=false;
   out.events.push_back(v);
   continue;
  }
  const auto& t=turns[i];
  v.raw_match=(t.raw_provider_response==log.events[i].raw_provider_response);
  v.parse_match=(t.parse.success==log.events[i].original_parse_ok);
  if(log.events[i].original_parsed_action && t.parsed_action){
   v.parse_match=v.parse_match &&
    t.parsed_action->action==log.events[i].original_parsed_action->action &&
    t.parsed_action->quantity==log.events[i].original_parsed_action->quantity &&
    t.parsed_action->price==log.events[i].original_parsed_action->price;
  }else if(log.events[i].original_parsed_action || t.parsed_action){
   v.parse_match=false;
  }
  v.fill_match=(t.submission.filled_quantity==log.events[i].original_filled_quantity);
  if(!v.raw_match || !v.parse_match || !v.fill_match){
   out.cross_event_valid=false;
   v.detail="mismatch";
  }
  if(!v.raw_match) out.byte_identical_raw=false;
  out.events.push_back(v);
 }

 // Outcome-level check against captured fills/trades count from replay vs log fills sum
 int expected_fills=0;
 for(const auto& ev:log.events) expected_fills+=ev.original_filled_quantity;
 out.outcome_match=
  out.cross_event_valid &&
  static_cast<int>(out.replayed.units_filled)==expected_fills;

 analysis::EvidenceEnvelope env;
 env.experiment_id="dv026-wave4-captured-provider-replay";
 env.seed=log.run_template.seed;
 env.evidence.hypothesis_id="H_CAPTURED_PROVIDER_TRANSCRIPT_REPLAY";
 env.evidence.discriminator_id="D_CROSS_EVENT_VALIDATION";
 env.evidence.direction=out.cross_event_valid
  ?analysis::EvidenceDirection::Supports:analysis::EvidenceDirection::Challenges;
 env.evidence.rationale=
  "Captured-provider transcript replay (not historical market-data replay). "
  "events="+std::to_string(log.events.size())+
  " cross_event_valid="+std::string(out.cross_event_valid?"true":"false")+
  " outcome_match="+std::string(out.outcome_match?"true":"false");
 out.evidence.append(std::move(env));
 return out;
}

std::string adaptive_discovery_json(const AdaptiveDiscoveryConfirmationReport& r){
 std::ostringstream o;
 o<<std::boolalpha
  <<"{\"discovery_separated\":"<<r.discovery_separated
  <<",\"leading_hypothesis\":\""<<esc(r.leading_hypothesis)<<"\""
  <<",\"confirmation_agree\":"<<r.confirmation_agree
  <<",\"confirmation_runs\":"<<r.confirmation_runs.size()
  <<",\"confirmed\":"<<r.confirmed
  <<",\"status\":\""<<esc(r.status)<<"\""
  <<",\"claim_boundary\":\""<<esc(r.claim_boundary)<<"\""
  <<",\"kind\":\"model_conditional_probe_scaffolding\"}";
 return o.str();
}

std::string captured_provider_replay_json(const CapturedProviderReplayReport& r){
 std::ostringstream o;
 o<<std::boolalpha
  <<"{\"kind\":\"captured_provider_transcript_replay\""
  <<",\"events\":"<<r.events.size()
  <<",\"byte_identical_raw\":"<<r.byte_identical_raw
  <<",\"outcome_match\":"<<r.outcome_match
  <<",\"cross_event_valid\":"<<r.cross_event_valid
  <<",\"claim_boundary\":\""<<esc(r.claim_boundary)<<"\"}";
 return o.str();
}

std::string historical_replay_json(const HistoricalReplayReport& r){
 return captured_provider_replay_json(r);
}
}
