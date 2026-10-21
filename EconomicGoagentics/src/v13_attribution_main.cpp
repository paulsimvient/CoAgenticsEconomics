#include "coagentics/analysis/InstitutionalAttribution.hpp"
#include <fstream>
#include <iostream>
int main(){auto r=coagentics::analysis::run_institutional_attribution(26026,40);std::ofstream("V13_INSTITUTION_AGENT_REPORT.md")<<coagentics::analysis::institutional_attribution_markdown(r);std::ofstream("V13_INSTITUTION_AGENT_REPORT.json")<<coagentics::analysis::institutional_attribution_json(r);std::cout<<"institution_floor="<<r.institution_floor<<" cells="<<r.cells.size()<<"\n";}
