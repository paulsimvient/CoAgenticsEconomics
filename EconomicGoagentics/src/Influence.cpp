#include "coagentics/analysis/Influence.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <stdexcept>
namespace coagentics::analysis {
InfluenceEvidence analyze_communication(const std::vector<CommunicationTrial>& control, const std::vector<CommunicationTrial>& treatment, double tolerance) {
  if(control.empty() || treatment.empty()) throw std::invalid_argument("paired trials required");
  std::map<unsigned,CommunicationTrial> by_seed;
  for(const auto& c:control) { if(!by_seed.emplace(c.seed,c).second) throw std::invalid_argument("duplicate control seed"); }
  InfluenceEvidence out; double visible_sum=0, conflict_sum=0; unsigned conflict_n=0;
  for(const auto& t:treatment) {
    auto it=by_seed.find(t.seed); if(it==by_seed.end()) throw std::invalid_argument("unmatched seed");
    const auto& c=it->second;
    if(c.message_visible) throw std::invalid_argument("control must hide message");
    if(!t.message_visible) throw std::invalid_argument("treatment must expose message");
    visible_sum+=(t.receiver_after-t.receiver_before)-(c.receiver_after-c.receiver_before);
    if(t.incentive_conflict){ conflict_sum+=(t.receiver_after-t.receiver_before)-(c.receiver_after-c.receiver_before); ++conflict_n; }
    if(t.private_signal && std::abs(t.statement-t.sender_belief)>tolerance) ++out.observed_misrepresentations;
    ++out.paired_trials;
  }
  if(out.paired_trials!=control.size()) throw std::invalid_argument("unmatched control trials");
  out.visibility_effect=visible_sum/out.paired_trials;
  out.conflict_effect=conflict_n?conflict_sum/conflict_n:0;
  out.intent_identifiable=false; // Statements and actions alone cannot establish intent.
  return out;
}
PropagationResult propagate(unsigned agents,const std::vector<NetworkEdge>& edges,unsigned origin,double impulse,unsigned steps,double retention){
  if(!agents||origin>=agents||retention<0||retention>1) throw std::invalid_argument("invalid network configuration");
  for(auto e:edges) if(e.from>=agents||e.to>=agents||e.weight<0||e.weight>1) throw std::invalid_argument("invalid edge");
  std::vector<double> state(agents,0); state[origin]=impulse;
  for(unsigned step=0;step<steps;++step){
    auto next=state; for(auto& v:next) v*=retention;
    for(const auto& e:edges) next[e.to]+=state[e.from]*e.weight;
    state=std::move(next);
  }
  PropagationResult r; r.responses=std::move(state);
  for(auto v:r.responses){if(std::abs(v)>1e-10)++r.reach; r.mean_response+=v;}
  r.reach/=agents; r.mean_response/=agents; return r;
}
}
