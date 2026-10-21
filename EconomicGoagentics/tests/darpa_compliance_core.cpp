#include "coagentics/market/Mechanism.hpp"
#include "coagentics/analysis/Classifier.hpp"
#include "coagentics/analysis/Discrimination.hpp"
#include <cassert>
#include <iostream>
using namespace coagentics;
static market::MarketConfig cfg(){market::MarketConfig c;c.fundamental_value["A"]=100;c.private_values.buy_values["B\nA"]={120};c.private_values.sell_costs["S\nA"]={80};return c;}
int main(){for(auto k:{market::MechanismKind::ContinuousDoubleAuction,market::MechanismKind::SealedBidDoubleAuction}){auto m=market::make_mechanism(k,cfg());m->add_account("B",{1000,{}});m->add_account("S",{0,{{"A",1}}});assert(m->submit({1,"B","A",1,110,market::Side::Buy}));assert(m->submit({1,"S","A",1,90,market::Side::Sell}));m->clear(1);assert(m->trades().size()==1);assert(m->utility().total_utility>39.9);assert(m->metrics().allocative_efficiency>99.9);}analysis::MechanismClassifier cl;auto x=cl.classify({10,0,0,0});assert(x.label=="direct_information_response");auto matrix=analysis::run_discrimination_matrix(7);assert(matrix.size()==16);std::cout<<"DARPA compliance core: PASS, discrimination cells="<<matrix.size()<<"\n";}
