#include "coagentics/market/Market.hpp"
#include <cassert>
using namespace coagentics::market;
int main(){ Market m({{{"A",10.0}},{{"A",10}}}); m.add_account("b",{200,{}});m.add_account("s",{0,{{"A",10}}}); assert(m.submit({0,"b","A",10,11,Side::Buy}));assert(m.submit({0,"s","A",10,9,Side::Sell}));assert(m.trades().size()==1);assert(m.metrics().allocative_efficiency>90); }
