#include "coagentics/analysis/Benchmark.hpp"
#include <cassert>
int main(){auto r=coagentics::analysis::run_benchmark_battery(42);assert(r.size()==5);for(auto&x:r)assert(x.recovered);}
