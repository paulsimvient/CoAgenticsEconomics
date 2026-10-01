#include "coagentics/experiment/Runner.hpp"
#include <cassert>
#include <iostream>
int main(){coagentics::experiment::ReferenceAuctionConfig c;c.rounds=50;double sum=0;int n=100;for(int i=0;i<n;i++){auto x=coagentics::experiment::run_reference_auction(1000+i,c);assert(x.metrics.maximum_surplus>0);assert(x.metrics.allocative_efficiency>=0&&x.metrics.allocative_efficiency<=100);sum+=x.metrics.allocative_efficiency;}std::cout<<"ZI mean allocative efficiency="<<sum/n<<"%\n";assert(sum/n>50);}
