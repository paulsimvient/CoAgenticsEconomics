#include "coagentics/market/Market.hpp"
#include <cassert>
#include <cmath>
using namespace coagentics::market;
int main(){
 MarketConfig c; c.fundamental_value["A"]=100; c.total_supply["A"]=2;
 c.private_values.buy_values["b1\nA"]={120}; c.private_values.buy_values["b2\nA"]={110};
 c.private_values.sell_costs["s1\nA"]={70}; c.private_values.sell_costs["s2\nA"]={90};
 Market m(c); m.add_account("b1",{500,{}});m.add_account("b2",{500,{}});m.add_account("s1",{0,{{"A",1}}});m.add_account("s2",{0,{{"A",1}}});
 assert(m.submit({1,"b1","A",1,115,Side::Buy})); assert(m.submit({1,"s1","A",1,75,Side::Sell}));
 assert(m.submit({2,"b2","A",1,105,Side::Buy})); assert(m.submit({2,"s2","A",1,95,Side::Sell}));
 auto x=m.metrics(); assert(std::abs(x.maximum_surplus-70)<1e-9); assert(std::abs(x.realized_surplus-70)<1e-9); assert(std::abs(x.allocative_efficiency-100)<1e-9); assert(x.efficient_quantity==2);
}
