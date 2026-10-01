#include "coagentics/analysis/Hypothesis.hpp"
#include <cassert>
using namespace coagentics::analysis;
int main(){
 BehavioralHypothesis h1{"H_COMMON_SIGNAL","Co-movement is explained by common direct exposure",{}};
 BehavioralHypothesis h2{"H_PEER_PROPAGATION","Co-movement persists when only one agent is exposed",{}};
 Discriminator common{"D_PRIVATE_EXPOSURE","Private exposure should not create strong non-target movement if common signal explains co-movement",4,0,.10};
 Discriminator peer{"D_PRIVATE_EXPOSURE","Private exposure should create measurable non-target movement if peer propagation explains co-movement",4,.25,1e9};
 PropagationSummary s;s.targeted_mean_abs_shift=10;s.non_target_mean_abs_shift=.2;s.propagation_ratio=.02;
 assert(evaluate(h1,common,s).direction==EvidenceDirection::Supports);
 auto p=evaluate(h2,peer,s);assert(p.direction!=EvidenceDirection::Supports);
}
