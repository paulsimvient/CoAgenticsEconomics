#include "coagentics/analysis/MarketTemporalValidation.hpp"
#include <iostream>
int main(){auto r=coagentics::analysis::validate_market_temporal();std::cout<<coagentics::analysis::market_temporal_markdown(r);}
