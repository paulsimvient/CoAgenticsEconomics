#include "coagentics/analysis/Behavior.hpp"
#include "coagentics/experiment/Runner.hpp"
#include <cassert>
#include <iostream>
int main(){using namespace coagentics;experiment::ReferenceAuctionConfig c;c.rounds=50;experiment::InformationTimeline ctl,trt;trt.add({0,"targeted-negative","ASSET",-18,1.0,std::string("B0"),"controlled misleading signal"});auto a=experiment::run_reference_auction(8801,c,ctl,false);auto b=experiment::run_reference_auction(8801,c,trt,false);auto x=analysis::compare_paired_bids(a.bids,b.bids,0,{"B0"});assert(!x.agents.empty());bool found=false;for(auto&r:x.agents)if(r.agent_id=="B0"){found=true;assert(r.targeted);assert(r.mean_abs_bid_shift>0);assert(r.reaction_latency>=0);}assert(found);std::cout<<"target shift="<<x.targeted_mean_abs_shift<<" non-target="<<x.non_target_mean_abs_shift<<" propagation="<<x.propagation_ratio<<"\n";}
