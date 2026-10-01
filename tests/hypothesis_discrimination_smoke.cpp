#include "coagentics/analysis/Hypothesis.hpp"
#include <cassert>
using namespace coagentics::analysis;
int main(){
 PropagationSummary s;s.targeted_mean_abs_shift=12;s.non_target_mean_abs_shift=.6;s.propagation_ratio=.05;
 BehavioralHypothesis direct{"H_DIRECT_INFO","Targeted agent response is primarily attributable to direct information exposure",{}};
 Discriminator d{"D_TARGET_ONLY","Targeted exposure produces a material target shift with little peer propagation",5,0,.15};
 auto e=evaluate(direct,d,s);assert(e.direction==EvidenceDirection::Supports);
 BehavioralHypothesis social{"H_SOCIAL_PROP","Response propagates materially through observation of other agents",{}};
 Discriminator p{"D_PROPAGATE","Non-target response is a substantial fraction of target response",5,.30,1e9};
 auto e2=evaluate(social,p,s);assert(e2.direction==EvidenceDirection::Inconclusive || e2.direction==EvidenceDirection::Challenges);
}
