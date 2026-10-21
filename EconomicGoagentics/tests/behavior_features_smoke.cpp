#include "coagentics/analysis/Behavior.hpp"
#include <cassert>
#include <cmath>
using namespace coagentics;
int main(){ std::vector<market::Bid>b={{1,"a","A",1,110,market::Side::Buy},{2,"a","A",1,90,market::Side::Buy}}; std::vector<market::Trade>t={{2,"A",1,95,"a","s"}}; auto x=analysis::characterize(b,t,{{"A",100}}); assert(x["a"].bids==2);assert(x["a"].trades==1);assert(std::abs(x["a"].mean_bid_price-100)<1e-9);assert(std::abs(x["a"].price_aggressiveness-.1)<1e-9); }
