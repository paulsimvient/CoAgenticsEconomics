#include "coagentics/analysis/BehavioralIdentification.hpp"
#include <cassert>
#include <iostream>
#include <string>
#include <vector>
using namespace coagentics;
int main(){
  using experiment::Candidate; using experiment::Probe;
  std::vector<Candidate> candidates{{"blind",0,0,0,.2},{"news",1,0,0,.2},{"risk",.75,0,0,.2},{"peer",0,.6,0,.2}};
  std::vector<Probe> dev{{"news",8,0,1},{"peer",0,8,1},{"repeat",8,0,4},{"mixed",8,8,4}};
  std::vector<Probe> held{{"held-news",5,0,1},{"held-peer",0,5,1},{"held-repeat",5,0,3},{"held-mixed",5,5,2}};
  auto r=analysis::identify_and_validate("signal-responsive",candidates,dev,held);
  assert(r.explanation_frozen);
  assert(!r.frozen_hypothesis.empty());
  assert(r.heldout_trials==80);
  assert(r.heldout_winner_matches_frozen);
  assert(r.heldout_separates);
  std::cout<<"behavioral_identification_smoke PASS\n";
  std::cout<<analysis::behavioral_identification_markdown(r);
}
