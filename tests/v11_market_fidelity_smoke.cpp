#include "coagentics/analysis/MarketFidelity.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
int main(){
 using namespace coagentics::analysis;
 auto z=gode_sunder_zi_c_target(); auto h=gode_sunder_human_target();
 assert(z.market_mean_efficiency.size()==5 && h.market_mean_efficiency.size()==5);
 assert(z.across_market_mean>98.6 && z.across_market_mean<98.8);
 assert(h.across_market_mean>97.6 && h.across_market_mean<97.7);
 auto a=run_gode_sunder_fidelity(26026,50); auto b=run_gode_sunder_fidelity(26026,50);
 assert(a.cells.size()==5); assert(a.protocol_structurally_aligned); assert(!a.numerical_replication_claimed);
 for(size_t i=0;i<a.cells.size();++i) assert(a.cells[i].simulated_mean_efficiency==b.cells[i].simulated_mean_efficiency);
 std::ofstream("v11_market_fidelity_report.md")<<market_fidelity_markdown(a);
 std::ofstream("v11_market_fidelity_report.json")<<market_fidelity_json(a);
 std::cout<<market_fidelity_json(a)<<"\n";
}
