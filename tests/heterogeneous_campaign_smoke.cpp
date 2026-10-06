#include "coagentics/experiment/HeterogeneousCampaign.hpp"
#include <cassert>
#include <iostream>
using namespace coagentics::experiment;
int main(){
 HeterogeneousCampaignSpec s;s.smoke=true;s.use_live_ollama=false;s.n_seeds=2;s.rounds=1;s.max_llm_seats=2;s.inference_repeats=2;s.results_dir="heterogeneous_campaign_smoke_results";
 auto r=run_heterogeneous_campaign(s);
 assert(r.cells.size()==8); // 2 seeds x 2 designs x 2 repeated inferences
 assert(r.counterfactuals.size()==2);
 assert(r.counterfactuals[0].decisions.size()==4); // 2 models x 2 repeats
 // Crossover: model occupying seat 0 rotates across seeds.
 assert(r.cells[0].assignment[0].model.model!=r.cells[4].assignment[0].model.model);
 // Activation position is also crossed over rather than fixed to model/seat.
 assert(r.cells[0].assignment[0].activation_position!=r.cells[4].assignment[0].activation_position);
 // Frozen and sequential are both explicit campaign cells.
 bool f=false,q=false;for(auto&c:r.cells){f|=c.activation_design==ActivationDesign::FrozenSnapshot;q|=c.activation_design==ActivationDesign::SequentialInteraction;}assert(f&&q);
 // Counterfactual substitution must hold the exact canonical decision payload constant across models.
 const auto& ds=r.counterfactuals[0].decisions;assert(!ds.empty());for(const auto&d:ds)assert(d.turn.canonical_request==ds[0].turn.canonical_request);
 std::cout<<heterogeneous_campaign_json(r)<<"\n";
}
