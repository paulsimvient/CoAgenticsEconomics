#include "coagentics/experiment/Intervention.hpp"
#include <cassert>
int main(){coagentics::experiment::InformationTimeline t;t.add({5,"e","X",-10,0.5,{},"temporary",8});assert(t.signal_for("A","X",4)==0);assert(t.signal_for("A","X",5)==-5);assert(t.signal_for("A","X",7)==-5);assert(t.signal_for("A","X",8)==0);}
