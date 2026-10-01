#include "coagentics/market/Market.hpp"
#include <iostream>
using namespace coagentics::market;
int main(){
 Market m({{{"ALPHA",100.0}},{{"ALPHA",10}}});
 m.add_account("buyer",{2000.0,{}}); m.add_account("seller",{0.0,{{"ALPHA",10}}});
 m.submit({1,"buyer","ALPHA",10,105.0,Side::Buy}); m.submit({1,"seller","ALPHA",10,95.0,Side::Sell});
 auto x=m.metrics(); std::cout<<"DV026 market qualification: efficiency="<<x.allocative_efficiency<<"% trades="<<m.trades().size()<<"\n";
 return x.allocative_efficiency>=90.0?0:1;
}
