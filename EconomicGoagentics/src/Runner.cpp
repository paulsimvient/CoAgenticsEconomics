#include "coagentics/experiment/Runner.hpp"
#include <algorithm>
#include <numeric>
#include <random>
namespace coagentics::experiment {
using namespace coagentics;
AuctionOutcome run_reference_auction(std::uint64_t seed,const ReferenceAuctionConfig& c,const InformationTimeline& timeline,bool heuristic){
 market::MarketConfig mc; mc.fundamental_value["ASSET"]=c.fundamental; mc.total_supply["ASSET"]=c.sellers;
 market::Market m(mc); std::vector<std::unique_ptr<agents::AgentPolicy>> ps;
 for(int i=0;i<c.buyers;i++){std::string id="B"+std::to_string(i); double v=c.fundamental+c.value_step*(c.buyers-i); mc.private_values.buy_values[id+"\nASSET"]={v};}
 for(int i=0;i<c.sellers;i++){std::string id="S"+std::to_string(i); double cost=c.fundamental-c.value_step*(c.sellers-i); mc.private_values.sell_costs[id+"\nASSET"]={cost};}
 // Recreate after evaluator truth has been populated.
 m=market::Market(mc);
 for(int i=0;i<c.buyers;i++){std::string id="B"+std::to_string(i);double v=mc.private_values.buy_values[id+"\nASSET"][0];m.add_account(id,{c.starting_cash,{}}); if(heuristic)ps.push_back(std::make_unique<agents::HeuristicAgent>(id,v,market::Side::Buy));else ps.push_back(std::make_unique<agents::ZeroIntelligenceAgent>(id,v,market::Side::Buy,seed*1009+i));}
 for(int i=0;i<c.sellers;i++){std::string id="S"+std::to_string(i);double v=mc.private_values.sell_costs[id+"\nASSET"][0];m.add_account(id,{0,{{"ASSET",1}}}); if(heuristic)ps.push_back(std::make_unique<agents::HeuristicAgent>(id,v,market::Side::Sell));else ps.push_back(std::make_unique<agents::ZeroIntelligenceAgent>(id,v,market::Side::Sell,seed*2027+i));}
 std::mt19937_64 rng(seed); std::vector<market::Bid>bids;
 for(int r=0;r<c.rounds;r++){std::vector<int> order(ps.size());std::iota(order.begin(),order.end(),0);std::shuffle(order.begin(),order.end(),rng); for(int j:order){auto id=ps[j]->id(); auto a=m.accounts().at(id); bool buyer=id[0]=='B'; int inv=0; auto it=a.inventory.find("ASSET"); if(it!=a.inventory.end()) inv=it->second; if((buyer && inv>0)||(!buyer && inv==0))continue; agents::Observation o{(std::uint64_t)r,"ASSET",c.fundamental,timeline.signal_for(id,"ASSET",r),m.best_bid("ASSET"),m.best_ask("ASSET"),m.last_trade_price("ASSET")}; auto b=ps[j]->act(o); if(m.submit(b))bids.push_back(b);}}
 return {seed,m.metrics(),std::move(bids),m.trades()};
}
std::vector<TrialResult> run_paired_news_experiment(std::uint64_t first,std::size_t n,const ReferenceAuctionConfig&c,double signal,double reliability){std::vector<TrialResult> out;out.reserve(n); for(size_t i=0;i<n;i++){auto seed=first+i;InformationTimeline ctl,trt;trt.add({5,"news","ASSET",signal,reliability,{},"controlled public signal",{}});auto a=run_reference_auction(seed,c,ctl,false);auto b=run_reference_auction(seed,c,trt,false);out.push_back({seed,a.metrics.allocative_efficiency,b.metrics.allocative_efficiency,b.metrics.allocative_efficiency-a.metrics.allocative_efficiency});}return out;}
}
