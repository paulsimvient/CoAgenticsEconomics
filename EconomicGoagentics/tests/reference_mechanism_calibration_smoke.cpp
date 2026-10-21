#include "coagentics/analysis/ReferenceMechanismCalibration.hpp"
#include <cassert>
#include <iostream>
int main(){ auto r=coagentics::analysis::calibrate_reference_mechanisms(91026,20); std::cout<<"reference mechanism recovery="<<r.recovery_rate<<" null_fp="<<r.null_false_positive_rate<<"\n"; assert(r.pass); assert(r.correct==r.trials); return 0; }
