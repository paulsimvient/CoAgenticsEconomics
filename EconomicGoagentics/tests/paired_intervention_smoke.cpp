#include "coagentics/experiment/Experiment.hpp"
#include <cassert>
#include <vector>
using namespace coagentics::experiment;
int main(){ std::vector<TrialResult>x; for(unsigned i=0;i<100;i++)x.push_back({i,96.0,89.0,-7.0}); auto s=summarize(x);assert(s.n==100);assert(s.mean_delta<-6.9&&s.mean_delta>-7.1); }
