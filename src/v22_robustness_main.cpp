#include "coagentics/analysis/Robustness.hpp"
#include <iostream>
int main(){auto r=coagentics::analysis::run_robustness_audit();std::cout<<coagentics::analysis::robustness_markdown(r);return r.all_passed?0:1;}
