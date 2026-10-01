#include "coagentics/experiment/Runner.hpp"
#include <cassert>
#include <iostream>
int main(){coagentics::experiment::ReferenceAuctionConfig c;c.rounds=40;auto xs=coagentics::experiment::run_paired_news_experiment(7000,100,c,-12,0.8);auto s=coagentics::experiment::summarize(xs);assert(s.n==100);std::cout<<"paired news delta="<<s.mean_delta<<" pp CI95 ["<<s.ci95_low<<","<<s.ci95_high<<"]\n";}
