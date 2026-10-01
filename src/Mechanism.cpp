#include "coagentics/market/Mechanism.hpp"
#include <algorithm>
#include <cmath>
namespace coagentics::market {
namespace {
UtilitySummary utility_from(const MarketConfig& c,const std::vector<Trade>& ts){
 UtilitySummary u; std::map<std::string,int> bi,si;
 for(auto&t:ts) for(int k=0;k<t.quantity;++k){
  auto bk=t.buyer+"\n"+t.asset,sk=t.seller+"\n"+t.asset; int b=bi[bk]++,s=si[sk]++;
  auto bv=c.private_values.buy_values.find(bk),sc=c.private_values.sell_costs.find(sk);
  if(bv!=c.private_values.buy_values.end()&&b<(int)bv->second.size()) u.buyer_utility+=bv->second[b]-t.price;
  if(sc!=c.private_values.sell_costs.end()&&s<(int)sc->second.size()) u.seller_utility+=t.price-sc->second[s];
 }
 u.total_utility=u.buyer_utility+u.seller_utility; return u;
}
}

class Cda final:public MarketMechanism{
 MarketConfig c_; Market m_;
public:
 explicit Cda(MarketConfig c):c_(c),m_(std::move(c)){}
 void add_account(std::string i,Account a)override{m_.add_account(std::move(i),std::move(a));}
 bool submit(const Bid&b)override{return m_.submit(b);}
 SubmitResult submit_detailed(const Bid&b)override{return m_.submit_detailed(b);}
 void clear(std::uint64_t)override{}
 const std::vector<Trade>& trades()const override{return m_.trades();}
 Metrics metrics()const override{return m_.metrics();}
 UtilitySummary utility()const override{return utility_from(c_,m_.trades());}
 const std::map<std::string,Account>& accounts()const override{return m_.accounts();}
 std::optional<double> best_bid_opt(const std::string& asset)const override{
  return m_.has_bid(asset)?std::optional<double>(m_.best_bid(asset)):std::nullopt;
 }
 std::optional<double> best_ask_opt(const std::string& asset)const override{
  return m_.has_ask(asset)?std::optional<double>(m_.best_ask(asset)):std::nullopt;
 }
 MechanismKind kind()const override{return MechanismKind::ContinuousDoubleAuction;}
};

class Sealed final:public MarketMechanism{
 MarketConfig c_; std::map<std::string,Account> a_; std::vector<Bid> b_; std::vector<Trade> t_;
public:
 explicit Sealed(MarketConfig c):c_(std::move(c)){}
 void add_account(std::string i,Account a)override{a_[std::move(i)]=std::move(a);}
 bool submit(const Bid&b)override{
  if(b.quantity<=0||b.price<0||!a_.count(b.agent_id)) return false;
  b_.push_back(b); return true;
 }
 SubmitResult submit_detailed(const Bid&b)override{
  SubmitResult r; r.submitted_quantity=b.quantity;
  if(b.quantity<=0 || b.price<0 || !std::isfinite(b.price) || !a_.count(b.agent_id)){
   r.rejection_reason="invalid_order"; return r;
  }
  if(!c_.fundamental_value.count(b.asset)){
   r.rejection_reason="unknown_asset"; return r;
  }
  auto &a=a_.at(b.agent_id);
  if(b.side==Side::Buy && a.cash + 1e-9 < b.price*b.quantity){
   r.rejection_reason="insufficient_cash"; return r;
  }
  if(b.side==Side::Sell && a.inventory[b.asset] < b.quantity){
   r.rejection_reason="insufficient_inventory"; return r;
  }
  b_.push_back(b);
  r.accepted=true; r.resting_quantity=b.quantity; return r;
 }
 void clear(std::uint64_t tm)override{
  std::vector<Bid> bu,se;
  for(auto b:b_) (b.side==Side::Buy?bu:se).push_back(b);
  std::sort(bu.begin(),bu.end(),[](auto&x,auto&y){return x.price>y.price;});
  std::sort(se.begin(),se.end(),[](auto&x,auto&y){return x.price<y.price;});
  size_t i=0,j=0;
  while(i<bu.size()&&j<se.size()&&bu[i].price>=se[j].price){
   int q=std::min(bu[i].quantity,se[j].quantity);
   double p=(bu[i].price+se[j].price)/2;
   auto&ba=a_[bu[i].agent_id]; auto&sa=a_[se[j].agent_id];
   q=std::min(q,(int)std::floor((ba.cash+1e-9)/p));
   q=std::min(q,sa.inventory[bu[i].asset]);
   if(q<=0){
    if(sa.inventory[bu[i].asset]<=0) ++j;
    else if(ba.cash+1e-9<p) ++i;
    else break;
    continue;
   }
   ba.cash-=p*q; ba.inventory[bu[i].asset]+=q;
   sa.cash+=p*q; sa.inventory[bu[i].asset]-=q;
   t_.push_back({tm,bu[i].asset,q,p,bu[i].agent_id,se[j].agent_id});
   bu[i].quantity-=q; se[j].quantity-=q;
   if(bu[i].quantity<=0) ++i;
   if(se[j].quantity<=0) ++j;
  }
  b_.clear();
 }
 const std::vector<Trade>& trades()const override{return t_;}
 Metrics metrics()const override{
  Market evaluator(c_);
  for(auto&[id,a]:a_) evaluator.add_account(id,a);
  auto m=evaluator.metrics();
  double realized=utility_from(c_,t_).total_utility;
  return{realized,m.maximum_surplus,
   m.maximum_surplus>0?100*std::clamp(realized/m.maximum_surplus,0.0,1.0):100,
   m.efficient_quantity};
 }
 UtilitySummary utility()const override{return utility_from(c_,t_);}
 const std::map<std::string,Account>& accounts()const override{return a_;}
 MechanismKind kind()const override{return MechanismKind::SealedBidDoubleAuction;}
};

std::unique_ptr<MarketMechanism> make_mechanism(MechanismKind k,MarketConfig c){
 if(k==MechanismKind::SealedBidDoubleAuction) return std::make_unique<Sealed>(std::move(c));
 return std::make_unique<Cda>(std::move(c));
}
}
