#include "coagentics/market/Market.hpp"
#include <algorithm>
#include <cmath>
namespace coagentics::market {
Market::Market(MarketConfig config):config_(std::move(config)){}
void Market::add_account(std::string id, Account account){ accounts_[std::move(id)]=std::move(account); }
bool Market::submit(const Bid& b){ return submit_detailed(b).accepted; }
SubmitResult Market::submit_detailed(const Bid& b){
 SubmitResult r; r.submitted_quantity=b.quantity;
 if(b.quantity<=0 || b.price<0 || !config_.fundamental_value.count(b.asset) || !accounts_.count(b.agent_id)){
  r.rejection_reason="invalid_order"; return r;
 }
 auto &a=accounts_.at(b.agent_id);
 if(b.side==Side::Buy && a.cash + 1e-9 < b.price*b.quantity){ r.rejection_reason="insufficient_cash"; return r; }
 if(b.side==Side::Sell && a.inventory[b.asset] < b.quantity){ r.rejection_reason="insufficient_inventory"; return r; }
 const std::size_t before=trades_.size();
 (b.side==Side::Buy?buys_:sells_).push_back(b);
 match(b.asset,b.time);
 r.accepted=true;
 double notional=0;
 for(std::size_t i=before;i<trades_.size();++i){
  const auto& t=trades_[i];
  if(t.buyer==b.agent_id || t.seller==b.agent_id){
   r.trades.push_back(t);
   r.filled_quantity+=t.quantity;
   notional+=t.price*t.quantity;
  }
 }
 if(r.filled_quantity>0) r.average_fill_price=notional/r.filled_quantity;
 r.resting_quantity=resting_quantity(b.agent_id,b.asset);
 return r;
}
void Market::match(const std::string& asset,std::uint64_t time){
 while(true){
  auto bi=buys_.end(); for(auto i=buys_.begin();i!=buys_.end();++i) if(i->asset==asset && (bi==buys_.end()||i->price>bi->price)) bi=i;
  auto si=sells_.end(); for(auto i=sells_.begin();i!=sells_.end();++i) if(i->asset==asset && (si==sells_.end()||i->price<si->price)) si=i;
  if(bi==buys_.end()||si==sells_.end()||bi->price<si->price) break;
  int q=std::min(bi->quantity,si->quantity); double p=(bi->price+si->price)/2.0;
  auto &buyer=accounts_.at(bi->agent_id); auto &seller=accounts_.at(si->agent_id);
  q=std::min(q,(int)std::floor((buyer.cash+1e-9)/p)); q=std::min(q,seller.inventory[asset]); if(q<=0) break;
  buyer.cash-=p*q; buyer.inventory[asset]+=q; seller.cash+=p*q; seller.inventory[asset]-=q;
  trades_.push_back({time,asset,q,p,bi->agent_id,si->agent_id}); bought_units_[bi->agent_id+"\n"+asset]+=q; sold_units_[si->agent_id+"\n"+asset]+=q;
  bi->quantity-=q; si->quantity-=q; if(bi->quantity==0) buys_.erase(bi); if(si->quantity==0) sells_.erase(si);
 }
}
Metrics Market::metrics() const {
 double realized=0,maxs=0; int efficient_q=0;
 std::map<std::string,int> bu,se;
 for(const auto&t:trades_) for(int k=0;k<t.quantity;++k){
  auto bk=t.buyer+"\n"+t.asset, sk=t.seller+"\n"+t.asset; int bi=bu[bk]++, si=se[sk]++;
  auto bvIt=config_.private_values.buy_values.find(bk), scIt=config_.private_values.sell_costs.find(sk);
  if(bvIt!=config_.private_values.buy_values.end() && scIt!=config_.private_values.sell_costs.end() && bi<(int)bvIt->second.size() && si<(int)scIt->second.size()) realized += bvIt->second[bi]-scIt->second[si];
 }
 for(const auto&[asset,_]:config_.fundamental_value){
  std::vector<double> bvals, costs;
  for(const auto&[key,vals]:config_.private_values.buy_values) if(key.size()>asset.size()+1 && key.ends_with("\n"+asset)) bvals.insert(bvals.end(),vals.begin(),vals.end());
  for(const auto&[key,vals]:config_.private_values.sell_costs) if(key.size()>asset.size()+1 && key.ends_with("\n"+asset)) costs.insert(costs.end(),vals.begin(),vals.end());
  std::sort(bvals.begin(),bvals.end(),std::greater<>()); std::sort(costs.begin(),costs.end());
  for(size_t i=0;i<std::min(bvals.size(),costs.size()) && bvals[i]>costs[i];++i){maxs+=bvals[i]-costs[i]; ++efficient_q;}
 }
 double eff=maxs>0?100.0*std::clamp(realized/maxs,0.0,1.0):100.0; return {realized,maxs,eff,efficient_q};
}
bool Market::has_bid(const std::string& asset) const { for(const auto& b:buys_) if(b.asset==asset) return true; return false; }
bool Market::has_ask(const std::string& asset) const { for(const auto& b:sells_) if(b.asset==asset) return true; return false; }
int Market::resting_quantity(const std::string& agent_id, const std::string& asset) const {
 int q=0;
 for(const auto& b:buys_) if(b.agent_id==agent_id && b.asset==asset) q+=b.quantity;
 for(const auto& b:sells_) if(b.agent_id==agent_id && b.asset==asset) q+=b.quantity;
 return q;
}
double Market::best_bid(const std::string& asset) const { double v=0; for(const auto& b:buys_) if(b.asset==asset) v=std::max(v,b.price); return v; }
double Market::best_ask(const std::string& asset) const { double v=0; bool set=false; for(const auto& b:sells_) if(b.asset==asset && (!set||b.price<v)){v=b.price;set=true;} return set?v:0; }
double Market::last_trade_price(const std::string& asset) const { for(auto it=trades_.rbegin();it!=trades_.rend();++it) if(it->asset==asset)return it->price; return 0; }
}
