#include "coagentics/analysis/MarketFidelity.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
int main(){using namespace coagentics::analysis;auto a=run_gode_sunder_fidelity(26026,50),b=run_gode_sunder_fidelity(26026,50);assert(a.version=="v12");assert(a.cells.size()==5);assert(a.protocol_structurally_aligned);assert(!a.numerical_replication_claimed);assert(a.audit.single_unit_quotes&&a.audit.improvement_rule&&a.audit.crossing_executes&&a.audit.earlier_quote_price&&a.audit.cancel_quotes_after_trade&&a.audit.sequential_marginal_units&&a.audit.zi_c_budget_constraint);for(size_t i=0;i<a.cells.size();++i)assert(a.cells[i].simulated_mean_efficiency==b.cells[i].simulated_mean_efficiency);std::ofstream("v12_market_fidelity_report.md")<<market_fidelity_markdown(a);std::ofstream("v12_market_fidelity_report.json")<<market_fidelity_json(a);std::cout<<market_fidelity_json(a)<<"\n";}
