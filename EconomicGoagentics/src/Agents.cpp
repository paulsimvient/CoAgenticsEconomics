#include "coagentics/agents/Agents.hpp"
#include <algorithm>
namespace coagentics::agents {
coagentics::market::Bid HeuristicAgent::act(const Observation&o){
 double perceived=reservation_+o.public_signal; double p=side_==coagentics::market::Side::Buy?perceived*(1.0-shade_):perceived*(1.0+shade_);
 return {o.time,id_,o.asset,qty_,std::max(0.0,p),side_};
}
coagentics::market::Bid ZeroIntelligenceAgent::act(const Observation&o){
 double perceived=reservation_+o.public_signal; if(o.last_trade>0) perceived += 0.20*(o.last_trade-o.fundamental); double lo,hi;
 if(side_==coagentics::market::Side::Buy){lo=std::max(0.0,perceived-span_);hi=std::max(lo,perceived);} else {lo=std::max(0.0,perceived);hi=perceived+span_;}
 std::uniform_real_distribution<double>d(lo,hi); return {o.time,id_,o.asset,qty_,d(rng_),side_};
}
}

namespace coagentics::agents {
coagentics::market::Bid SignalResponsiveAgent::act(const Observation&o){double p=reservation_+gain_*o.public_signal;return{o.time,id_,o.asset,1,std::max(0.0,p),side_};}
coagentics::market::Bid RiskSensitiveAgent::act(const Observation&o){double uncertainty=std::abs(o.public_signal);double p=reservation_+o.public_signal;if(side_==coagentics::market::Side::Buy)p-=aversion_*uncertainty;else p+=aversion_*uncertainty;return{o.time,id_,o.asset,1,std::max(0.0,p),side_};}
coagentics::market::Bid PeerResponsiveAgent::act(const Observation&o){double peer=o.last_trade>0?o.last_trade:o.fundamental;double p=reservation_+peer_gain_*(peer-o.fundamental);return{o.time,id_,o.asset,1,std::max(0.0,p),side_};}
coagentics::market::Bid AdaptiveAgent::act(const Observation&o){double observed=o.last_trade>0?o.last_trade-o.fundamental:o.public_signal;belief_=(1.0-learning_)*belief_+learning_*observed;double p=reservation_+o.public_signal+belief_;return{o.time,id_,o.asset,1,std::max(0.0,p),side_};}
}
