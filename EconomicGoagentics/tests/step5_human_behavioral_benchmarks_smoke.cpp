#include "coagentics/analysis/HumanComparison.hpp"
#include "coagentics/analysis/HumanReference.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace coagentics::analysis;
int main(){
 auto b=empirical_human_behavioral_benchmarks();
 assert(b.size()>=3);
 assert(b[0].source_id=="doi:10.1111/iere.12630");
 assert(b[0].observations>=80000);
 auto refs=empirical_market_reference_catalog();
 std::vector<ModelObservation> obs={
   {"initial_buyer_seller_aggressiveness","private-information continuous double auctions","live","r1",0.25,EvidenceOrigin::LiveProvider},
   {"initial_price_relative_to_equilibrium","private-information continuous double auctions","live","r1",-1.5,EvidenceOrigin::LiveProvider},
   {"price_convergence_over_periods","private-information continuous double auctions","live","r1",3.0,EvidenceOrigin::LiveProvider}
 };
 auto r=compare_human_behavior(refs,obs);
 assert(r.behavioral_benchmarks.size()==3);
 assert(r.behavioral_benchmarks[0].status=="CONSISTENT_WITH_REPORTED_PATTERN");
 assert(r.behavioral_benchmarks[1].status=="CONSISTENT_WITH_REPORTED_PATTERN");
 assert(r.behavioral_benchmarks[2].status=="CONSISTENT_WITH_REPORTED_PATTERN");
 auto scripted=obs; scripted[0].origin=EvidenceOrigin::ScriptedControl;
 auto rs=compare_human_behavior(refs,scripted);
 assert(rs.behavioral_benchmarks[0].status=="SCRIPTED_CONTROL_ONLY");
 std::cout<<"step5 human behavioral benchmarks: PASS\n";
}
