#include "coagentics/analysis/HumanComparison.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace coagentics::analysis;
int main(){
 auto refs=empirical_market_reference_catalog();
 auto missing=compare_human_behavior(refs,{});
 assert(!missing.darpa_claim_ready&&!missing.live_model_evidence);
 assert(missing.comparisons.size()==5);
 assert(missing.comparisons[0].status=="NO_CONDITION_MATCHED_OBSERVATIONS");
 for(size_t i=1;i<5;++i)assert(missing.comparisons[i].status=="NO_EMPIRICAL_HUMAN_REFERENCE");
 const auto condition=refs.get("allocative_efficiency").condition;
 auto wrong=compare_human_behavior(refs,{{"allocative_efficiency","different market","model", "run1",99.0,EvidenceOrigin::LiveProvider}});
 assert(wrong.comparisons[0].status=="NO_CONDITION_MATCHED_OBSERVATIONS");
 auto scripted=compare_human_behavior(refs,{{"allocative_efficiency",condition,"scripted","run1",99.0,EvidenceOrigin::ScriptedControl}});
 assert(scripted.comparisons[0].status=="SCRIPTED_CONTROL_ONLY");
 assert(!scripted.darpa_claim_ready);
 auto live=compare_human_behavior(refs,{{"allocative_efficiency",condition,"provider-model","run1",99.0,EvidenceOrigin::LiveProvider}});
 assert(live.comparisons[0].status=="DESCRIPTIVE_COMPARISON");
 assert(std::abs(live.comparisons[0].difference-1.38)<1e-9);
 assert(live.comparisons[0].human_units==5);
 assert(!live.darpa_claim_ready); // Other four human behavior domains absent.
 auto mixed=compare_human_behavior(refs,{{"allocative_efficiency",condition,"provider-model","run1",99.0,EvidenceOrigin::LiveProvider},{"allocative_efficiency",condition,"scripted","run2",98.0,EvidenceOrigin::ScriptedControl}});
 assert(mixed.comparisons[0].status=="MIXED_ORIGIN_REJECTED");
 assert(human_comparison_json(missing).find("darpa_claim_ready\":false")!=std::string::npos);
 std::cout<<"v23 human comparison: 6 provenance/condition/coverage gates PASS\n";
}
