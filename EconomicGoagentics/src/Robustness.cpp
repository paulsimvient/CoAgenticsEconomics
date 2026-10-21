#include "coagentics/analysis/Robustness.hpp"
#include <algorithm>
#include <cmath>
#include <random>
#include <sstream>
namespace coagentics::analysis {
using namespace coagentics::agents;
static std::vector<HarnessRecord> synthetic(int kind, int seed, double noise=0){
 std::mt19937 rng(seed);std::normal_distribution<double> eps(0,noise);
 std::vector<HarnessRecord> out;
 for(int rep=0;rep<16;++rep)for(int stage=0;stage<5;++stage){
  double p=100;
  if(kind==1){if(stage==1)p+=10;if(stage==3)p-=10;}
  if(kind==2){if(stage==1)p+=10;if(stage==2)p+=7;if(stage==3)p-=3;}
  if(kind==3&&stage==4)p+=6;
  if(kind==4){if(stage==1)p+=4;if(stage==2)p-=6;if(stage==3)p+=9;if(stage==4)p-=8;}
  p+=eps(rng);
  HarnessRecord r; r.request.request_id="r"+std::to_string(rep*5+stage);
  r.request.model={"opaque","test-model","v1"};r.request.observation.time=rep*5+stage;
  r.response.request_id=r.request.request_id;
  r.response.action=LlmAction{r.request.observation.time,"asset-A",1,p,coagentics::market::Side::Buy,false};
  out.push_back(r);
 }return out;
}
RobustnessReport run_robustness_audit(){
 RobustnessReport result;
 auto check=[&](std::string name,std::string expected,const std::vector<HarnessRecord>&records){auto x=infer_black_box(records);result.cases.push_back({std::move(name),expected,x.hypothesis,expected==x.hypothesis});};
 const char* labels[]={"blind","direct-news","persistent-response","peer-response"};
 for(int k=0;k<4;++k)for(int seed=9001;seed<9021;++seed){auto rs=synthetic(k,seed,.45);check("held-out noise k="+std::to_string(k)+" seed="+std::to_string(seed),labels[k],rs);}
 for(int seed=9101;seed<9121;++seed)check("unknown mechanism seed="+std::to_string(seed),"unresolved",synthetic(4,seed,.45));
 auto missing=synthetic(1,1);missing.erase(std::remove_if(missing.begin(),missing.end(),[](auto&r){return r.request.observation.time%5==3;}),missing.end());check("missing reversal stage","unresolved",missing);
 auto sparse=synthetic(1,1);sparse.erase(std::remove_if(sparse.begin(),sparse.end(),[](auto&r){return r.request.observation.time>=10;}),sparse.end());check("insufficient stage replication","unresolved",sparse);
 auto failed=synthetic(1,1);for(int i=0;i<25;++i){failed[i].response.error="provider timeout";failed[i].response.action.reset();}check("provider failure reliability","unresolved",failed);
 auto shuffled=synthetic(2,1);std::mt19937 rng(99);std::shuffle(shuffled.begin(),shuffled.end(),rng);check("record order invariance","persistent-response",shuffled);
 auto tampered=synthetic(3,1);for(auto&r:tampered){r.request.model={"unrelated","different","v999"};r.request.agent_id="other";r.response.raw_output="changed";}check("metadata isolation","peer-response",tampered);
 result.total=result.cases.size();for(auto&x:result.cases)result.passed+=x.pass;result.all_passed=result.passed==result.total;return result;
}
std::string robustness_markdown(const RobustnessReport&r){std::ostringstream o;o<<"# v22 Black-box robustness audit\n\n"<<r.passed<<"/"<<r.total<<" checks passed.\n\n";
 for(auto&x:r.cases)if(!x.pass)o<<"- FAIL "<<x.name<<" expected="<<x.expected<<" observed="<<x.observed<<"\n";
 o<<"\nThese are synthetic, preregistered-signature stress tests, not live-model experiments. The unknown mechanism is one constructed out-of-distribution case; no broad open-world detection claim is justified.\n";return o.str();}
}
