#include "coagentics/analysis/ScientistCalibration.hpp"
#include <fstream>
#include <iostream>
int main(){auto r=coagentics::analysis::validate_market_scientist();auto md=coagentics::analysis::scientist_calibration_markdown(r);std::cout<<md;std::ofstream("V17_SCIENTIST_CALIBRATION_REPORT.md")<<md;std::ofstream("V17_SCIENTIST_CALIBRATION_REPORT.json")<<coagentics::analysis::scientist_calibration_json(r);return (r.calibration_pass&&r.null_control_pass&&r.recovery_pass)?0:2;}
