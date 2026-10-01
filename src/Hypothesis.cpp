#include "coagentics/analysis/Hypothesis.hpp"
#include <sstream>
namespace coagentics::analysis {
std::string to_string(EvidenceDirection d){switch(d){case EvidenceDirection::Supports:return "supports";case EvidenceDirection::Challenges:return "challenges";default:return "inconclusive";}}
EvidenceRecord evaluate(const BehavioralHypothesis& h,const Discriminator& d,const PropagationSummary& s){
 EvidenceRecord e; e.hypothesis_id=h.id;e.discriminator_id=d.id;e.targeted_mean_abs_shift=s.targeted_mean_abs_shift;e.non_target_mean_abs_shift=s.non_target_mean_abs_shift;e.propagation_ratio=s.propagation_ratio;
 const bool target=s.targeted_mean_abs_shift>=d.targeted_min_shift;
 const bool prop=s.propagation_ratio>=d.propagation_min_ratio && s.propagation_ratio<=d.propagation_max_ratio;
 if(target&&prop)e.direction=EvidenceDirection::Supports;
 else if(!target || s.propagation_ratio>d.propagation_max_ratio)e.direction=EvidenceDirection::Challenges;
 else e.direction=EvidenceDirection::Inconclusive;
 std::ostringstream os;os<<"target_shift="<<s.targeted_mean_abs_shift<<", non_target_shift="<<s.non_target_mean_abs_shift<<", propagation_ratio="<<s.propagation_ratio<<"; preregistered bounds: target >= "<<d.targeted_min_shift<<", propagation in ["<<d.propagation_min_ratio<<", "<<d.propagation_max_ratio<<"]";e.rationale=os.str();return e;
}
}
