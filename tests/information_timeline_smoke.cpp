#include "coagentics/experiment/Intervention.hpp"
#include <cassert>
#include <cmath>
using namespace coagentics::experiment;
int main(){ InformationTimeline t; t.add({10,"n1","A",-10,0.8,{},"public"}); t.add({12,"n2","A",5,1.0,std::string("agent1"),"private"}); assert(t.signal_for("agent2","A",20)==-8); assert(t.signal_for("agent1","A",20)==-3); assert(t.signal_for("agent1","A",9)==0); }
