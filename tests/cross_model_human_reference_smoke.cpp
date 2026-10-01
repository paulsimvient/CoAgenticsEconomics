#include "coagentics/experiment/CrossModelCampaign.hpp"
#include <cassert>
#include <iostream>
int main(){
 std::vector<coagentics::agents::ModelIdentity> models;
 for(int i=0;i<10;++i) models.push_back({"provider"+std::to_string(i%3),"model"+std::to_string(i),"pinned-v1","black-box"});
 auto refs=coagentics::analysis::qualification_reference_fixture();
 auto s=coagentics::experiment::run_cross_model_qualification(models,5000,3,refs);
 assert(s.distinct_models==10); assert(s.mechanisms==2); assert(s.cells.size()==10*3*2*2); assert(s.mean_efficiency>90.0); assert(!s.human_reference_comparisons.empty());
 assert(refs.get("allocative_efficiency").provenance.find("synthetic")!=std::string::npos);
 std::cout<<coagentics::experiment::cross_model_json(s);
}
