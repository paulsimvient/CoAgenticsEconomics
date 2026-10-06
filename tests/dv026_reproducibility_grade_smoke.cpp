#include "coagentics/experiment/Dv026LiveCampaign.hpp"
#include <cassert>
#include <iostream>
int main(){
 using namespace coagentics::experiment;
 coagentics::agents::ModelIdentity m{"ollama","qwen-test","local","black-box"};
 auto unmeasured=grade_reproducibility(m,"temperature=0;inference_seed=42",true,"supported");
 assert(unmeasured.simulator_seed_controlled);
 assert(unmeasured.grade=="B");
 assert(unmeasured.repeatability==RepeatabilityStatus::NotMeasured);
 auto exact=grade_reproducibility(m,"temperature=0;inference_seed=42",true,"supported",{"{A}","{A}","{A}"});
 assert(exact.grade=="A"); assert(exact.exact_repeats==3); assert(exact.repeatability==RepeatabilityStatus::Exact);
 auto variable=grade_reproducibility(m,"temperature=0;inference_seed=42",true,"supported",{"{A}","{B}","{A}"});
 assert(variable.grade=="B"); assert(variable.exact_repeats==2); assert(variable.repeatability==RepeatabilityStatus::Variable);
 auto unsupported=grade_reproducibility(m,"temperature=0",false,"unsupported");
 assert(unsupported.grade=="C");
 auto failed=grade_reproducibility(m,"temperature=0;inference_seed=42",true,"supported",{"{A}",""});
 assert(failed.grade=="F");
 auto json=reproducibility_grade_json(exact);
 assert(json.find("\"grade\":\"A\"")!=std::string::npos);
 assert(json.find("\"repeatability\":\"exact\"")!=std::string::npos);
 std::cout<<"reproducibility grading OK\n";
}
