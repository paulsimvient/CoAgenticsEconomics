#include "coagentics/experiment/MarketScientist.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <numeric>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
namespace coagentics::experiment {
namespace {
struct Trader {bool buyer{};double marginal[4]{};int next{};};
struct Quote {int trader{};double price{};int seq{};};
struct Draw {int actor{};double u{};};
std::vector<Draw> tape(std::uint64_t seed,int events){std::mt19937_64 rng(seed);std::uniform_int_distribution<int>actor(0,11);std::uniform_real_distribution<double>u(0.,1.);std::vector<Draw> v;v.reserve(events);for(int i=0;i<events;++i)v.push_back({actor(rng),u(rng)});return v;}
MarketOutcome simulate(const std::vector<Draw>&draws,const std::string&policy,const Probe&p,bool treatment,std::vector<DecisionTrace>* trace=nullptr,const std::vector<double>* network_exposure=nullptr,
 const std::function<void(const DecisionTrace&)>* on_decision=nullptr){
 Trader t[12];for(int i=0;i<6;++i){t[i].buyer=true;for(int j=0;j<4;++j)t[i].marginal[j]=196-10*i-18*j;t[i+6].buyer=false;for(int j=0;j<4;++j)t[i+6].marginal[j]=20+10*i+18*j;}
 std::vector<double> b,s;for(const auto&x:t)for(double v:x.marginal)(x.buyer?b:s).push_back(v);std::sort(b.begin(),b.end(),std::greater<>());std::sort(s.begin(),s.end());double optimum=0;for(size_t i=0;i<b.size()&&b[i]>s[i];++i)optimum+=b[i]-s[i];
 std::optional<Quote> bid,ask;double surplus=0,last_price=100,price_sum=0,target_sum=0;int trades=0,target_quotes=0,seq=0;
 for(size_t event=0;event<draws.size();++event){const auto&d=draws[event];int i=d.actor;auto&x=t[i];
 DecisionTrace tr;tr.event=static_cast<int>(event);tr.actor=i;tr.marginal_unit=x.next;
 tr.buyer=x.buyer;tr.standing_bid=bid?bid->price:0.;tr.standing_ask=ask?ask->price:200.;tr.draw=d.u;
 tr.news_visible=treatment?p.news:0.;tr.peer_visible=treatment?(network_exposure?(*network_exposure)[static_cast<size_t>(i)]:p.peer):0.;
 auto record=[&](){if(trace)trace->push_back(tr);if(on_decision)(*on_decision)(tr);};
 if(x.next>=4){record();continue;}
 double value=x.marginal[x.next];tr.private_limit=value;
 double lo=x.buyer?(bid?bid->price:0.):value;double hi=x.buyer?value:(ask?ask->price:200.);
 if(lo>=hi){tr.disposition=QuoteDisposition::no_legal_quote;record();continue;}
 tr.legal=true;
 double price=lo+(hi-lo)*d.u;tr.desired_price=price;
 if(i==0 || (network_exposure && i<6)){tr.desired_price=price; // Single targeted buyer, all other traders remain matched ZI-C controls.
   double news=treatment?p.news:0.;double peer=treatment?(network_exposure?(*network_exposure)[static_cast<size_t>(i)]:p.peer):0.;double adjustment=0;
   if(policy=="signal-responsive")adjustment=news;
   else if(policy=="peer-responsive")adjustment=.6*peer;
   else if(policy=="adaptive")adjustment=news*(1+.15*std::max(0,p.repetitions-1));
   else if(policy=="risk-sensitive")adjustment=.75*news;
   else if(policy!="information-blind")throw std::invalid_argument("unknown target policy");
   // Quote-level intervention, bounded by marginal value; intervention changes
   // the target's desired bid but never changes its private value or budget constraint.
   tr.desired_price=price+adjustment;
   price=std::clamp(tr.desired_price,lo,hi);
 }
 tr.attempted=true;tr.submitted_price=price;tr.disposition=QuoteDisposition::accepted;
 if(x.buyer){if(bid&&price<=bid->price){tr.disposition=QuoteDisposition::rejected_standing;record();continue;}
 if(i==0){target_sum+=price;target_quotes++;}
   Quote q{i,price,++seq};if(ask&&price>=ask->price){int seller=ask->trader;double transaction=ask->seq<q.seq?ask->price:q.price;surplus+=value-t[seller].marginal[t[seller].next];++x.next;++t[seller].next;price_sum+=transaction;last_price=transaction;++trades;tr.disposition=QuoteDisposition::executed;tr.transaction_price=transaction;tr.counterparty=seller;bid.reset();ask.reset();}else bid=q;
 record();
 }else{if(ask&&price>=ask->price){tr.disposition=QuoteDisposition::rejected_standing;record();continue;}Quote q{i,price,++seq};if(bid&&bid->price>=price){int buyer=bid->trader;double transaction=bid->seq<q.seq?bid->price:q.price;surplus+=t[buyer].marginal[t[buyer].next]-value;++x.next;++t[buyer].next;price_sum+=transaction;last_price=transaction;++trades;tr.disposition=QuoteDisposition::executed;tr.transaction_price=transaction;tr.counterparty=buyer;bid.reset();ask.reset();}else ask=q;record();}
 }
 (void)last_price;return {100*surplus/optimum,surplus,target_quotes?target_sum/target_quotes:0.,trades?price_sum/trades:0.,trades,target_quotes};
}
}
MarketPair CdaMarketDomain::run_market(const Probe&p,std::uint64_t seed)const{if(events_<1)throw std::invalid_argument("quote events must be positive");auto draws=tape(seed,events_);return {simulate(draws,policy_,p,false),simulate(draws,policy_,p,true),seed};}
TracedMarketPair CdaMarketDomain::run_traced(const Probe&p,std::uint64_t seed)const{
 if(events_<1)throw std::invalid_argument("quote events must be positive");
 auto draws=tape(seed,events_);TracedMarketPair r;r.outcomes.seed=seed;
 r.outcomes.control=simulate(draws,policy_,p,false,&r.control);
 r.outcomes.treatment=simulate(draws,policy_,p,true,&r.treatment);
 return r;
}
PairedObservation CdaMarketDomain::run(const Probe&p,std::uint64_t seed)const{auto x=run_market(p,seed); // The scientist sees the causal quote-level response; efficiency is a secondary market outcome.
 return {x.control.mean_target_quote,x.treatment.mean_target_quote,seed};}

NetworkMarketResult run_network_market(const std::vector<analysis::NetworkEdge>& edges,
 unsigned origin,double impulse,unsigned propagation_steps,std::uint64_t seed,int quote_events){
 if(quote_events<1)throw std::invalid_argument("quote_events must be positive");
 // Six buyer nodes can receive the external message. The other six traders
 // are unchanged ZI-C market controls. No behavioral ground truth is read.
 auto exposure=analysis::propagate(6,edges,origin,impulse,propagation_steps);
 std::vector<double> signals(12,0.0);
 for(size_t i=0;i<6;++i)signals[i]=exposure.responses[i];
 auto draws=tape(seed,quote_events); Probe p{"network-message",0,0,1};
 NetworkMarketResult r;r.exposure=std::move(exposure);
 r.market.outcomes.seed=seed;
 r.market.outcomes.control=simulate(draws,"peer-responsive",p,false,&r.market.control,&signals);
 r.market.outcomes.treatment=simulate(draws,"peer-responsive",p,true,&r.market.treatment,&signals);
 r.attempted_quote_shift.assign(6,0);r.matched_attempts.assign(6,0);
 // Compare matched activations only when both market states allow a quote.
 // This is an intent-level observable, not a direct causal effect on utility.
 for(size_t j=0;j<r.market.control.size();++j){
  const auto& c=r.market.control[j];const auto& t=r.market.treatment[j];
  if(c.actor>=0 && c.actor<6 && c.attempted && t.attempted){
   auto i=static_cast<size_t>(c.actor);
   r.attempted_quote_shift[i]+=t.desired_price-c.desired_price;
   ++r.matched_attempts[i];
  }
 }
 for(size_t i=0;i<6;++i)if(r.matched_attempts[i])r.attempted_quote_shift[i]/=r.matched_attempts[i];
 return r;
}

TracedMarketPair run_live_network_market(std::uint64_t seed,int quote_events,
 std::vector<double>& exposures,
 const std::function<void(const DecisionTrace&,std::vector<double>&)>& on_decision){
 if(quote_events<1 || exposures.size()!=12)throw std::invalid_argument("invalid live market configuration");
 auto draws=tape(seed,quote_events);Probe p{"live-network",0,0,1};
 TracedMarketPair r;r.outcomes.seed=seed;
 r.outcomes.control=simulate(draws,"peer-responsive",p,false,&r.control,&exposures);
 std::function<void(const DecisionTrace&)> relay=[&](const DecisionTrace&tr){on_decision(tr,exposures);};
 r.outcomes.treatment=simulate(draws,"peer-responsive",p,true,&r.treatment,&exposures,&relay);
 return r;
}
MarketScientistResult run_market_scientist(const CdaMarketDomain&d,const std::vector<Candidate>&c,const std::vector<Probe>&p,std::uint64_t seed,const ScientistConfig&cfg){MarketScientistResult r;r.scientist=run_scientist(d,c,p,seed,cfg);for(size_t i=0;i<r.scientist.steps.size();++i)r.selected_probe_market_pairs.push_back(d.run_market(r.scientist.steps[i].probe,seed+i*100000));return r;}
std::string market_scientist_markdown(const MarketScientistResult&r){std::ostringstream o;o<<"# CoAgentics v16 — Full-market automated scientist\n\n"<<scientist_markdown(r.scientist)<<"\n## Secondary market outcomes (first matched seed per selected probe)\n\n| Probe | Control efficiency | Treatment efficiency | Δ efficiency | Control trades | Treatment trades | Target quotes (C/T) |\n|---|---:|---:|---:|---:|---:|---:|\n";for(size_t i=0;i<r.selected_probe_market_pairs.size();++i){const auto&x=r.selected_probe_market_pairs[i];o<<"| "<<r.scientist.steps[i].probe.id<<" | "<<std::fixed<<std::setprecision(2)<<x.control.efficiency<<"% | "<<x.treatment.efficiency<<"% | "<<x.treatment.efficiency-x.control.efficiency<<" pp | "<<x.control.trades<<" | "<<x.treatment.trades<<" | "<<x.control.target_quotes<<"/"<<x.treatment.target_quotes<<" |\n";}o<<"\nThe CDA uses transparent analogue marginal schedules, not verified published schedules. The targeted agent is a controlled policy, not a live LLM. A matched activation/random-number tape controls exogenous randomness, but changed trades alter subsequent market state; quote averages may also change composition. Posterior inference uses the v15 linear probe approximation, which is not calibrated to this nonlinear market; treat its probabilities as exploratory, not validated mechanism recovery.\n";return o.str();}
}
