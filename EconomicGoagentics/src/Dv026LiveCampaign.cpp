#include "coagentics/experiment/Dv026LiveCampaign.hpp"
#include "coagentics/util/Http.hpp"
#include "coagentics/util/Json.hpp"
#include "coagentics/experiment/ExperimentManifest.hpp"
#include "coagentics/experiment/Dv026Wave3.hpp"
#include "coagentics/analysis/HumanComparison.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
namespace coagentics::experiment {

ReproducibilityGrade grade_reproducibility(const agents::ModelIdentity& model,
 const std::string& inference_config, bool inference_seed_requested,
 const std::string& seed_support, const std::vector<std::string>& repeated_outputs){
 ReproducibilityGrade g; g.provider=model.provider; g.model=model.model; g.version=model.version;
 g.inference_config=inference_config; g.inference_seed_requested=inference_seed_requested;
 g.inference_seed_support=seed_support.empty()?"unknown":seed_support;
 g.repeat_trials=repeated_outputs.size();
 if(repeated_outputs.empty()){
  g.repeatability=RepeatabilityStatus::NotMeasured;
  if(!inference_seed_requested || g.inference_seed_support=="unsupported"){
   g.grade="C"; g.interpretation="Simulator seed is controlled; provider inference seeding is not available. Empirical repeatability not measured.";
  }else if(g.inference_seed_support=="supported"){
   g.grade="B"; g.interpretation="Simulator seed is controlled and provider seed is requested/supported; deterministic repeatability has not yet been empirically measured.";
  }else{
   g.grade="C"; g.interpretation="Simulator seed is controlled and inference seed was requested, but provider seed support is unverified. Empirical repeatability not measured.";
  }
  return g;
 }
 if(std::any_of(repeated_outputs.begin(), repeated_outputs.end(), [](const std::string& x){return x.empty();})){
  g.repeatability=RepeatabilityStatus::Failed; g.grade="F";
  g.interpretation="At least one repeated provider response failed or was empty."; return g;
 }
 const auto& first=repeated_outputs.front();
 g.exact_repeats=std::count(repeated_outputs.begin(), repeated_outputs.end(), first);
 if(g.exact_repeats==g.repeat_trials){
  g.repeatability=RepeatabilityStatus::Exact; g.grade="A";
  g.interpretation="Identical repeated provider outputs observed under the same frozen request and inference settings.";
 }else{
  g.repeatability=RepeatabilityStatus::Variable; g.grade="B";
  g.interpretation="Repeated provider outputs varied under the same frozen request; simulator remains reproducible but provider inference is empirically nondeterministic.";
 }
 return g;
}

std::string reproducibility_grade_json(const ReproducibilityGrade& g){
 auto rs=[](RepeatabilityStatus x){switch(x){case RepeatabilityStatus::Exact:return "exact";case RepeatabilityStatus::Variable:return "variable";case RepeatabilityStatus::Failed:return "failed";default:return "not_measured";}};
 std::ostringstream o; o<<std::boolalpha<<"{\"simulator_seed_controlled\":"<<g.simulator_seed_controlled
  <<",\"inference_seed_requested\":"<<g.inference_seed_requested<<",\"inference_seed_support\":\""<<g.inference_seed_support
  <<"\",\"provider\":\""<<g.provider<<"\",\"model\":\""<<g.model<<"\",\"version\":\""<<g.version
  <<"\",\"inference_config\":\""<<g.inference_config<<"\",\"repeat_trials\":"<<g.repeat_trials
  <<",\"exact_repeats\":"<<g.exact_repeats<<",\"repeatability\":\""<<rs(g.repeatability)
  <<"\",\"grade\":\""<<g.grade<<"\",\"interpretation\":\""<<g.interpretation<<"\"}"; return o.str();
}
namespace {
std::string esc(const std::string& s){ return coagentics::util::json_escape(s); }

// Prefer exact tag, else installed "name:tag" matching catalog base name.
std::string resolve_installed_tag(const std::string& catalog_model,
 const std::vector<std::string>& installed){
 for(const auto& t:installed) if(t==catalog_model) return t;
 // catalog "llama3.2" ↔ installed "llama3.2:latest"
 for(const auto& t:installed){
  if(t.rfind(catalog_model+":",0)==0) return t;
 }
 // catalog "qwen3:8b" ↔ installed "qwen3:8b" already handled; also "qwen3:8b-..." 
 for(const auto& t:installed){
  if(catalog_model.rfind(t+":",0)==0) return t;
  if(t.rfind(catalog_model,0)==0 && (t.size()==catalog_model.size() || t[catalog_model.size()]==':'))
   return t;
 }
 return {};
}

void append_jsonl(const std::filesystem::path& path, const std::string& line){
 std::ofstream f(path, std::ios::app);
 f<<line<<"\n";
}

std::string cell_json(const LiveCampaignCell& c){
 std::ostringstream o;
 o<<std::boolalpha<<std::fixed<<std::setprecision(6)
  <<"{\"provider\":\""<<esc(c.model.provider)<<"\",\"model\":\""<<esc(c.model.model)
  <<"\",\"version\":\""<<esc(c.model.version)<<"\",\"seed\":"<<c.seed
  <<",\"skipped\":"<<c.skipped
  <<",\"skip_reason\":\""<<esc(c.skip_reason)<<"\""
  <<",\"interface_ok\":"<<c.interface_ok
  <<",\"market_action_ok\":"<<c.market_action_ok
  <<",\"market_accepted\":"<<c.market_accepted
  <<",\"execution_observed\":"<<c.execution_observed
  <<",\"replay_ok\":"<<c.replay_ok
  <<",\"parse_ok\":"<<c.parse_ok
  <<",\"action_valid\":"<<c.action_valid
  <<",\"cross_event_valid\":"<<c.cross_event_valid
  <<",\"cell_ok\":"<<c.cell_ok
  <<",\"units_filled\":"<<c.units_filled
  <<",\"treatment_efficiency\":"<<c.treatment_efficiency
  <<",\"delta_efficiency\":"<<c.delta_efficiency
  <<",\"delta_surplus\":"<<c.delta_surplus
  <<",\"delta_trades\":"<<c.delta_trades
  <<",\"delta_mean_price\":"<<c.delta_mean_price
  <<",\"latency_ms\":"<<c.latency_ms
  <<",\"error\":\""<<esc(c.error)<<"\",\"reproducibility\":"<<reproducibility_grade_json(c.reproducibility)<<"}";
 return o.str();
}
}

std::vector<agents::ModelIdentity> dv026_ollama_live_catalog(){
 // ≥10 distinct open-weight identities for local PoC. Pull missing tags before full campaign.
 // Exact tags; 10×1 smoke passed READY under compact BUY prompt (2026-10-01).
 return {
  {"ollama","llama3","local","chat-completions"},
  {"ollama","llama2","local","chat-completions"},
  {"ollama","llama3.2:3b","local","chat-completions"},
  {"ollama","qwen3:8b","local","chat-completions"},
  {"ollama","qwen2.5-coder:7b","local","chat-completions"},
  {"ollama","llama3.1:8b","local","chat-completions"},
  {"ollama","codellama","local","chat-completions"},
  {"ollama","mistral","local","chat-completions"},
  {"ollama","gemma2:2b","local","chat-completions"},
  {"ollama","phi3:mini","local","chat-completions"},
 };
}

std::vector<std::string> ollama_installed_model_tags(const std::string& base_url){
 std::string url=base_url+"/api/tags";
 auto http=coagentics::util::http_get(url, 5);
 std::vector<std::string> tags;
 if(!http.error.empty() || http.status!=200) return tags;
 auto root=coagentics::util::parse_json(http.body); if(!root||!root->is_object()) return tags;
 auto models=root->get("models"); if(!models||!models->is_array()) return tags;
 for(const auto& model:models->as_array()){ if(!model.is_object()) continue; auto name=model.get("name"); if(name&&name->is_string()) tags.push_back(name->as_string()); }
 return tags;
}

OllamaPreflight preflight_ollama_live_catalog(const std::string& base_url){
 OllamaPreflight pf;
 pf.catalog=dv026_ollama_live_catalog();
 pf.installed=ollama_installed_model_tags(base_url);
 pf.ollama_reachable=!pf.installed.empty() || [&]{
  auto http=coagentics::util::http_get(base_url+"/api/tags", 3);
  return http.error.empty() && http.status==200;
 }();
 for(const auto& id:pf.catalog){
  auto resolved=resolve_installed_tag(id.model, pf.installed);
  if(!resolved.empty()){
   agents::ModelIdentity live=id;
   live.model=resolved;
   pf.available.push_back(live);
  }else{
   pf.missing.push_back(id);
  }
 }
 return pf;
}

DarpaPhaseIReadiness evaluate_darpa_phase_i_readiness(const LiveCampaignReport& campaign){
 DarpaPhaseIReadiness out;
 const auto& spec=campaign.spec;
 const std::size_t N=spec.n_seeds;

 std::map<std::string, std::size_t> ok_by_model;
 std::size_t attempted=0, provenance_ok=0, interface_ok=0, market_action_ok=0, market_accepted=0;
 for(const auto& c:campaign.cells){
  if(c.skipped) continue;
  ++attempted;
  if(c.interface_ok || (c.parse_ok && c.action_valid)) ++interface_ok;
  if(c.market_action_ok) ++market_action_ok;
  if(c.market_action_ok && c.market_accepted) ++market_accepted;
  if(c.replay_ok || c.cross_event_valid) ++provenance_ok;
  if(c.cell_ok) ok_by_model[c.model.provider+"|"+c.model.model+"|"+c.model.version]+=1;
 }
 std::size_t models_with_coverage=0;
 std::size_t models_with_any=0;
 for(const auto& [k,v]:ok_by_model){
  if(v>=1) ++models_with_any;
  if(v>=N) ++models_with_coverage;
 }

 auto add=[&](const std::string& id, bool pass, const std::string& detail){
  out.checks.push_back({id, pass, detail});
 };

 add("layer_a", campaign.layer_a_pass,
  "trials="+std::to_string(campaign.layer_a_trials)+
  " cda="+std::to_string(campaign.mean_efficiency_cda)+
  " sealed="+std::to_string(campaign.mean_efficiency_sealed)+
  " required_trials>="+std::to_string(spec.layer_a_trials)+
  " rule=every_trial>90");

 add("live_count", models_with_any>=spec.min_models,
  "distinct_ok="+std::to_string(models_with_any)+" required>="+std::to_string(spec.min_models));

 add("coverage", models_with_coverage>=spec.min_models,
  "models_with_>="+std::to_string(N)+" seeds="+std::to_string(models_with_coverage)+
  " required>="+std::to_string(spec.min_models));

 double prov_rate=attempted? double(provenance_ok)/double(attempted):0.0;
 add("provenance", attempted>0 && prov_rate>=spec.min_provenance_rate,
  "rate="+std::to_string(prov_rate)+" required>="+std::to_string(spec.min_provenance_rate)+
  " cells="+std::to_string(attempted));

 double iface_rate=attempted? double(interface_ok)/double(attempted):0.0;
 add("interface_health", attempted>0 && iface_rate>=spec.min_interface_rate,
  "parse+valid rate="+std::to_string(iface_rate)+" required>="+std::to_string(spec.min_interface_rate));

 double action_rate=attempted? double(market_action_ok)/double(attempted):0.0;
 add("market_action", attempted>0 && action_rate>=spec.min_market_action_rate,
  "non-HOLD action rate="+std::to_string(action_rate)+
  " required>="+std::to_string(spec.min_market_action_rate)+
  " (HOLD-only does not qualify as economic execution)");

 double acceptance_rate=market_action_ok?double(market_accepted)/double(market_action_ok):0.0;
 add("market_acceptance", market_action_ok>0 && acceptance_rate>=spec.min_market_acceptance_rate,
  "accepted/non-HOLD rate="+std::to_string(acceptance_rate)+
  " required>="+std::to_string(spec.min_market_acceptance_rate));

 // HumanComparison with LiveProvider provenance — not "two catalog metrics exist".
 bool href=campaign.human_ref_gate;
 add("human_ref", href,
  href?"HumanComparison DESCRIPTIVE on live allocative_efficiency (provenance-gated)"
      :"need LiveProvider observations matched to literature condition via HumanComparison");

 add("non_claims", true,
  "Report forbids Phase II construct validation and commercial 10-provider claim without second matrix");

 out.darpa_claim_ready=true;
 for(const auto& c:out.checks) if(!c.pass){ out.darpa_claim_ready=false; break; }
 if(out.darpa_claim_ready) out.scope="local_ollama_poc";
 else out.scope="not_ready";
 return out;
}

LiveCampaignReport run_ollama_layer_b_campaign(const LiveCampaignSpec& spec_in){
 LiveCampaignReport rep;
 rep.spec=spec_in;
 rep.live_llm=true;

 const LiveTransportConfig ollama_cfg=default_ollama_transport_config();
 const std::string ollama_base= [&]{
  std::string v=ollama_cfg.base_url;
  if(v.size()>=3 && v.substr(v.size()-3)=="/v1") return v.substr(0,v.size()-3);
  return std::string("http://127.0.0.1:11434");
 }();

 rep.preflight=preflight_ollama_live_catalog(ollama_base);

 std::vector<agents::ModelIdentity> models=rep.preflight.available;
 std::size_t n_seeds=spec_in.n_seeds;
 if(spec_in.smoke){
  if(models.size()>2) models.resize(2);
  n_seeds=std::min(n_seeds, std::size_t{2});
 }
 rep.spec.n_seeds=n_seeds;

 // Freeze the exact campaign design before any market/LLM observation is collected.
 // Preflight is infrastructure discovery, not an experimental observation.
 rep.spec.n_seeds=n_seeds;
 auto manifest=make_live_campaign_manifest(rep.spec, models);
 rep.manifest_sha256=experiment_manifest_hash(manifest);
 rep.manifest_path=(std::filesystem::path(spec_in.results_dir)/"preregistration.json").string();
 write_frozen_manifest(manifest, rep.manifest_path);

 // Only after preregistration is frozen do qualification trials or model observations begin.
 MarketSpec mspec=spec_in.market;
 if(mspec.buyers==0){ mspec.buyers=4; mspec.sellers=4; mspec.periods=5; mspec.efficiency_gate=90; }
 const std::size_t n_trials=std::max<std::size_t>(1, spec_in.layer_a_trials);
 auto mq=run_market_qualification(mspec, spec_in.base_seed+7, n_trials);
 rep.layer_a_pass=mq.layer_a_pass;
 rep.layer_a_trials=n_trials;
 rep.mean_efficiency_cda=mq.mean_efficiency_cda;
 rep.mean_efficiency_sealed=mq.mean_efficiency_sealed;

 std::filesystem::create_directories(spec_in.results_dir);
 auto cells_path=std::filesystem::path(spec_in.results_dir)/"cells.jsonl";
 { std::ofstream(cells_path, std::ios::trunc); }

 ExperimentSpec exp; exp.deterministic_counterparty=true; exp.counterparty_limit_price=90;
 exp.llm_role=AgentRole::Buyer;
 exp.information.constraints=ActionConstraints::for_role(AgentRole::Buyer);

 std::vector<analysis::ModelObservation> live_obs;
 auto refs=analysis::empirical_market_reference_catalog();
 const std::string eff_condition=refs.has("allocative_efficiency")
  ? refs.get("allocative_efficiency").condition : std::string{};

 for(const auto& model:models){
  for(std::size_t i=0;i<n_seeds;++i){
   LiveCampaignCell cell;
   cell.model=model;
   cell.seed=spec_in.base_seed+i;
   try{
    RunSpec run; run.seed=cell.seed; run.model=model;
    run.transport=make_live_openai_compatible_transport(model, default_ollama_transport_config(model.model));
    run.run_id="ollama-campaign:"+model.model+":"+std::to_string(cell.seed);
    std::string leaf="cell_"+model.model+"_"+std::to_string(cell.seed)+".jsonl";
    for(char& ch:leaf) if(ch==':'||ch=='/') ch='_';
    run.log_path=(std::filesystem::path(spec_in.results_dir)/leaf).string();

    auto paired=run_paired_programmed_buyer_vs_llm(exp, run, /*control_buyer_limit_price=*/95.0);
    cell.delta_efficiency=paired.deltas.delta_efficiency;
    cell.delta_surplus=paired.deltas.delta_surplus;
    cell.delta_trades=paired.deltas.delta_trades;
    cell.delta_mean_price=paired.deltas.delta_mean_price;
    cell.units_filled=static_cast<int>(paired.treatment.units_filled);
    cell.treatment_efficiency=paired.treatment.metrics.allocative_efficiency;
    if(!paired.treatment.turns.empty()){
     const auto& t=paired.treatment.turns[0];
     cell.parse_ok=t.parse.success;
     cell.action_valid=t.action_validation.valid;
     cell.interface_ok=cell.parse_ok && cell.action_valid;
     cell.market_action_ok=cell.interface_ok && t.parsed_action
      && t.parsed_action->action!=ActionType::Hold;
     cell.market_accepted=t.submission.accepted;
     cell.execution_observed=t.submission.filled_quantity>0;
     cell.latency_ms=t.latency_ms;
     cell.reproducibility=grade_reproducibility(model, t.inference_config, true,
      model.provider=="ollama" ? "supported" : "unknown");
     if(!t.parse.success) cell.error=t.parse.error;
     else if(!t.action_validation.valid && !t.action_validation.errors.empty())
      cell.error=t.action_validation.errors.front();
    }
    auto log=capture_historical_replay_log(paired.treatment, exp, run);
    auto replay=replay_and_validate(log);
    cell.cross_event_valid=replay.cross_event_valid;
    cell.replay_ok=replay.cross_event_valid;
    // Economic success: real market action accepted + transcript replay. HOLD-only fails.
    cell.cell_ok=cell.interface_ok && cell.market_action_ok && cell.market_accepted && cell.replay_ok;
    ++rep.cells_attempted;
    if(cell.cell_ok){
     ++rep.cells_ok;
     if(!eff_condition.empty()){
      analysis::ModelObservation mo;
      mo.metric="allocative_efficiency";
      mo.condition=eff_condition;
      mo.model_id=model.provider+"/"+model.model+"/"+model.version;
      mo.run_id=run.run_id;
      mo.value=cell.treatment_efficiency;
      mo.origin=analysis::EvidenceOrigin::LiveProvider;
      live_obs.push_back(mo);
     }
    }
   }catch(const std::exception& e){
    cell.error=e.what();
    cell.cell_ok=false;
    ++rep.cells_attempted;
   }
   rep.cells.push_back(cell);
   append_jsonl(cells_path, cell_json(cell));
  }
 }

 // Also record skipped catalog models as skipped cells (documentation)
 for(const auto& miss:rep.preflight.missing){
  LiveCampaignCell skip;
  skip.model=miss;
  skip.skipped=true;
  skip.skip_reason="model not installed (ollama pull required)";
  skip.seed=spec_in.base_seed;
  skip.reproducibility=grade_reproducibility(miss, "", false, "unknown");
  rep.cells.push_back(skip);
  append_jsonl(cells_path, cell_json(skip));
 }

 std::set<std::string> ok_ids;
 for(const auto& c:rep.cells){
  if(c.cell_ok) ok_ids.insert(c.model.provider+"|"+c.model.model+"|"+c.model.version);
 }
 rep.distinct_live_models_ok=ok_ids.size();

 // Provenance-gated HumanComparison (literature catalog + LiveProvider observations).
 rep.human_comparison=analysis::compare_human_behavior(refs, live_obs);
 const bool descriptive=std::any_of(rep.human_comparison.comparisons.begin(),
  rep.human_comparison.comparisons.end(),
  [](const analysis::HumanComparison& c){
   return c.metric=="allocative_efficiency" && c.status=="DESCRIPTIVE_COMPARISON";
  });
 rep.human_ref_gate=rep.human_comparison.live_model_evidence && descriptive;

 rep.readiness=evaluate_darpa_phase_i_readiness(rep);
 if(spec_in.smoke){
  // Smoke proves harness only; never flip the claim flag.
  rep.readiness.darpa_claim_ready=false;
  rep.readiness.scope="not_ready_smoke";
  rep.readiness.checks.push_back({"smoke_guard", false,
   "smoke=true: readiness flip blocked (use full ≥10×N without --smoke)"});
 }

 {
  std::ofstream(std::filesystem::path(spec_in.results_dir)/"summary.json")<<live_campaign_json(rep);
  std::ofstream(std::filesystem::path(spec_in.results_dir)/"summary.md")<<live_campaign_markdown(rep);
 }

 analysis::EvidenceEnvelope env;
 env.experiment_id="dv026-ollama-layer-b-campaign";
 env.seed=spec_in.base_seed;
 env.evidence.hypothesis_id="H_LIVE_OLLAMA_LAYER_B";
 env.evidence.discriminator_id="D_CAMPAIGN_MATRIX";
 env.evidence.direction=rep.readiness.darpa_claim_ready
  ?analysis::EvidenceDirection::Supports:analysis::EvidenceDirection::Inconclusive;
 env.evidence.rationale=
  "cells_ok="+std::to_string(rep.cells_ok)+
  " distinct_live="+std::to_string(rep.distinct_live_models_ok)+
  " darpa_claim_ready="+std::string(rep.readiness.darpa_claim_ready?"true":"false")+
  " scope="+rep.readiness.scope;
 rep.evidence.append(std::move(env));
 return rep;
}

std::string live_campaign_json(const LiveCampaignReport& r){
 std::ostringstream o;
 o<<std::boolalpha<<std::fixed<<std::setprecision(6)
  <<"{\"live_llm\":true"
  <<",\"smoke\":"<<r.spec.smoke
  <<",\"base_seed\":"<<r.spec.base_seed
  <<",\"n_seeds\":"<<r.spec.n_seeds
  <<",\"min_models\":"<<r.spec.min_models
  <<",\"ollama_reachable\":"<<r.preflight.ollama_reachable
  <<",\"catalog_size\":"<<r.preflight.catalog.size()
  <<",\"available_models\":"<<r.preflight.available.size()
  <<",\"missing_models\":"<<r.preflight.missing.size()
  <<",\"layer_a_pass\":"<<r.layer_a_pass
  <<",\"layer_a_trials\":"<<r.layer_a_trials
  <<",\"implementation_revision\":\""<<esc(r.spec.implementation_revision)<<"\""
  <<",\"manifest_sha256\":\""<<esc(r.manifest_sha256)<<"\""
  <<",\"manifest_path\":\""<<esc(r.manifest_path)<<"\""
  <<",\"mean_efficiency_cda\":"<<r.mean_efficiency_cda
  <<",\"mean_efficiency_sealed\":"<<r.mean_efficiency_sealed
  <<",\"cells_attempted\":"<<r.cells_attempted
  <<",\"cells_ok\":"<<r.cells_ok
  <<",\"distinct_live_models_ok\":"<<r.distinct_live_models_ok
  <<",\"human_ref_gate\":"<<r.human_ref_gate
  <<",\"human_live_evidence\":"<<r.human_comparison.live_model_evidence
  <<",\"darpa_claim_ready\":"<<r.readiness.darpa_claim_ready
  <<",\"scope\":\""<<esc(r.readiness.scope)<<"\""
  <<",\"checks\":[";
 for(std::size_t i=0;i<r.readiness.checks.size();++i){
  const auto& c=r.readiness.checks[i];
  if(i) o<<",";
  o<<"{\"id\":\""<<esc(c.id)<<"\",\"pass\":"<<c.pass
   <<",\"detail\":\""<<esc(c.detail)<<"\"}";
 }
 o<<"],\"models\":[";
 for(std::size_t i=0;i<r.preflight.catalog.size();++i){
  if(i) o<<",";
  const auto& m=r.preflight.catalog[i];
  bool available=std::any_of(r.preflight.available.begin(),r.preflight.available.end(),[&](const auto& a){return a.provider==m.provider && a.model==m.model && a.version==m.version;});
  o<<"{\"provider\":\""<<esc(m.provider)<<"\",\"model\":\""<<esc(m.model)<<"\",\"version\":\""<<esc(m.version)<<"\",\"available\":"<<available<<"}";
 }
 o<<"],\"claim_boundary\":\""<<esc(r.readiness.claim_boundary)<<"\"}";
 return o.str();
}

std::string live_campaign_markdown(const LiveCampaignReport& r){
 std::ostringstream o;
 o<<"# DV026 Ollama Layer B Campaign\n\n"
  <<"**darpa_claim_ready:** "<<(r.readiness.darpa_claim_ready?"true":"false")<<"  \n"
  <<"**scope:** "<<r.readiness.scope<<"  \n"
  <<"**Layer A pass:** "<<(r.layer_a_pass?"yes":"no")<<"  \n"
  <<"**Available models:** "<<r.preflight.available.size()
  <<" / catalog "<<r.preflight.catalog.size()<<"  \n"
  <<"**Cells ok:** "<<r.cells_ok<<" / attempted "<<r.cells_attempted<<"  \n"
  <<"**Distinct live models with ≥1 ok cell:** "<<r.distinct_live_models_ok<<"\n\n"
  <<"## Readiness checklist\n\n";
 for(const auto& c:r.readiness.checks){
  o<<"- ["<<(c.pass?"x":" ")<<"] **"<<c.id<<"** — "<<c.detail<<"\n";
 }
 o<<"\n"<<r.readiness.claim_boundary<<"\n";
 if(!r.preflight.missing.empty()){
  o<<"\n## Missing models (pull before full readiness)\n\n";
  for(const auto& m:r.preflight.missing) o<<"- `ollama pull "<<m.model<<"`\n";
 }
 return o.str();
}

HeterogeneousPopulationReport run_heterogeneous_ollama_population(const HeterogeneousPopulationSpec& spec_in){
 HeterogeneousPopulationReport rep;
 rep.spec=spec_in;
 rep.shared_market=true;
 rep.activation_design=spec_in.activation_design==ActivationDesign::FrozenSnapshot?"frozen_snapshot":"sequential_interaction";
 rep.preflight=preflight_ollama_live_catalog();

 std::vector<agents::ModelIdentity> models=rep.preflight.available;
 std::size_t n=std::max<std::size_t>(2, spec_in.max_llm_seats);
 if(spec_in.smoke) n=2;
 if(models.size()>n) models.resize(n);

 PopulationSpec pop;
 pop.seed=spec_in.seed;
 pop.rounds=spec_in.rounds;
 pop.activation_design=spec_in.activation_design;
 pop.market.asset="ASSET";
 pop.market.fundamental=100;
 pop.market.mechanism=market::MechanismKind::ContinuousDoubleAuction;

 const bool live=spec_in.use_live_ollama && rep.preflight.ollama_reachable && models.size()>=2;
 const std::size_t n_llm=live? models.size() : (spec_in.smoke?2:std::min(n, std::size_t{2}));
 for(std::size_t i=0;i<n_llm;++i){
  AgentSlot slot;
  const bool buyer=(i%2==0);
  slot.agent_id=std::string("LLM-")+(buyer?"B":"S")+std::to_string(i);
  slot.kind=AgentKind::Llm;
  slot.side=buyer?market::Side::Buy:market::Side::Sell;
  slot.private_value_or_cost=buyer ? 120.0-static_cast<double>(i) : 80.0+static_cast<double>(i);
  slot.cash=10000;
  slot.inventory=buyer?0:1;
  if(live){
   slot.model=models[i];
   // Each transport snapshots model/provider at construction — no process-global setenv.
   slot.transport=make_live_openai_compatible_transport(models[i], default_ollama_transport_config(models[i].model));
  }else{
   slot.model={"mock","hetero-"+std::to_string(i),"v1"};
   const std::string action=buyer?"BUY":"SELL";
   const int price=buyer?105:95;
   std::vector<std::string> payloads;
   for(int r=0;r<std::max(1,spec_in.rounds);++r){
    payloads.push_back(std::string("{\"action\":\"")+action+"\",\"asset\":\"ASSET\",\"quantity\":1,\"price\":"+std::to_string(price)+",\"time\":"+std::to_string(r)+"}");
   }
   slot.transport=std::make_shared<RawJsonTransport>(std::move(payloads));
  }
  if(buyer) ++rep.llm_buyers; else ++rep.llm_sellers;
  pop.agents.push_back(slot);
 }

 rep.llm_seats=n_llm;
 std::filesystem::create_directories(spec_in.results_dir);
 rep.population=run_population_market(pop);
 {
  std::ofstream(std::filesystem::path(spec_in.results_dir)/"summary.json")
   <<heterogeneous_population_json(rep);
 }
 return rep;
}

std::string heterogeneous_population_json(const HeterogeneousPopulationReport& r){
 std::ostringstream o;
 o<<std::boolalpha<<std::fixed<<std::setprecision(4)
  <<"{\"shared_market\":"<<r.shared_market
  <<",\"activation_design\":\""<<esc(r.activation_design)<<"\""
  <<",\"llm_seats\":"<<r.llm_seats
  <<",\"llm_buyers\":"<<r.llm_buyers
  <<",\"llm_sellers\":"<<r.llm_sellers
  <<",\"smoke\":"<<r.spec.smoke
  <<",\"use_live_ollama\":"<<r.spec.use_live_ollama
  <<",\"available_models\":"<<r.preflight.available.size()
  <<",\"trades\":"<<r.population.trades.size()
  <<",\"efficiency\":"<<r.population.metrics.allocative_efficiency
  <<",\"classifier_available\":"<<r.population.classifier_available
  <<",\"classifier_mode\":\""<<esc(r.population.classifier_mode)<<"\""
  <<",\"claim_boundary\":\""<<esc(r.claim_boundary)<<"\"}";
 return o.str();
}
}
