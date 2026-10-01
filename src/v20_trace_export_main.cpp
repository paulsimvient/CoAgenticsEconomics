#include "coagentics/experiment/MarketScientist.hpp"
#include <iomanip>
#include <iostream>
#include <string>
namespace {
const char* disposition(coagentics::experiment::QuoteDisposition d){using Q=coagentics::experiment::QuoteDisposition;switch(d){case Q::exhausted:return "exhausted";case Q::no_legal_quote:return "no_legal_quote";case Q::rejected_standing:return "rejected_standing";case Q::accepted:return "accepted";case Q::executed:return "executed";}return "unknown";}
void dump(const std::vector<coagentics::experiment::DecisionTrace>&v,const char* arm,std::uint64_t seed){for(const auto&x:v){std::cout<<"{\"seed\":"<<seed<<",\"arm\":\""<<arm<<"\",\"event\":"<<x.event<<",\"actor\":"<<x.actor<<",\"buyer\":"<<std::boolalpha<<x.buyer<<",\"marginal_unit\":"<<x.marginal_unit<<",\"private_limit\":"<<x.private_limit<<",\"standing_bid\":"<<x.standing_bid<<",\"standing_ask\":"<<x.standing_ask<<",\"random_draw\":"<<x.draw<<",\"news_visible\":"<<x.news_visible<<",\"peer_visible\":"<<x.peer_visible<<",\"legal\":"<<x.legal<<",\"attempted\":"<<x.attempted<<",\"desired_price\":"<<x.desired_price<<",\"submitted_price\":"<<x.submitted_price<<",\"disposition\":\""<<disposition(x.disposition)<<"\",\"transaction_price\":"<<x.transaction_price<<",\"counterparty\":"<<x.counterparty<<"}\n";}}
}
int main(){constexpr std::uint64_t seed=919000;coagentics::experiment::CdaMarketDomain d("signal-responsive",150);auto x=d.run_traced({"news-pulse",8,0,1},seed);std::cout<<std::setprecision(12);dump(x.control,"control",seed);dump(x.treatment,"treatment",seed);}
