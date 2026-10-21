#include "coagentics/analysis/Robustness.hpp"
#include <cassert>
#include <iostream>
int main(){auto r=coagentics::analysis::run_robustness_audit();std::cout<<coagentics::analysis::robustness_markdown(r);assert(r.total==105);assert(r.all_passed);}
