#include "coagentics/analysis/ScientistCalibration.hpp"
#include <cassert>
int main(){coagentics::analysis::CalibrationConfig c;c.train_trials=2;c.test_trials=3;c.replicates=4;c.quote_events=700;auto r=coagentics::analysis::validate_market_scientist(c);assert(r.heldout_trials==15);assert(r.mechanisms.size()==5);assert(r.calibrated_noise_sd>0);assert(r.heldout_brier>=0&&r.heldout_brier<=1);assert(r.null_false_discovery_rate>=0&&r.null_false_discovery_rate<=1);}
