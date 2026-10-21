#include "coagentics/analysis/Behavior.hpp"
#include <cmath>
#include <algorithm>
#include <set>
namespace coagentics::analysis {
std::map<std::string,AgentBehavior> characterize(const std::vector<coagentics::market::Bid>& bids,const std::vector<coagentics::market::Trade>& trades,const std::map<std::string,double>& refs){
 std::map<std::string,AgentBehavior> out; std::map<std::string,double> bidSum,tradeSum,agg;
 for(const auto&b:bids){auto&x=out[b.agent_id];++x.bids;bidSum[b.agent_id]+=b.price; auto it=refs.find(b.asset);if(it!=refs.end()&&it->second!=0)agg[b.agent_id]+=std::abs(b.price-it->second)/std::abs(it->second);}
 for(const auto&t:trades){for(auto id:{t.buyer,t.seller}){auto&x=out[id];++x.trades;tradeSum[id]+=t.price;}}
 for(auto&[id,x]:out){if(x.bids){x.mean_bid_price=bidSum[id]/x.bids;x.price_aggressiveness=agg[id]/x.bids;}if(x.trades)x.mean_trade_price=tradeSum[id]/x.trades;} return out;
}
}

namespace coagentics::analysis {
static bool contains_id(const std::vector<std::string>& xs,const std::string& id){return std::find(xs.begin(),xs.end(),id)!=xs.end();}
PropagationSummary compare_paired_bids(const std::vector<coagentics::market::Bid>& control,const std::vector<coagentics::market::Bid>& treatment,std::uint64_t t0,const std::vector<std::string>& targeted,double reaction_threshold,double recovery_threshold){
 using coagentics::market::Bid; std::map<std::string,std::map<std::uint64_t,double>> c,t;
 std::map<std::uint64_t,std::vector<double>> cp,tp;
 for(const auto& b:control){c[b.agent_id][b.time]=b.price;cp[b.time].push_back(b.price);} for(const auto& b:treatment){t[b.agent_id][b.time]=b.price;tp[b.time].push_back(b.price);}
 PropagationSummary out;out.intervention_time=t0; std::set<std::string> ids;for(auto&x:c)ids.insert(x.first);for(auto&x:t)ids.insert(x.first);
 double tsum=0,nsum=0;int tn=0,nn=0;
 for(const auto&id:ids){PairedAgentResponse r;r.agent_id=id;r.targeted=contains_id(targeted,id);double sum=0,ab=0,conf=0;int n=0;bool reacted=false,recovered=false;int rt=-1,rec=-1;double peak=0;
  for(const auto&[time,p0]:c[id]){if(time<t0)continue;auto it=t[id].find(time);if(it==t[id].end())continue;double d=it->second-p0;sum+=d;ab+=std::abs(d);peak=std::max(peak,std::abs(d));n++;
   if(!reacted&&std::abs(d)>=reaction_threshold){reacted=true;rt=(int)(time-t0);} else if(reacted&&!recovered&&std::abs(d)<=recovery_threshold){recovered=true;rec=(int)(time-t0);}
   auto mean=[](const std::vector<double>&v){double s=0;for(double x:v)s+=x;return v.empty()?0:s/v.size();};double cm=mean(cp[time]),tm=mean(tp[time]);conf+=(std::abs(it->second-tm)-std::abs(p0-cm));
  }
  r.matched_post_bids=n;if(n){r.mean_bid_shift=sum/n;r.mean_abs_bid_shift=ab/n;r.conformity_change=conf/n;}r.reaction_latency=rt;r.recovery_latency=rec;r.peak_abs_shift=peak;out.agents.push_back(r);if(r.targeted){tsum+=r.mean_abs_bid_shift;tn++;}else{nsum+=r.mean_abs_bid_shift;nn++;}
 }
 out.targeted_mean_abs_shift=tn?tsum/tn:0;out.non_target_mean_abs_shift=nn?nsum/nn:0;out.propagation_ratio=out.targeted_mean_abs_shift>0?out.non_target_mean_abs_shift/out.targeted_mean_abs_shift:0;return out;
}
}
