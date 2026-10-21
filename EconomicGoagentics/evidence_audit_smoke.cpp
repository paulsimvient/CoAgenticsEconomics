#include "coagentics/analysis/EvidenceAudit.hpp"
#include "coagentics/experiment/LlmBehavioralCampaign.hpp"
#include <cassert>
#include <iostream>
#include <string>

using namespace coagentics;
int main(){
    experiment::LlmBehavioralCampaignSpec s;
    s.use_live_ollama=false; s.smoke=true; s.minimum_distinct_models=2;
    s.paired_trials_per_probe=4; s.results_dir="results/section14_evidence_audit";
    auto r=experiment::run_llm_behavioral_campaign(s);
    auto a=analysis::audit_llm_behavioral_campaign(r);
    assert(a.valid);
    assert(a.models_checked==2);
    assert(a.evidence_records==24);
    assert(a.complete_records==24);
    const auto jsonl=analysis::evidence_jsonl(r);
    const auto md=analysis::evidence_index_markdown(r);
    assert(jsonl.find("control_raw_response")!=std::string::npos);
    assert(jsonl.find("treatment_raw_response")!=std::string::npos);
    assert(jsonl.find("control_observation_fingerprint")!=std::string::npos);
    assert(md.find("Evidence")!=std::string::npos);
    std::cout<<"evidence_audit_smoke PASS records="<<a.evidence_records<<"\n";
}
