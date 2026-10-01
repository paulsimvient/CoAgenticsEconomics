#include "coagentics/analysis/BlackBox.hpp"
#include <cassert>
#include <iostream>
int main(){auto r=coagentics::analysis::validate_black_box_controls();assert(r.rows.size()==4);assert(r.leakage_test_passed);assert(r.accuracy==1.0);assert(r.null_false_discovery==0);std::cout<<coagentics::analysis::black_box_report(r);}
