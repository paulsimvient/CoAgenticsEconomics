#include "coagentics/experiment/MarketScientist.hpp"
#include <stdexcept>
#define assert(x) do { if(!(x)) throw std::runtime_error("v25 check failed: " #x); } while(0)
#include <cmath>
#include <iostream>
using namespace coagentics;
int main(){
 using analysis::NetworkEdge;
 std::vector<NetworkEdge> chain{{0,1,0.8},{1,2,0.75}};
 auto full=experiment::run_network_market(chain,0,10,2,424242,1800);
 auto cut=experiment::run_network_market({{0,1,0.8}},0,10,2,424242,1800);
 auto null=experiment::run_network_market(chain,0,0,2,424242,1800);
 assert(full.exposure.responses[2]>0);
 assert(cut.exposure.responses[2]==0);
 assert(full.market.control.size()==full.market.treatment.size());
 assert(full.market.control.size()==1800);
 assert(full.matched_attempts[2]>0);
 // Direction of the quote response depends on side/book state; influence is a non-zero matched shift.
 assert(std::abs(full.attempted_quote_shift[2])>1e-9);
 assert(std::abs(cut.attempted_quote_shift[2])<1e-9);
 assert(std::abs(null.attempted_quote_shift[2])<1e-9);
 auto replay=experiment::run_network_market(chain,0,10,2,424242,1800);
 assert(replay.attempted_quote_shift==full.attempted_quote_shift);
 assert(replay.market.outcomes.control.efficiency==full.market.outcomes.control.efficiency);
 bool rejected=false;
 try{(void)experiment::run_network_market({{0,7,1}},0,10,2,424242);}catch(const std::invalid_argument&){rejected=true;}
 assert(rejected);
 std::cout<<"v25 network/full-market paired experiment passed; node 2 shift "<<full.attempted_quote_shift[2]<<"; cut "<<cut.attempted_quote_shift[2]<<"\n";
}
