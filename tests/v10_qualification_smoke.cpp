#include "coagentics/analysis/HumanReference.hpp"
#include "coagentics/analysis/Qualification.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
int main(){
 auto refs=coagentics::analysis::empirical_market_reference_catalog();
 assert(refs.has("allocative_efficiency")); auto& d=refs.get("allocative_efficiency");
 assert(d.empirical); assert(d.samples.size()==5); assert(d.source_id=="doi:10.1086/261868");
 assert(d.mean>97.6 && d.mean<97.7);
 auto c=refs.compare("allocative_efficiency",99.0); assert(c.empirical_reference); assert(c.empirical_percentile>=0.0);
 auto q=coagentics::analysis::build_v10_qualification_report(99.0,refs);
 assert(q.software_green); assert(!q.darpa_claim_ready); assert(q.checks.size()>=9);
 std::ofstream("v10_qualification_report.md")<<coagentics::analysis::qualification_markdown(q);
 std::ofstream("v10_qualification_report.json")<<coagentics::analysis::qualification_json(q);
 std::cout<<coagentics::analysis::qualification_json(q)<<"\n";
}
