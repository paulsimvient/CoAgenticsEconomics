#include "coagentics/experiment/LlmBehavioralCampaign.hpp"
#include <cassert>
#include <iostream>
using namespace coagentics::experiment;
int main(){
    LlmBehavioralCampaignSpec s; s.use_live_ollama=false; s.smoke=true; s.minimum_distinct_models=2; s.paired_trials_per_probe=20;
    auto r=run_llm_behavioral_campaign(s);
    assert(r.campaign_ready); assert(r.models.size()==2); assert(r.minimum_model_requirement_met);
    for(const auto&m:r.models){
        assert(m.valid_pairs==60); assert(m.invalid_pairs==0); assert(m.profile.sufficient_evidence);
        for(const auto&d:m.profile.dimensions) assert(d.status==coagentics::analysis::ClaimStatus::Supports);
    }
    std::cout<<"llm_behavioral_campaign_smoke PASS\n";
}
