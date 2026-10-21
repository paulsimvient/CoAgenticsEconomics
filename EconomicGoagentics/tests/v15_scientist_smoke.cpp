#include "coagentics/experiment/Scientist.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace coagentics::experiment;
int main(){
 std::vector<Candidate>h={{"blind",0,0,0,.25},{"news",1,0,0,.25},{"peer",0,.6,0,.25},{"mixed",1,.6,0,.25}};
 std::vector<Probe>p={{"news-only",10,0,1},{"peer-only",0,20,1},{"both",10,20,1}};
 ScientistConfig cfg;cfg.noise_sd=.25;cfg.stop_posterior=.95;
 for(auto m:{"information-blind","signal-responsive","peer-responsive"}){
  AgentProbeDomain d(m);auto r=run_scientist(d,h,p,26026,cfg);assert(r.status.find("SEPARATED")!=std::string::npos);assert(!r.steps.empty());assert(!r.evidence.records().empty());
  std::string expected=std::string(m)=="information-blind"?"blind":std::string(m)=="signal-responsive"?"news":"peer";assert(r.leading_hypothesis==expected);
  auto repeat=run_scientist(d,h,p,26026,cfg);assert(scientist_markdown(r)==scientist_markdown(repeat));
 }
 std::vector<Candidate>identical={{"a",0,0,0,.5},{"b",0,0,0,.5}};AgentProbeDomain blind("information-blind");auto unresolved=run_scientist(blind,identical,p,1,cfg);assert(unresolved.status.find("UNRESOLVED")!=std::string::npos);assert(unresolved.steps.empty());
 std::cout<<"v15 scientist: three mechanism recoveries, deterministic replay, unresolved null, evidence provenance PASS\n";
}
