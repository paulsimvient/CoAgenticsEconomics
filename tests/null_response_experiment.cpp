#include "coagentics/analysis/Behavior.hpp"
#include "coagentics/experiment/Runner.hpp"
#include <cassert>
int main(){using namespace coagentics;experiment::ReferenceAuctionConfig c;c.rounds=40;experiment::InformationTimeline x;auto a=experiment::run_reference_auction(91,c,x,false);auto b=experiment::run_reference_auction(91,c,x,false);auto r=analysis::compare_paired_bids(a.bids,b.bids,10,{});assert(r.targeted_mean_abs_shift==0);assert(r.non_target_mean_abs_shift==0);}
