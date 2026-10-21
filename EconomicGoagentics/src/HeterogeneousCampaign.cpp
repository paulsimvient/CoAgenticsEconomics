#include "coagentics/experiment/HeterogeneousCampaign.hpp"
#include "coagentics/util/Json.hpp"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <stdexcept>
#include <sstream>
#include <unistd.h>
namespace coagentics::experiment {
namespace {
std::string esc(const std::string&s){return coagentics::util::json_escape(s);}
std::string design_name(ActivationDesign d){return d==ActivationDesign::FrozenSnapshot?"frozen_snapshot":"sequential_interaction";}
bool incremental_evidence(const HeterogeneousCampaignSpec& in){
 if(std::getenv("COAGENTICS_RESUME_RUN_DIR")||std::getenv("COAGENTICS_INCREMENTAL_EVIDENCE")) return true;
 return !in.smoke;
}
void atomic_write_file(const std::filesystem::path& dest, const std::string& data, bool overwrite){
 std::filesystem::create_directories(dest.parent_path());
 if(std::filesystem::exists(dest)&&!overwrite)
  throw std::runtime_error("refusing to overwrite completed evidence "+dest.string());
 auto tmp=dest; tmp+=(".tmp."+std::to_string(getpid()));
 int fd=::open(tmp.c_str(), O_WRONLY|O_CREAT|O_EXCL, 0644);
 if(fd<0) throw std::runtime_error("unable to open temp evidence file "+tmp.string());
 ssize_t n=::write(fd, data.data(), data.size());
 if(n<0||static_cast<size_t>(n)!=data.size()){ ::close(fd); std::filesystem::remove(tmp); throw std::runtime_error("short write to "+tmp.string()); }
 if(::fsync(fd)!=0){ ::close(fd); throw std::runtime_error("fsync failed for "+tmp.string()); }
 ::close(fd);
 std::filesystem::rename(tmp, dest);
}
bool evidence_completed(const std::filesystem::path& run_dir, std::size_t index){
 auto p=run_dir/"cell_evidence"/("cell_"+std::to_string(index)+".json");
 if(!std::filesystem::exists(p)) return false;
 std::ifstream in(p); std::string s((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
 return s.find("\"status\":\"completed\"")!=std::string::npos;
}
void append_progress(const std::filesystem::path& run_dir, const std::string& line){
 auto p=run_dir/"hetero_progress.jsonl";
 std::ofstream out(p, std::ios::app);
 out<<line<<"\n";
 out.flush();
}
std::string fmt4(double v){ std::ostringstream o; o<<std::fixed<<std::setprecision(4)<<v; return o.str(); }
std::string cell_evidence_json(std::size_t index, std::uint64_t seed, std::size_t repeat, ActivationDesign d,
 const PopulationRunResult& pop, double elapsed){
 std::size_t turns=0; for(const auto&a:pop.agents) turns+=a.llm_turns.size();
 std::ostringstream e;
 e<<"{\"schema_version\":1,\"index\":"<<index<<",\"seed\":"<<seed<<",\"repeat\":"<<repeat
  <<",\"design\":\""<<design_name(d)<<"\",\"worker_id\":0,\"elapsed_seconds\":"<<elapsed
  <<",\"observed_llm_turns\":"<<turns<<",\"agents\":"<<pop.agents.size()
  <<",\"status\":\"completed\",\"error\":\"\",\"efficiency\":"<<pop.metrics.allocative_efficiency
  <<",\"trades\":"<<pop.trades.size()<<",\"assignment\":[";
 for(std::size_t j=0;j<pop.spec.agents.size();++j){const auto&a=pop.spec.agents[j]; if(j)e<<",";
  e<<"{\"seat\":\""<<esc(a.agent_id)<<"\",\"model\":\""<<esc(a.model.model)
   <<"\",\"provider\":\""<<esc(a.model.provider)<<"\",\"version\":\""<<esc(a.model.version)
   <<"\",\"role\":\""<<(a.side==market::Side::Buy?"buyer":"seller")
   <<"\",\"private_value_or_cost\":"<<a.private_value_or_cost
   <<"\",\"cash\":"<<a.cash<<",\"inventory\":"<<a.inventory<<"}";}
 e<<"]}";
 return e.str();
}
std::string cell_observation_json(std::size_t index, std::uint64_t seed, std::size_t repeat, ActivationDesign d,
 const PopulationRunResult& pop){
 std::ostringstream e;
 e<<"{\"schema_version\":1,\"index\":"<<index<<",\"seed\":"<<seed<<",\"repeat\":"<<repeat
  <<",\"design\":\""<<design_name(d)<<"\",\"trades\":[";
 for(std::size_t ti=0;ti<pop.trades.size();++ti){const auto&t=pop.trades[ti]; if(ti)e<<",";
  e<<"{\"time\":"<<t.time<<",\"asset\":\""<<esc(t.asset)<<"\",\"quantity\":"<<t.quantity
   <<",\"price\":"<<t.price<<",\"buyer\":\""<<esc(t.buyer)<<"\",\"seller\":\""<<esc(t.seller)<<"\"}";}
 e<<"],\"bids\":[";
 for(std::size_t bi=0;bi<pop.bids.size();++bi){const auto&b=pop.bids[bi]; if(bi)e<<",";
  e<<"{\"time\":"<<b.time<<",\"agent_id\":\""<<esc(b.agent_id)<<"\",\"asset\":\""<<esc(b.asset)
   <<"\",\"quantity\":"<<b.quantity<<",\"price\":"<<b.price
   <<",\"side\":\""<<(b.side==market::Side::Buy?"buy":"sell")<<"\"}";}
 e<<"],\"agent_turns\":[";
 bool first=true;
 for(const auto&a:pop.agents) for(const auto&t:a.llm_turns){
  if(!first)e<<","; first=false;
  e<<"{\"agent_id\":\""<<esc(a.agent_id)<<"\",\"provider\":\""<<esc(t.model.provider)
   <<"\",\"model\":\""<<esc(t.model.model)<<"\",\"version\":\""<<esc(t.model.version)
   <<"\",\"request_id\":\""<<esc(t.request_id)<<"\",\"time\":"<<t.time<<",\"seed\":"<<t.seed
   <<",\"canonical_request\":\""<<esc(t.canonical_request)
   <<"\",\"raw_provider_response\":\""<<esc(t.raw_provider_response)
   <<"\",\"latency_ms\":"<<t.latency_ms<<",\"prompt_tokens\":"<<t.prompt_tokens
   <<",\"completion_tokens\":"<<t.completion_tokens<<"}";
 }
 e<<"]}";
 return e.str();
}
std::string cell_snapshot_json(const PopulationRunResult& pop){
 std::ostringstream e;
 e<<"{\"agents\":"<<pop.agents.size()<<",\"trades\":"<<pop.trades.size()
  <<",\"efficiency\":"<<pop.metrics.allocative_efficiency
  <<",\"classifier_available\":"<<(pop.classifier_available?"true":"false")
  <<",\"classifier\":\""<<esc(pop.classification.label)<<"\",\"classifier_mode\":\""<<esc(pop.classifier_mode)
  <<"\",\"classifier_synthetic\":"<<(pop.classifier_synthetic?"true":"false")<<",\"fingerprints\":[";
 for(std::size_t i=0;i<pop.agents.size();++i){const auto&a=pop.agents[i]; const auto&f=a.fingerprint; if(i)e<<",";
  e<<"{\"agent_id\":\""<<esc(a.agent_id)<<"\",\"model\":\""<<esc(a.model.model)
   <<"\",\"aggressiveness\":"<<fmt4(f.aggressiveness)
   <<",\"reservation_value_violation_rate\":"<<fmt4(f.reservation_value_violation_rate)
   <<",\"information_sensitivity\":"<<fmt4(f.information_sensitivity)
   <<",\"peer_sensitivity\":"<<fmt4(f.peer_sensitivity)
   <<",\"price_improvement\":"<<fmt4(f.price_improvement)
   <<",\"trade_frequency\":"<<fmt4(f.trade_frequency)
   <<",\"surplus_capture\":"<<fmt4(f.surplus_capture)
   <<",\"action_validity\":"<<fmt4(f.action_validity)
   <<",\"response_consistency\":"<<fmt4(f.response_consistency)<<"}";}
 e<<"]}";
 return e.str();
}
void persist_completed_cell(const std::filesystem::path& run_dir, std::size_t index, std::uint64_t seed,
 std::size_t repeat, ActivationDesign d, const PopulationRunResult& pop, double elapsed){
 auto ev=run_dir/"cell_evidence"/("cell_"+std::to_string(index)+".json");
 if(std::filesystem::exists(ev))
  throw std::runtime_error("refusing to overwrite completed evidence "+ev.string());
 atomic_write_file(run_dir/"cell_observations"/("cell_"+std::to_string(index)+".json"),
  cell_observation_json(index,seed,repeat,d,pop), false);
 atomic_write_file(run_dir/"cell_snapshots"/("cell_"+std::to_string(index)+".json"),
  cell_snapshot_json(pop), false);
 atomic_write_file(ev, cell_evidence_json(index,seed,repeat,d,pop,elapsed), false);
}
void write_run_manifest(const std::filesystem::path& run_dir, const HeterogeneousCampaignSpec& in,
 const std::vector<ActivationDesign>& designs){
 auto p=run_dir/"run_manifest.json";
 if(std::filesystem::exists(p)) return;
 std::size_t planned=std::max<std::size_t>(1,in.n_seeds)*std::max<std::size_t>(1,designs.size())*std::max<std::size_t>(1,in.inference_repeats);
 std::ostringstream o;
 o<<"{\"campaign_id\":\""<<esc(in.campaign_id)<<"\",\"base_seed\":"<<in.base_seed
  <<",\"n_seeds\":"<<in.n_seeds<<",\"repeats\":"<<in.inference_repeats
  <<",\"designs\":[";
 for(std::size_t i=0;i<designs.size();++i){if(i)o<<","; o<<"\""<<design_name(designs[i])<<"\"";}
 o<<"],\"cells_planned\":"<<planned<<",\"reconstructed\":false}";
 atomic_write_file(p, o.str(), false);
}
std::shared_ptr<agents::ModelTransport> transport_for(const agents::ModelIdentity&m,bool live,market::Side side,int rounds){
 if(live) return make_live_openai_compatible_transport(m, default_ollama_transport_config(m.model));
 std::vector<std::string> p; const bool buy=side==market::Side::Buy; for(int r=0;r<std::max(1,rounds);++r)
  p.push_back(std::string("{\"action\":\"")+(buy?"BUY":"SELL")+"\",\"asset\":\"ASSET\",\"quantity\":1,\"price\":"+(buy?"105":"95")+",\"time\":"+std::to_string(r)+"}");
 return std::make_shared<RawJsonTransport>(std::move(p));
}
PopulationSpec make_population(const std::vector<BalancedSeatAssignment>& asg,std::uint64_t seed,int rounds,ActivationDesign design,bool live,double signal){
 PopulationSpec p; p.seed=seed;p.rounds=rounds;p.activation_design=design;p.market.asset="ASSET";p.market.fundamental=100;p.public_signal=signal;p.market.mechanism=market::MechanismKind::ContinuousDoubleAuction;
 for(const auto&a:asg){AgentSlot s;s.agent_id=a.agent_id;s.kind=AgentKind::Llm;s.side=a.side;s.private_value_or_cost=a.private_value_or_cost;s.cash=10000;s.inventory=a.side==market::Side::Sell?1:0;s.model=a.model;s.transport=transport_for(a.model,live,a.side,rounds);p.agents.push_back(std::move(s));}
 // Rotate the preregistered activation order independently of model identity; repeated inference preserves it.
 std::vector<std::pair<std::size_t,std::string>> ordered; for(const auto&a:asg) ordered.push_back({a.activation_position,a.agent_id});
 std::sort(ordered.begin(),ordered.end()); for(const auto&x:ordered)p.activation_order.push_back(x.second);
 return p;
}
DecisionContext fixed_context(){DecisionContext c;c.market.time=0;c.market.asset="ASSET";c.market.fundamental=100;c.market.public_signal=0;c.market.best_bid=98;c.market.best_ask=102;c.market.last_trade=100;c.agent.agent_id="COUNTERFACTUAL-SEAT";c.agent.role=AgentRole::Buyer;c.agent.cash=10000;c.agent.inventory=0;c.agent.private_value=120;c.agent.remaining_demand=1;c.information.information_condition="fixed_counterfactual_snapshot";c.information.history_visible=true;c.information.history={{0,100.0,98.0,102.0}};c.information.constraints=ActionConstraints::for_role(AgentRole::Buyer);return c;}
std::string model_key(const agents::ModelIdentity&m){return m.provider+"|"+m.model+"|"+m.version;}
}
std::vector<BalancedSeatAssignment> balanced_assignment_for_seed(const std::vector<agents::ModelIdentity>& models,std::uint64_t seed,std::uint64_t base){
 std::vector<BalancedSeatAssignment> out;if(models.empty())return out;const std::size_t n=models.size();const std::size_t rot=static_cast<std::size_t>((seed-base)%n);
 // Latin/crossover rotation: each model cycles through seat, role, and value schedule across matched seeds.
 for(std::size_t seat=0;seat<n;++seat){const std::size_t mi=(seat+rot)%n;BalancedSeatAssignment a;a.agent_id="LLM-SEAT-"+std::to_string(seat);a.model=models[mi];a.side=seat%2==0?market::Side::Buy:market::Side::Sell;a.private_value_or_cost=a.side==market::Side::Buy?120.0-2.0*seat:80.0+2.0*seat;a.activation_position=(seat+rot)%n;out.push_back(a);}return out;
}
HeterogeneousCampaignReport run_heterogeneous_campaign(const HeterogeneousCampaignSpec& in){
 HeterogeneousCampaignReport r;r.spec=in;r.preflight=preflight_ollama_live_catalog(); if(const char* h=std::getenv("COAGENTICS_EXPECTATIONS_SHA256")) r.research_expectations_sha256=h; if(const char* j=std::getenv("COAGENTICS_EXPECTATIONS_JSON")) r.research_expectations_json=j;std::vector<agents::ModelIdentity> models=r.preflight.available;const bool live=in.use_live_ollama&&r.preflight.ollama_reachable&&models.size()>=2;
 if(in.use_live_ollama && !in.smoke && !live){r.minimum_model_requirement_met=false;r.claim_boundary+=" Live Ollama unavailable; mock fallback forbidden.";return r;}
 if(!in.required_model.empty()){auto it=std::find_if(models.begin(),models.end(),[&](const auto&m){return m.model==in.required_model;});if(it==models.end()){r.claim_boundary+=" Requested model unavailable; no experiment executed.";return r;}std::rotate(models.begin(),it,it+1);}
 if(!live){models.clear();for(std::size_t i=0;i<std::max<std::size_t>(2,in.max_llm_seats);++i)models.push_back({"mock","hetero-model-"+std::to_string(i),"v1","mock"});}
 if(models.size()>in.max_llm_seats) models.resize(in.max_llm_seats);
 if(in.smoke&&models.size()>2) models.resize(2);
 if(in.require_minimum_distinct_models && !in.smoke && models.size()<in.minimum_distinct_models){
   r.minimum_model_requirement_met=false;
   r.distinct_models_used=models.size();
   r.claim_boundary += " Campaign not executed because the minimum distinct-model requirement was not met.";
   return r;
 }
 r.distinct_models_used=models.size();
 r.minimum_model_requirement_met=models.size()>=in.minimum_distinct_models;
 std::vector<ActivationDesign> designs;if(in.run_frozen_snapshot)designs.push_back(ActivationDesign::FrozenSnapshot);if(in.run_sequential_interaction)designs.push_back(ActivationDesign::SequentialInteraction);
 const bool persist=incremental_evidence(in);
 std::filesystem::path run_dir=in.results_dir;
 if(persist){
  std::filesystem::create_directories(run_dir/"cell_evidence");
  std::filesystem::create_directories(run_dir/"cell_observations");
  std::filesystem::create_directories(run_dir/"cell_snapshots");
  write_run_manifest(run_dir,in,designs);
 }
 std::size_t index=0;
 for(std::size_t si=0;si<std::max<std::size_t>(1,in.n_seeds);++si){auto seed=in.base_seed+si;auto asg=balanced_assignment_for_seed(models,seed,in.base_seed);for(auto d:designs)for(std::size_t rep=0;rep<std::max<std::size_t>(1,in.inference_repeats);++rep){
  HeterogeneousCampaignCell c;c.seed=seed;c.repeat=rep;c.activation_design=d;c.assignment=asg;
  if(persist && evidence_completed(run_dir,index)){
   r.cells.push_back(std::move(c));
   ++index;
   continue;
  }
  if(persist && std::filesystem::exists(run_dir/"cell_evidence"/("cell_"+std::to_string(index)+".json")))
   throw std::runtime_error("incomplete cell evidence exists; refusing to overwrite cell_"+std::to_string(index)+".json");
  if(persist) append_progress(run_dir, std::string("{\"status\":\"running\",\"index\":")+std::to_string(index)+",\"seed\":"+std::to_string(seed)+",\"repeat\":"+std::to_string(rep)+",\"design\":\""+design_name(d)+"\"}");
  auto t0=std::chrono::steady_clock::now();
  c.population=run_population_market(make_population(asg,seed,in.rounds,d,live,in.condition=="intervention"?70.0:in.condition=="control"?100.0:0.0));
  double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();
  if(persist){
   persist_completed_cell(run_dir,index,seed,rep,d,c.population,elapsed);
   append_progress(run_dir, std::string("{\"status\":\"completed\",\"index\":")+std::to_string(index)+",\"seed\":"+std::to_string(seed)+",\"repeat\":"+std::to_string(rep)+",\"design\":\""+design_name(d)+"\"}");
  }
  r.cells.push_back(std::move(c));
  ++index;
 }
  if(in.run_counterfactual_substitution){CounterfactualSubstitutionReport cf;cf.seed=seed;cf.fixed_context=fixed_context();for(const auto&m:models)for(std::size_t rep=0;rep<std::max<std::size_t>(1,in.inference_repeats);++rep){auto t=transport_for(m,live,market::Side::Buy,1);LlmAgentAdapter a(m,t,seed,"coagentics-llm-adapter/0.1","","counterfactual:"+std::to_string(seed));CounterfactualModelDecision d;d.model=m;d.repeat=rep;d.turn=a.decide(cf.fixed_context);cf.decisions.push_back(std::move(d));}r.counterfactuals.push_back(std::move(cf));}
 }
 // Aggregate by exact model identity. These are descriptive campaign summaries, not causal claims.
 std::map<std::string, ModelCampaignSummary> summary;
 for(const auto& cell:r.cells){
   for(const auto& a:cell.assignment){
     auto key=model_key(a.model); auto& s=summary[key]; s.model=a.model; ++s.assignment_count;
   }
   for(const auto& a:cell.population.agents){
     auto key=model_key(a.model); auto it=summary.find(key);
     if(it==summary.end()) continue;
     auto& s=it->second;
     s.turn_count += a.turns;
     s.accepted_count += a.fills;
     s.rejected_count += a.market_rejections + a.action_validation_failures;
     for(const auto& t:a.llm_turns){
       if(t.parsed_action){
         if(t.parsed_action->action==ActionType::Buy) ++s.buy_count;
         else if(t.parsed_action->action==ActionType::Sell) ++s.sell_count;
         else ++s.hold_or_abstain_count;
       } else {
         ++s.hold_or_abstain_count;
       }
     }
   }
 }
 for(auto& kv:summary) r.model_summaries.push_back(kv.second);
 std::filesystem::create_directories(in.results_dir);
 // Retain raw inference evidence, not just summary statistics. Never write a successful
 // protocol artifact without a completed cell and model response.
 if(!in.protocol_id.empty()){
  for(std::size_t i=0;i<r.cells.size();++i){const auto& c=r.cells[i];
   std::ostringstream e;e<<"{\"protocol_id\":\""<<esc(in.protocol_id)<<"\",\"protocol_sha256\":\""<<esc(in.protocol_sha256)
    <<"\",\"scenario_id\":\""<<esc(in.scenario_id)<<"\",\"trial\":\""<<esc(in.trial)
    <<"\",\"condition\":\""<<esc(in.condition)<<"\",\"requested_model\":\""<<esc(in.required_model)
    <<"\",\"seed\":"<<c.seed<<",\"repeat\":"<<c.repeat<<",\"fundamental\":"<<c.population.spec.market.fundamental<<",\"public_signal\":"<<c.population.spec.public_signal<<",\"turns\":[";
   bool first=true;for(const auto&a:c.population.agents)for(const auto&t:a.llm_turns){if(!first)e<<",";first=false;
    e<<"{\"model\":\""<<esc(a.model.model)<<"\",\"agent_id\":\""<<esc(a.agent_id)
     <<"\",\"canonical_request\":\""<<esc(t.canonical_request)<<"\",\"raw_response\":\""<<esc(t.raw_provider_response)
     <<"\",\"parse_ok\":"<<(t.parse.success?"true":"false")<<"}";}
   e<<"],\"trades\":[";
   for(std::size_t ti=0;ti<c.population.trades.size();++ti){const auto&t=c.population.trades[ti];if(ti)e<<",";
    e<<"{\"time\":"<<t.time<<",\"asset\":\""<<esc(t.asset)<<"\",\"quantity\":"<<t.quantity
     <<",\"price\":"<<t.price<<",\"buyer\":\""<<esc(t.buyer)<<"\",\"seller\":\""<<esc(t.seller)<<"\"}";
   }
   e<<"],\"market_metrics\":{\"allocative_efficiency\":"<<c.population.metrics.allocative_efficiency
    <<",\"trade_count\":"<<c.population.trades.size()<<"}}";
   const auto path=std::filesystem::path(in.results_dir)/("protocol_cell_"+std::to_string(i)+".json");
   std::ofstream file(path);if(!file)throw std::runtime_error("unable to open protocol evidence file");
   file<<e.str();file.flush();if(!file)throw std::runtime_error("failed to persist protocol evidence");
  }
 }
 std::ofstream(std::filesystem::path(in.results_dir)/"campaign.json")<<heterogeneous_campaign_json(r);return r;
}
std::string heterogeneous_campaign_json(const HeterogeneousCampaignReport&r){
 std::ostringstream o;
 o<<std::boolalpha<<"{\"campaign_id\":\""<<esc(r.spec.campaign_id)
  <<"\",\"protocol_id\":\""<<esc(r.spec.protocol_id)<<"\",\"protocol_sha256\":\""<<esc(r.spec.protocol_sha256)<<"\",\"condition\":\""<<esc(r.spec.condition)<<"\",\"scenario_id\":\""<<esc(r.spec.scenario_id)<<"\",\"trial\":\""<<esc(r.spec.trial)<<"\",\"base_seed\":"<<r.spec.base_seed<<",\"n_seeds\":"<<r.spec.n_seeds
  <<",\"inference_repeats\":"<<r.spec.inference_repeats
  <<",\"distinct_models_used\":"<<r.distinct_models_used
  <<",\"minimum_model_requirement_met\":"<<(r.minimum_model_requirement_met?"true":"false")
  <<",\"balanced_role_value_rotation\":"<<(r.balanced_role_value_rotation?"true":"false")
  <<",\"research_expectations\":{\"sha256\":\""<<esc(r.research_expectations_sha256)<<"\",\"document\":"<<(r.research_expectations_json.empty()?"null":r.research_expectations_json)<<"},\"cells\":[";
 for(size_t i=0;i<r.cells.size();++i){if(i)o<<",";auto&c=r.cells[i];o<<"{\"seed\":"<<c.seed<<",\"repeat\":"<<c.repeat<<",\"activation_design\":\""<<design_name(c.activation_design)<<"\",\"efficiency\":"<<c.population.metrics.allocative_efficiency<<",\"trades\":"<<c.population.trades.size()<<",\"assignment\":[";for(size_t j=0;j<c.assignment.size();++j){if(j)o<<",";auto&a=c.assignment[j];o<<"{\"seat\":\""<<esc(a.agent_id)<<"\",\"model\":\""<<esc(a.model.model)<<"\",\"role\":\""<<(a.side==market::Side::Buy?"buyer":"seller")<<"\",\"private_value_or_cost\":"<<a.private_value_or_cost<<"}";}o<<"]}";}
 o<<"],\"counterfactuals\":[";
 for(size_t i=0;i<r.counterfactuals.size();++i){if(i)o<<",";auto&cf=r.counterfactuals[i];o<<"{\"seed\":"<<cf.seed<<",\"decisions\":[";for(size_t j=0;j<cf.decisions.size();++j){if(j)o<<",";auto&d=cf.decisions[j];o<<"{\"model\":\""<<esc(d.model.model)<<"\",\"repeat\":"<<d.repeat<<",\"canonical_request\":\""<<esc(d.turn.canonical_request)<<"\",\"raw_response\":\""<<esc(d.turn.raw_provider_response)<<"\",\"parse_ok\":"<<(d.turn.parse.success?"true":"false")<<"}";}o<<"]}";}
 o<<"],\"model_summaries\":[";
 for(size_t i=0;i<r.model_summaries.size();++i){if(i)o<<",";auto&s=r.model_summaries[i];o<<"{\"model\":\""<<esc(s.model.model)<<"\",\"provider\":\""<<esc(s.model.provider)<<"\",\"version\":\""<<esc(s.model.version)<<"\",\"assignments\":"<<s.assignment_count<<",\"turns\":"<<s.turn_count<<",\"buys\":"<<s.buy_count<<",\"sells\":"<<s.sell_count<<",\"hold_or_abstain\":"<<s.hold_or_abstain_count<<",\"accepted\":"<<s.accepted_count<<",\"rejected\":"<<s.rejected_count<<"}";}
 o<<"],\"claim_boundary\":\""<<esc(r.claim_boundary)<<"\"}";
 return o.str();
}
}
