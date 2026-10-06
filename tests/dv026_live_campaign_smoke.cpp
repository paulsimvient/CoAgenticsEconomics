#include "coagentics/experiment/Dv026LiveCampaign.hpp"
#include "coagentics/analysis/HumanReference.hpp"
#include <cassert>
#include <iostream>
int main(){
 using namespace coagentics::experiment;
 auto catalog=dv026_ollama_live_catalog();
 assert(catalog.size()>=10);
 for(const auto& m:catalog){
  assert(m.provider=="ollama");
  assert(m.version=="local");
  assert(!m.model.empty());
 }

 auto refs=coagentics::analysis::empirical_market_reference_catalog();
 std::size_t empirical=0;
 for(const auto& k:refs.metrics()) if(refs.get(k).empirical) ++empirical;
 assert(empirical>=2);
 assert(refs.has("allocative_efficiency_high_condition"));
 assert(refs.has("allocative_efficiency_market5_low"));
 assert(refs.has("zi_c_allocative_efficiency"));

 LiveCampaignReport empty;
 empty.spec.n_seeds=20;
 empty.spec.min_models=10;
 empty.layer_a_pass=true;
 empty.human_ref_gate=true;
 auto ready=evaluate_darpa_phase_i_readiness(empty);
 assert(!ready.darpa_claim_ready);
 assert(ready.scope=="not_ready");

 HeterogeneousPopulationSpec hs; hs.smoke=true; hs.use_live_ollama=false; hs.rounds=1;
 auto het=run_heterogeneous_ollama_population(hs);
 assert(het.shared_market);
 assert(het.llm_seats==2);
 assert(het.llm_buyers==1);
 assert(het.llm_sellers==1);
 assert(het.population.agents.size()==2);
 assert(!het.population.trades.empty());
 assert(het.population.agents[0].kind==AgentKind::Llm);
 assert(het.population.agents[1].kind==AgentKind::Llm);
 assert(het.population.agents[0].model.model!=het.population.agents[1].model.model);
 assert(het.activation_design=="sequential_interaction");
 HeterogeneousPopulationSpec snap=hs; snap.activation_design=ActivationDesign::FrozenSnapshot;
 auto het_snap=run_heterogeneous_ollama_population(snap);
 assert(het_snap.activation_design=="frozen_snapshot");

 auto pf=preflight_ollama_live_catalog();
 std::cout<<"catalog="<<catalog.size()
  <<" ollama_reachable="<<pf.ollama_reachable
  <<" available="<<pf.available.size()
  <<" missing="<<pf.missing.size()
  <<" empirical_refs="<<empirical
  <<" hetero_llm_seats="<<het.llm_seats
  <<" readiness_empty=false OK\n";
 return 0;
}
