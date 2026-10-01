#include "coagentics/analysis/MarketTemporalValidation.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){using namespace coagentics::analysis;MarketTemporalConfig c;c.training_trials=5;c.heldout_trials=8;auto a=validate_market_temporal(c),b=validate_market_temporal(c);assert(a.rows.size()==5);assert(a.heldout_recovery==b.heldout_recovery);assert(a.null_false_discovery==b.null_false_discovery);assert(a.heldout_recovery>=0&&a.heldout_recovery<=1);assert(market_temporal_json(a).find("heldout_recovery")!=std::string::npos);auto blind=market_temporal_trajectory("information-blind",3333);for(auto x:blind)assert(std::abs(x)<1e-9);std::cout<<"v19 full CDA temporal PASS recovery="<<a.heldout_recovery<<" null="<<a.null_false_discovery<<"\n";}
