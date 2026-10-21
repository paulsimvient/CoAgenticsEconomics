#include "coagentics/experiment/MarketScientist.hpp"
#include "coagentics/analysis/Influence.hpp"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <cstdint>
using namespace coagentics;
static const char* name(experiment::QuoteDisposition d){using Q=experiment::QuoteDisposition;switch(d){case Q::exhausted:return "exhausted";case Q::no_legal_quote:return "no_legal_quote";case Q::rejected_standing:return "rejected_standing";case Q::accepted:return "accepted";case Q::executed:return "executed";}return "unknown";}
int main(int argc,char**argv){
 try{
 const std::uint64_t seed=argc>1?std::stoull(argv[1]):424242;
 const int events=argc>2?std::stoi(argv[2]):600;
 if(events<1||events>10000)throw std::invalid_argument("events out of range");
 std::vector<double> exposures(12,0.);double impulse=0;bool edge=true;
 std::cout<<std::setprecision(12);
 auto r=experiment::run_live_network_market(seed,events,exposures,[&](const experiment::DecisionTrace&t,std::vector<double>&signals){
  std::cout<<"{\"kind\":\"decision\",\"event\":"<<t.event<<",\"actor\":"<<t.actor<<",\"buyer\":"<<(t.buyer?"true":"false")<<",\"private_limit\":"<<t.private_limit<<",\"standing_bid\":"<<t.standing_bid<<",\"standing_ask\":"<<t.standing_ask<<",\"news\":"<<t.news_visible<<",\"peer\":"<<t.peer_visible<<",\"attempted\":"<<(t.attempted?"true":"false")<<",\"desired_price\":"<<t.desired_price<<",\"submitted_price\":"<<t.submitted_price<<",\"disposition\":\""<<name(t.disposition)<<"\",\"transaction_price\":"<<t.transaction_price<<",\"counterparty\":"<<t.counterparty<<"}"<<std::endl;
  std::string cmd; if(!std::getline(std::cin,cmd))throw std::runtime_error("control channel closed");
  // Commands originate in the localhost controller after the current decision is published.
  // NEXT <impulse> <edge_enabled>. Zero impulse is the matched null condition.
  double next_impulse=impulse;int next_edge=edge?1:0;
  if(std::sscanf(cmd.c_str(),"NEXT %lf %d",&next_impulse,&next_edge)==2){impulse=next_impulse;edge=next_edge!=0;}
  std::vector<analysis::NetworkEdge> edges{{0,1,.8}};
  if(edge)edges.push_back({1,2,.75});
  auto p=analysis::propagate(6,edges,0,impulse,2);
  for(size_t i=0;i<6;++i)signals[i]=p.responses[i];
 });
 auto emit=[&](const char* arm,const experiment::MarketOutcome&o){std::cout<<"{\"kind\":\"outcome\",\"arm\":\""<<arm<<"\",\"efficiency\":"<<o.efficiency<<",\"surplus\":"<<o.surplus<<",\"trades\":"<<o.trades<<",\"mean_transaction_price\":"<<o.mean_transaction_price<<"}"<<std::endl;};
 emit("control",r.outcomes.control);emit("treatment",r.outcomes.treatment);
 std::cout<<"{\"kind\":\"complete\",\"seed\":"<<seed<<",\"events\":"<<events<<"}"<<std::endl;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
