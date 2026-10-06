#include "coagentics/experiment/HeterogeneousCampaign.hpp"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
namespace coagentics::experiment {
namespace {
std::string esc(const std::string&s){std::string o;for(char c:s){if(c=='"'||c=='\\')o+='\\';if(c=='\n'){o+="\\n";continue;}o+=c;}return o;}
std::string design_name(ActivationDesign d){return d==ActivationDesign::FrozenSnapshot?"frozen_snapshot":"sequential_interaction";}
std::shared_ptr<agents::ModelTransport> transport_for(const agents::ModelIdentity&m,bool live,market::Side side,int rounds){
 if(live){setenv("COAGENTICS_LLM_PROVIDER","ollama",1);setenv("COAGENTICS_LLM_BASE_URL","http://127.0.0.1:11434/v1",1);setenv("COAGENTICS_LLM_MODEL",m.model.c_str(),1);return make_live_openai_compatible_transport(m);}
 std::vector<std::string> p; const bool buy=side==market::Side::Buy; for(int r=0;r<std::max(1,rounds);++r)
  p.push_back(std::string("{\"action\":\"")+(buy?"BUY":"SELL")+"\",\"asset\":\"ASSET\",\"quantity\":1,\"price\":"+(buy?"105":"95")+",\"time\":"+std::to_string(r)+"}");
 return std::make_shared<RawJsonTransport>(std::move(p));
}
PopulationSpec make_population(const std::vector<BalancedSeatAssignment>& asg,std::uint64_t seed,int rounds,ActivationDesign design,bool live){
 PopulationSpec p; p.seed=seed;p.rounds=rounds;p.activation_design=design;p.market.asset="ASSET";p.market.fundamental=100;p.market.mechanism=market::MechanismKind::ContinuousDoubleAuction;
 for(const auto&a:asg){AgentSlot s;s.agent_id=a.agent_id;s.kind=AgentKind::Llm;s.side=a.side;s.private_value_or_cost=a.private_value_or_cost;s.cash=10000;s.inventory=a.side==market::Side::Sell?1:0;s.model=a.model;s.transport=transport_for(a.model,live,a.side,rounds);p.agents.push_back(std::move(s));}
 // Rotate the preregistered activation order independently of model identity; repeated inference preserves it.
 std::vector<std::pair<std::size_t,std::string>> ordered; for(const auto&a:asg) ordered.push_back({a.activation_position,a.agent_id});
 std::sort(ordered.begin(),ordered.end()); for(const auto&x:ordered)p.activation_order.push_back(x.second);
 return p;
}
DecisionContext fixed_context(){DecisionContext c;c.market.time=0;c.market.asset="ASSET";c.market.fundamental=100;c.market.public_signal=0;c.market.best_bid=98;c.market.best_ask=102;c.market.last_trade=100;c.agent.agent_id="COUNTERFACTUAL-SEAT";c.agent.role=AgentRole::Buyer;c.agent.cash=10000;c.agent.inventory=0;c.agent.private_value=120;c.agent.remaining_demand=1;c.information.information_condition="fixed_counterfactual_snapshot";c.information.history_visible=true;c.information.history={{0,100.0,98.0,102.0}};c.information.constraints=ActionConstraints::for_role(AgentRole::Buyer);return c;}
}
std::vector<BalancedSeatAssignment> balanced_assignment_for_seed(const std::vector<agents::ModelIdentity>& models,std::uint64_t seed,std::uint64_t base){
 std::vector<BalancedSeatAssignment> out;if(models.empty())return out;const std::size_t n=models.size();const std::size_t rot=static_cast<std::size_t>((seed-base)%n);
 // Latin/crossover rotation: each model cycles through seat, role, and value schedule across matched seeds.
 for(std::size_t seat=0;seat<n;++seat){const std::size_t mi=(seat+rot)%n;BalancedSeatAssignment a;a.agent_id="LLM-SEAT-"+std::to_string(seat);a.model=models[mi];a.side=seat%2==0?market::Side::Buy:market::Side::Sell;a.private_value_or_cost=a.side==market::Side::Buy?120.0-2.0*seat:80.0+2.0*seat;a.activation_position=(seat+rot)%n;out.push_back(a);}return out;
}
HeterogeneousCampaignReport run_heterogeneous_campaign(const HeterogeneousCampaignSpec& in){
 HeterogeneousCampaignReport r;r.spec=in;r.preflight=preflight_ollama_live_catalog(); if(const char* h=std::getenv("COAGENTICS_EXPECTATIONS_SHA256")) r.research_expectations_sha256=h; if(const char* j=std::getenv("COAGENTICS_EXPECTATIONS_JSON")) r.research_expectations_json=j;std::vector<agents::ModelIdentity> models=r.preflight.available;const bool live=in.use_live_ollama&&r.preflight.ollama_reachable&&models.size()>=2;
 if(!live){models.clear();for(std::size_t i=0;i<std::max<std::size_t>(2,in.max_llm_seats);++i)models.push_back({"mock","hetero-model-"+std::to_string(i),"v1"});}
 if(models.size()>in.max_llm_seats) models.resize(in.max_llm_seats);
 if(in.smoke&&models.size()>2) models.resize(2);
 std::vector<ActivationDesign> designs;if(in.run_frozen_snapshot)designs.push_back(ActivationDesign::FrozenSnapshot);if(in.run_sequential_interaction)designs.push_back(ActivationDesign::SequentialInteraction);
 for(std::size_t si=0;si<std::max<std::size_t>(1,in.n_seeds);++si){auto seed=in.base_seed+si;auto asg=balanced_assignment_for_seed(models,seed,in.base_seed);for(auto d:designs)for(std::size_t rep=0;rep<std::max<std::size_t>(1,in.inference_repeats);++rep){HeterogeneousCampaignCell c;c.seed=seed;c.repeat=rep;c.activation_design=d;c.assignment=asg;c.population=run_population_market(make_population(asg,seed,in.rounds,d,live));r.cells.push_back(std::move(c));}
  if(in.run_counterfactual_substitution){CounterfactualSubstitutionReport cf;cf.seed=seed;cf.fixed_context=fixed_context();for(const auto&m:models)for(std::size_t rep=0;rep<std::max<std::size_t>(1,in.inference_repeats);++rep){auto t=transport_for(m,live,market::Side::Buy,1);LlmAgentAdapter a(m,t,seed,"coagentics-llm-adapter/0.1","","counterfactual:"+std::to_string(seed));CounterfactualModelDecision d;d.model=m;d.repeat=rep;d.turn=a.decide(cf.fixed_context);cf.decisions.push_back(std::move(d));}r.counterfactuals.push_back(std::move(cf));}
 }
 std::filesystem::create_directories(in.results_dir);std::ofstream(std::filesystem::path(in.results_dir)/"campaign.json")<<heterogeneous_campaign_json(r);return r;
}
std::string heterogeneous_campaign_json(const HeterogeneousCampaignReport&r){std::ostringstream o;o<<std::boolalpha<<"{\"base_seed\":"<<r.spec.base_seed<<",\"n_seeds\":"<<r.spec.n_seeds<<",\"inference_repeats\":"<<r.spec.inference_repeats<<",\"balanced_role_value_rotation\":"<<r.balanced_role_value_rotation<<",\"research_expectations\":{\"sha256\":\""<<esc(r.research_expectations_sha256)<<"\",\"document\":"<<(r.research_expectations_json.empty()?"null":r.research_expectations_json)<<"},\"cells\":[";for(size_t i=0;i<r.cells.size();++i){if(i)o<<",";auto&c=r.cells[i];o<<"{\"seed\":"<<c.seed<<",\"repeat\":"<<c.repeat<<",\"activation_design\":\""<<design_name(c.activation_design)<<"\",\"efficiency\":"<<c.population.metrics.allocative_efficiency<<",\"trades\":"<<c.population.trades.size()<<",\"assignment\":[";for(size_t j=0;j<c.assignment.size();++j){if(j)o<<",";auto&a=c.assignment[j];o<<"{\"seat\":\""<<esc(a.agent_id)<<"\",\"model\":\""<<esc(a.model.model)<<"\",\"role\":\""<<(a.side==market::Side::Buy?"buyer":"seller")<<"\",\"private_value_or_cost\":"<<a.private_value_or_cost<<"}";}o<<"]}";}o<<"],\"counterfactuals\":[";for(size_t i=0;i<r.counterfactuals.size();++i){if(i)o<<",";auto&cf=r.counterfactuals[i];o<<"{\"seed\":"<<cf.seed<<",\"decisions\":[";for(size_t j=0;j<cf.decisions.size();++j){if(j)o<<",";auto&d=cf.decisions[j];o<<"{\"model\":\""<<esc(d.model.model)<<"\",\"repeat\":"<<d.repeat<<",\"canonical_request\":\""<<esc(d.turn.canonical_request)<<"\",\"raw_response\":\""<<esc(d.turn.raw_provider_response)<<"\",\"parse_ok\":"<<d.turn.parse.success<<"}";}o<<"]}";}o<<"],\"claim_boundary\":\""<<esc(r.claim_boundary)<<"\"}";return o.str();}
}
