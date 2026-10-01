#include "coagentics/analysis/Influence.hpp"
#include <cassert>
#include <cmath>
#include <stdexcept>
using namespace coagentics::analysis;
int main(){
 std::vector<CommunicationTrial> c,t;
 for(unsigned i=0;i<20;++i){ c.push_back({i,false,true,false,110,100,100,100}); t.push_back({i,true,true,i%2==0,110,100,100,105}); }
 auto e=analyze_communication(c,t); assert(e.paired_trials==20 && e.observed_misrepresentations==20 && !e.intent_identifiable && std::abs(e.visibility_effect-5)<1e-9);
 bool rejected=false;try{t[0].seed=100;analyze_communication(c,t);}catch(const std::invalid_argument&){rejected=true;} assert(rejected);
 auto chain=propagate(4,{{0,1,0.5},{1,2,0.5},{2,3,0.5}},0,8,3);
 assert(std::abs(chain.responses[3]-1)<1e-9 && std::abs(chain.reach-0.25)<1e-9);
 auto cut=propagate(4,{{0,1,0.5},{2,3,0.5}},0,8,3); assert(cut.responses[3]==0);
}
