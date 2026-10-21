#include "coagentics/analysis/Behavior.hpp"
#include <cassert>
int main(){using namespace coagentics::market;std::vector<Bid> c{{0,"A","X",1,100,Side::Buy},{1,"A","X",1,100,Side::Buy},{2,"A","X",1,100,Side::Buy},{3,"A","X",1,100,Side::Buy}};std::vector<Bid> t{{0,"A","X",1,90,Side::Buy},{1,"A","X",1,92,Side::Buy},{2,"A","X",1,99.8,Side::Buy},{3,"A","X",1,100,Side::Buy}};auto r=coagentics::analysis::compare_paired_bids(c,t,0,{"A"},0.5,0.5);assert(r.agents[0].reaction_latency==0);assert(r.agents[0].recovery_latency==2);assert(r.agents[0].peak_abs_shift==10);}
