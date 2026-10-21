#include "coagentics/analysis/Evidence.hpp"
#include <cassert>
int main(){using namespace coagentics::analysis;EvidenceStore s;EvidenceRecord e;e.hypothesis_id="H1";e.discriminator_id="D1";e.direction=EvidenceDirection::Supports;e.propagation_ratio=.1;s.append({"EXP1","EXP1-C","EXP1-T",42,e});auto j=s.jsonl();assert(j.find("\"seed\":42")!=std::string::npos);assert(j.find("\"direction\":\"supports\"")!=std::string::npos);}
