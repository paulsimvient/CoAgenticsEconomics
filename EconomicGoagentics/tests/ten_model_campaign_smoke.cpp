#include "coagentics/experiment/HeterogeneousCampaign.hpp"
#include <cassert>
#include <iostream>
using namespace coagentics::experiment;
int main(){
 HeterogeneousCampaignSpec s;
 s.smoke=false; s.use_live_ollama=false; s.n_seeds=3; s.rounds=1; s.max_llm_seats=10;
 s.minimum_distinct_models=10; s.inference_repeats=2;
 s.results_dir="ten_model_campaign_smoke_results";
 auto r=run_heterogeneous_campaign(s);
 assert(r.minimum_model_requirement_met);
 assert(r.distinct_models_used==10);
 assert(r.cells.size()==3*2*2);
 assert(r.counterfactuals.size()==3);
 assert(r.model_summaries.size()==10);
 for(const auto& x:r.model_summaries) assert(x.assignment_count>0 && x.turn_count>0);
 // Each model is observed in the fixed-context substitution test twice per seed.
 for(const auto& cf:r.counterfactuals) assert(cf.decisions.size()==20);
 std::cout<<"ten_model_campaign_smoke PASS models="<<r.distinct_models_used<<" cells="<<r.cells.size()<<" summaries="<<r.model_summaries.size()<<"\n";
}
