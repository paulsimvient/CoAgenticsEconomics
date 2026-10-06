#include "coagentics/experiment/Dv026PhaseI.hpp"
#include "coagentics/experiment/Dv026LiveCampaign.hpp"
#include "coagentics/analysis/HumanReference.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
namespace coagentics::experiment {
namespace {
std::string esc(const std::string& s){
 std::string o; o.reserve(s.size());
 for(char c:s){ if(c=='\\'||c=='"') o.push_back('\\'); if(c=='\n'){o+="\\n"; continue;} o.push_back(c); }
 return o;
}

// Lightweight parse of campaign summary.json written by run_ollama_layer_b_campaign.
struct CampaignArtifactStatus {
 bool present{false};
 bool darpa_claim_ready{false};
 std::string scope;
 std::size_t distinct_live{0};
 std::size_t cells_ok{0};
 std::string path;
 bool revision_match{false};
};
CampaignArtifactStatus load_campaign_artifact(const std::string& path="results/dv026_ollama_campaign/summary.json"){
 CampaignArtifactStatus st;
 const std::vector<std::string> candidates={
  path,
  "../"+path,
  "../../"+path,
 };
 std::string chosen;
 for(const auto& c:candidates){
  if(std::filesystem::exists(c)){ chosen=c; break; }
 }
 if(chosen.empty()){ st.path=path; return st; }
 st.path=chosen;
 std::ifstream in(chosen); std::ostringstream ss; ss<<in.rdbuf();
 const std::string body=ss.str();
 if(body.empty()) return st;
 st.present=true;
 st.darpa_claim_ready=body.find("\"darpa_claim_ready\":true")!=std::string::npos;
 auto scope_key=body.find("\"scope\":\"");
 if(scope_key!=std::string::npos){
  auto q1=scope_key+9; auto q2=body.find('"', q1);
  if(q2!=std::string::npos) st.scope=body.substr(q1, q2-q1);
 }
 auto grab_size=[&](const char* key)->std::size_t{
  auto k=body.find(key); if(k==std::string::npos) return 0;
  auto colon=body.find(':', k); if(colon==std::string::npos) return 0;
  return static_cast<std::size_t>(std::stoull(body.substr(colon+1)));
 };
 st.distinct_live=grab_size("\"distinct_live_models_ok\"");
 st.cells_ok=grab_size("\"cells_ok\"");
 const std::string rev_key="\"implementation_revision\":\"2026-10-02-pass3-step13-preregistered-research-expectations\"";
 st.revision_match=body.find(rev_key)!=std::string::npos;
 return st;
}
const char* group_name(PhaseIGroup g){
 switch(g){
  case PhaseIGroup::MarketMechanics: return "A_MARKET_MECHANICS";
  case PhaseIGroup::LlmInterface: return "B_LLM_INTERFACE";
  case PhaseIGroup::MultiAgent: return "C_MULTI_AGENT";
  case PhaseIGroup::Dv026Qualification: return "D_DV026_QUALIFICATION";
 }
 return "UNKNOWN";
}
void add(PhaseISuiteReport& r, PhaseIGroup g, std::string id, std::string status,
 std::string evidence, std::string limitation){
 PhaseICheck c; c.group=g; c.id=std::move(id); c.status=std::move(status);
 c.evidence=std::move(evidence); c.limitation=std::move(limitation);
 r.checks.push_back(std::move(c));
}
}

PhaseISuiteReport run_phase_i_qualification_suite(std::uint64_t seed){
 PhaseISuiteReport out;

 // ----- A. Market mechanics (Layer A) -----
 try{
  MarketSpec spec; spec.buyers=4; spec.sellers=4; spec.periods=5; spec.efficiency_gate=90;
  auto mq=run_market_qualification(spec, seed, 5);
  add(out, PhaseIGroup::MarketMechanics, "A1_CDA_EFFICIENCY",
   mq.cda_qualified?"PASS":"FAIL",
   "mean_efficiency_cda="+std::to_string(mq.mean_efficiency_cda),
   "Programmed reference agents only; not LLM performance.");
  add(out, PhaseIGroup::MarketMechanics, "A2_SEALED_EFFICIENCY",
   mq.sealed_qualified?"PASS":"FAIL",
   "mean_efficiency_sealed="+std::to_string(mq.mean_efficiency_sealed),
   "Programmed reference agents only; not LLM performance.");
  add(out, PhaseIGroup::MarketMechanics, "A3_LAYER_A_GATE",
   mq.layer_a_pass?"PASS":"FAIL",
   "layer_a_pass: every qualification trial >90% for both mechanisms",
   "Qualifies MARKET TEST ENVIRONMENT only.");
 }catch(const std::exception& e){
  add(out, PhaseIGroup::MarketMechanics, "A_MARKET_EXCEPTION", "FAIL", e.what(), "Suite aborted in group A");
 }

 // ----- B. LLM interface -----
 try{
  ExperimentSpec exp; exp.deterministic_counterparty=true; exp.counterparty_limit_price=90;
  auto transport=std::make_shared<RawJsonTransport>(std::vector<std::string>{
   R"({"action":"BUY","asset":"ASSET","quantity":1,"price":95,"time":0})"
  });
  RunSpec run; run.seed=seed; run.model={"mock","phase-i","v1"}; run.transport=transport; run.run_id="phase-i-b";
  auto result=run_llm_market_experiment(exp, run);
  const bool turn_ok=!result.turns.empty() && result.turns[0].parse.success
   && result.turns[0].action_validation.valid && result.turns[0].submission.accepted;
  add(out, PhaseIGroup::LlmInterface, "B1_PARSE_VALIDATE_SUBMIT",
   turn_ok?"PASS":"FAIL",
   "turns="+std::to_string(result.turns.size())+" filled="+std::to_string(result.units_filled),
   "Mock transport; not live provider.");
  const bool boundary=!result.turns.empty()
   && result.turns[0].canonical_request.find("private_value")!=std::string::npos
   && result.turns[0].canonical_request.find(run.seller_agent_id)==std::string::npos;
  add(out, PhaseIGroup::LlmInterface, "B2_PRIVATE_STATE_BOUNDARY",
   boundary?"PASS":"FAIL",
   "own private_value present; counterparty id absent from decision payload",
   "Information boundary check only.");
  auto log=capture_historical_replay_log(result, exp, run);
  auto replay=replay_and_validate(log);
  add(out, PhaseIGroup::LlmInterface, "B3_HISTORICAL_REPLAY",
   replay.cross_event_valid?"PASS":"FAIL",
   "cross_event_valid="+std::string(replay.cross_event_valid?"true":"false"),
   "Captured-payload replay; not live-provider fidelity.");
 }catch(const std::exception& e){
  add(out, PhaseIGroup::LlmInterface, "B_LLM_EXCEPTION", "FAIL", e.what(), "Suite aborted in group B");
 }

 // ----- C. Multi-agent -----
 try{
  PopulationSpec pop; pop.seed=seed; pop.rounds=1;
  pop.market.asset="ASSET"; pop.market.fundamental=100;
  AgentSlot buyer_llm;
  buyer_llm.agent_id="LLM-B0"; buyer_llm.kind=AgentKind::Llm; buyer_llm.side=market::Side::Buy;
  buyer_llm.private_value_or_cost=120; buyer_llm.cash=10000;
  buyer_llm.model={"mock","phase-i-pop","v1"};
  buyer_llm.transport=std::make_shared<RawJsonTransport>(std::vector<std::string>{
   R"({"action":"BUY","asset":"ASSET","quantity":1,"price":95,"time":0})"
  });
  AgentSlot seller;
  seller.agent_id="S0"; seller.kind=AgentKind::ProgrammedHeuristic; seller.side=market::Side::Sell;
  seller.private_value_or_cost=80; seller.inventory=1; seller.cash=0;
  pop.agents={buyer_llm, seller};
  auto run=run_population_market(pop);
  add(out, PhaseIGroup::MultiAgent, "C1_MIXED_POPULATION_TRADE",
   run.trades.size()>=1?"PASS":"FAIL",
   "agents="+std::to_string(run.agents.size())+" trades="+std::to_string(run.trades.size()),
   "Observable multi-agent mediation only.");
  add(out, PhaseIGroup::MultiAgent, "C2_STANDALONE_NO_CLASSIFIER",
   (!run.classifier_available && run.classifier_mode=="none" && run.classification.label.empty())?"PASS":"FAIL",
   "classifier_mode="+run.classifier_mode,
   "Standalone population outcomes must not be presented as behavioral classification; paired observed contrasts are tested in D3.");
  add(out, PhaseIGroup::MultiAgent, "C3_HUMAN_REFERENCE_ATTACHED",
   !run.human_reference.empty()?"PASS":"FAIL",
   "comparisons="+std::to_string(run.human_reference.size()),
   "Literature catalog comparison; not new human-subject research.");
 }catch(const std::exception& e){
  add(out, PhaseIGroup::MultiAgent, "C_MULTI_EXCEPTION", "FAIL", e.what(), "Suite aborted in group C");
 }

 // ----- D. DV026 qualification rollup -----
 try{
  MarketSpec spec; spec.efficiency_gate=90; spec.periods=5;
  auto mq=run_market_qualification(spec, seed+7, 5);
  add(out, PhaseIGroup::Dv026Qualification, "D1_MARKET_GT_90",
   mq.layer_a_pass?"PASS":"FAIL",
   "cda="+std::to_string(mq.mean_efficiency_cda)+" sealed="+std::to_string(mq.mean_efficiency_sealed)+" strict_every_trial_gt_90",
   "Software Layer A gate; DARPA PoC live reproduction remains NOT_CLAIMED.");

  auto catalog=dv026_distinct_llm_catalog();
  auto ten=run_ten_llm_interface_qualification(catalog, seed);
  add(out, PhaseIGroup::Dv026Qualification, "D2_TEN_LLM_INTERFACE",
   ten.qualifies?"PASS":"FAIL",
   "distinct_models="+std::to_string(ten.distinct_models),
   "Interface/provenance via ScriptedTransport; live multi-provider suite NOT_CLAIMED.");

  std::vector<market::Bid> ctl={{0,"T","ASSET",1,100,market::Side::Buy}};
  std::vector<market::Bid> trt={{0,"T","ASSET",1,110,market::Side::Buy}};
  auto clf=run_operational_classifier(ctl, trt, 0, {"T"});
  add(out, PhaseIGroup::Dv026Qualification, "D3_OPERATIONAL_CLASSIFIER",
   !clf.classification.label.empty()?"PASS":"FAIL",
   "label="+clf.classification.label,
   "Operational wiring only; no accuracy/performance metrics (FAQ 31).");

  auto refs=analysis::empirical_market_reference_catalog();
  auto cmp=refs.compare("allocative_efficiency", mq.mean_efficiency_cda);
  std::size_t empirical_n=0;
  for(const auto& m:refs.metrics()) if(refs.get(m).empirical) ++empirical_n;
  add(out, PhaseIGroup::Dv026Qualification, "D4_HUMAN_REFERENCE_COMPARE",
   empirical_n>=2?"PASS":"PARTIAL",
   "empirical_metrics="+std::to_string(empirical_n)+
   " observed_cda="+std::to_string(mq.mean_efficiency_cda)+" z="+std::to_string(cmp.z_score),
   "Software comparison to Gode–Sunder catalog (≥2 condition-tagged metrics); not protocol replication claim.");

  std::vector<Candidate> h={{"blind",0,0,0,.25},{"news",1,0,0,.25},{"peer",0,.6,0,.25},{"mixed",1,.6,0,.25}};
  std::vector<Probe> p={{"news-only",10,0,1},{"peer-only",0,20,1},{"both",10,20,1}};
  ScientistConfig cfg; cfg.noise_sd=.25; cfg.stop_posterior=.95;
  ConfirmationConfig conf; conf.heldout_seeds=2; conf.min_posterior=.80;
  AgentProbeDomain domain("signal-responsive");
  auto adapt=run_adaptive_discovery_confirmation(domain, h, p, seed, cfg, conf);
  add(out, PhaseIGroup::Dv026Qualification, "D5_ADAPTIVE_DISCOVERY_CONFIRM",
   adapt.confirmed?"PASS":"FAIL",
   "leading="+adapt.leading_hypothesis+" status="+adapt.status,
   "Model-conditional controlled probes; not Phase II constructs.");

  auto camp=load_campaign_artifact();
  if(!camp.present){
   add(out, PhaseIGroup::Dv026Qualification, "D6_LIVE_10_LLM_ECONOMIC",
    "NOT_CLAIMED",
    "No campaign artifact at "+camp.path+"; run ollama-campaign (10 models × N seeds)",
    "Local Ollama PoC matrix required before darpa_claim_ready; commercial multi-provider still follow-on.");
  }else if(camp.darpa_claim_ready && camp.scope=="local_ollama_poc" && camp.revision_match){
   add(out, PhaseIGroup::Dv026Qualification, "D6_LIVE_10_LLM_ECONOMIC",
    "PASS",
    "campaign scope=local_ollama_poc distinct_live="+std::to_string(camp.distinct_live)+
    " cells_ok="+std::to_string(camp.cells_ok)+" revision_match=true",
    "Local Ollama PoC only; does not claim commercial 10-provider matrix.");
  }else{
   add(out, PhaseIGroup::Dv026Qualification, "D6_LIVE_10_LLM_ECONOMIC",
    "PARTIAL",
    "campaign present scope="+camp.scope+" distinct_live="+std::to_string(camp.distinct_live)+
    " cells_ok="+std::to_string(camp.cells_ok)+" ready=false",
    "Campaign artifacts exist but readiness gate not fully passed.");
  }
  add(out, PhaseIGroup::Dv026Qualification, "D7_PHASE_II_BIAS_DECEPTION",
   "NOT_CLAIMED",
   "Phase II operational scaffolding exists (see Dv026PhaseII); constructs not validated",
   "Observable protocols only; no intent inference or Phase II milestone claim.");
 }catch(const std::exception& e){
  add(out, PhaseIGroup::Dv026Qualification, "D_QUAL_EXCEPTION", "FAIL", e.what(), "Suite aborted in group D");
 }

 auto camp_ready=load_campaign_artifact();
 out.darpa_claim_ready=camp_ready.present && camp_ready.darpa_claim_ready
  && camp_ready.scope=="local_ollama_poc" && camp_ready.revision_match;
 out.phase_ii_deferred=true;
 out.software_green=true;
 for(const auto& c:out.checks){
  if(c.status=="FAIL"){ out.software_green=false; break; }
 }

 analysis::EvidenceEnvelope env;
 env.experiment_id="dv026-phase-i-suite";
 env.seed=seed;
 env.evidence.hypothesis_id="H_PHASE_I_SOFTWARE_EVIDENCE";
 env.evidence.discriminator_id="D_SUITE_ABCD";
 env.evidence.direction=out.software_green
  ?analysis::EvidenceDirection::Supports:analysis::EvidenceDirection::Challenges;
 env.evidence.rationale=
  "checks="+std::to_string(out.checks.size())+
  " software_green="+std::string(out.software_green?"true":"false")+
  " darpa_claim_ready="+std::string(out.darpa_claim_ready?"true":"false")+
  " scope="+(out.darpa_claim_ready?std::string("local_ollama_poc"):std::string("not_ready"))+
  " phase_ii_deferred=true";
 out.evidence.append(std::move(env));
 return out;
}

std::string phase_i_suite_markdown(const PhaseISuiteReport& r){
 std::ostringstream o;
 o<<"# DV026 Phase I Software Evidence Suite — "<<r.version<<"\n\n"
  <<"**Software green:** "<<(r.software_green?"YES":"NO")<<"  \n"
  <<"**DARPA claim ready:** "<<(r.darpa_claim_ready?"YES (scope=local_ollama_poc)":"NO")<<"  \n"
  <<"**Phase II:** DEFERRED\n\n"
  <<r.claim_boundary<<"\n\n";
 PhaseIGroup cur=static_cast<PhaseIGroup>(-1);
 for(const auto& c:r.checks){
  if(c.group!=cur){ cur=c.group; o<<"## "<<group_name(c.group)<<"\n\n"; }
  o<<"### "<<c.id<<" — "<<c.status<<"\n"
   <<"**Evidence:** "<<c.evidence<<"\n\n"
   <<"**Limitation:** "<<c.limitation<<"\n\n";
 }
 o<<"## Interpretation\n"
  <<"PASS means the Wave 1–4 software path executed as designed under mock/scripted transports. "
  <<"It does not convert a code test into an empirical claim about live LLMs, humans, or DARPA milestone satisfaction.\n";
 return o.str();
}

std::string phase_i_suite_json(const PhaseISuiteReport& r){
 std::ostringstream o;
 o<<std::boolalpha<<"{\"version\":\""<<esc(r.version)<<"\""
  <<",\"software_green\":"<<r.software_green
  <<",\"darpa_claim_ready\":"<<r.darpa_claim_ready
  <<",\"phase_ii_deferred\":"<<r.phase_ii_deferred
  <<",\"claim_boundary\":\""<<esc(r.claim_boundary)<<"\""
  <<",\"checks\":[";
 for(std::size_t i=0;i<r.checks.size();++i){
  const auto& c=r.checks[i];
  if(i) o<<",";
  o<<"{\"group\":\""<<group_name(c.group)<<"\",\"id\":\""<<esc(c.id)<<"\""
   <<",\"status\":\""<<esc(c.status)<<"\",\"evidence\":\""<<esc(c.evidence)<<"\""
   <<",\"limitation\":\""<<esc(c.limitation)<<"\"}";
 }
 o<<"]}";
 return o.str();
}
}
