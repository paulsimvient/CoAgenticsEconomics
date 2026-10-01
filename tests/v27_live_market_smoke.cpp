#include "coagentics/experiment/MarketScientist.hpp"
#include <cassert>
#include <cmath>
#include <vector>
using namespace coagentics;
int main(){
 std::vector<double> x(12,0),y(12,0);int seen=0;
 auto a=experiment::run_live_network_market(1234,300,x,[&](const experiment::DecisionTrace&t,std::vector<double>&v){
  assert(t.event==seen++);if(t.event==75){v[0]=10;v[1]=8;v[2]=6;}if(t.event==175){v[2]=0;}
 });
 assert(seen==300&&a.control.size()==300&&a.treatment.size()==300);
 int exposed=0;for(auto&t:a.treatment)if(t.actor==2&&t.peer_visible>0)++exposed;assert(exposed>0);
 auto b=experiment::run_live_network_market(1234,300,y,[&](const experiment::DecisionTrace&t,std::vector<double>&v){if(t.event==75){v[0]=10;v[1]=8;v[2]=6;}if(t.event==175)v[2]=0;});
 assert(a.outcomes.treatment.trades==b.outcomes.treatment.trades);
 for(size_t i=0;i<a.treatment.size();++i)assert(a.treatment[i].submitted_price==b.treatment[i].submitted_price);
 // Unintervened run must equal its matched control, preserving the original tape.
 std::vector<double> z(12,0);auto n=experiment::run_live_network_market(1234,300,z,[](auto&,auto&){});
 for(size_t i=0;i<n.control.size();++i)assert(n.control[i].submitted_price==n.treatment[i].submitted_price);
}
