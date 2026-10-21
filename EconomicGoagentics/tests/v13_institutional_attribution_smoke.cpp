#include "coagentics/analysis/InstitutionalAttribution.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace coagentics::analysis;
int main(){auto r=run_institutional_attribution(26026,20);assert(r.cells.size()==20);assert(r.institution_floor>90.0);assert(!r.live_llms_used);for(auto&c:r.cells){assert(std::isfinite(c.mean_efficiency));assert(c.mean_efficiency>=0&&c.mean_efficiency<=100);}bool saw_signal=false,saw_peer=false;for(auto&c:r.cells){if(c.policy=="signal-biased"&&c.condition=="public-signal")saw_signal=true;if(c.policy=="peer-anchored")saw_peer=true;}assert(saw_signal&&saw_peer);std::cout<<"cells="<<r.cells.size()<<" institution_floor="<<r.institution_floor<<"\n";}
