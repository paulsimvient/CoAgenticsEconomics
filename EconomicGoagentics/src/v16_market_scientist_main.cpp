#include "coagentics/experiment/MarketScientist.hpp"
#include <iostream>
using namespace coagentics::experiment;
int main(){CdaMarketDomain d("signal-responsive");std::vector<Candidate> c={{"blind",0,0,0,.333},{"news",1,0,0,.334},{"peer",0,.6,0,.333}};std::vector<Probe> p={{"news-only",8,0,1},{"peer-only",0,8,1},{"news-and-peer",8,8,1}};ScientistConfig cfg;cfg.replicates=16;cfg.noise_sd=1.5;cfg.minimum_separation=.1;auto r=run_market_scientist(d,c,p,26026,cfg);std::cout<<market_scientist_markdown(r);}
