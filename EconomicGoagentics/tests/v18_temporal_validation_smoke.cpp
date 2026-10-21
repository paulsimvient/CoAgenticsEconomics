#include "coagentics/analysis/TemporalValidation.hpp"
#include <cassert>
#include <iostream>
int main(){using namespace coagentics::analysis;TemporalConfig c;c.train_trials=8;c.heldout_trials=12;auto a=validate_temporal_scientist(c),b=validate_temporal_scientist(c);assert(a.mechanisms.size()==5);assert(a.recovery==b.recovery);assert(a.null_false_discovery==b.null_false_discovery);assert(a.recovery_gate);assert(a.null_gate);assert(temporal_json(a).find("heldout_recovery")!=std::string::npos);std::cout<<"v18 temporal validation PASS recovery="<<a.recovery<<" null="<<a.null_false_discovery<<"\n";}
