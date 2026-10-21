#include "coagentics/experiment/Scientist.hpp"
#include <iostream>
using namespace coagentics::experiment;
int main(){std::vector<Candidate>h={{"blind",0,0,0,.25},{"news",1,0,0,.25},{"peer",0,.6,0,.25},{"mixed",1,.6,0,.25}};std::vector<Probe>p={{"news-only",10,0,1},{"peer-only",0,20,1},{"both",10,20,1}};for(auto m:{"information-blind","signal-responsive","peer-responsive"}){auto r=run_scientist(AgentProbeDomain(m),h,p,26026);std::cout<<"\n## Qualification fixture: "<<m<<"\n"<<scientist_markdown(r);}}
