#include "coagentics/analysis/HumanComparison.hpp"
#include <iostream>
int main(){auto report=coagentics::analysis::compare_human_behavior(coagentics::analysis::empirical_market_reference_catalog(),{});std::cout<<coagentics::analysis::human_comparison_markdown(report);}
