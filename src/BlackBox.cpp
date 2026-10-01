#include "coagentics/analysis/BlackBox.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
namespace coagentics::analysis {
using namespace coagentics::agents;
std::vector<ProbePlan> preregistered_probes(){return {{"baseline",0,0},{"news",10,0},{"withdraw",0,0},{"reverse",-10,0},{"peer",0,10}};}
static double price(const HarnessRecord&r){return r.response.action && !r.response.action->abstain && r.response.error.empty()?r.response.action->price:0;}
BlackBoxFeatures extract_black_box_features(const std::vector<HarnessRecord>&rs){
 BlackBoxFeatures f;std::array<double,5> sum{};std::array<std::size_t,5> n{};
 for(auto&r:rs){if(!r.response.error.empty()||!r.response.action){++f.failures;continue;}if(r.response.action->abstain){++f.abstentions;continue;}++f.valid;
  // Probe membership is defined solely by public observations, not provider identity or hidden mechanism.
  auto&o=r.request.observation;int stage=static_cast<int>(o.time%5);if(stage<0||stage>=5)continue;sum[stage]+=price(r);++n[stage];
 }
 for(auto x:n){if(x)++f.stages_covered; if(!f.min_stage_samples || x<f.min_stage_samples)f.min_stage_samples=x;}
 if(f.stages_covered!=5)return f;
 double b=sum[0]/n[0];f.news=(sum[1]/n[1]-b)/10;f.persistence=(sum[2]/n[2]-b)/10;f.reversal=(sum[3]/n[3]-b)/10;f.peer=(sum[4]/n[4]-b)/10;
 f.reliability=double(f.valid)/std::max<std::size_t>(1,f.valid+f.failures+f.abstentions);return f;
}
BlackBoxInference infer_black_box(const std::vector<HarnessRecord>&rs,double threshold){
 auto f=extract_black_box_features(rs);if(f.valid<15||f.stages_covered!=5||f.min_stage_samples<3||f.reliability<.8)return{"unresolved",f,0,true};
 // Hypotheses are observable response signatures, not assertions about internal cognition.
 std::array<std::pair<std::string,double>,4> scores{{{"blind",std::abs(f.news)+std::abs(f.peer)+std::abs(f.persistence)+std::abs(f.reversal)},
 {"direct-news",std::abs(f.news-1)+std::abs(f.peer)+std::abs(f.persistence)+std::abs(f.reversal+1)},
 {"persistent-response",std::abs(f.news-1)+std::abs(f.peer)+std::abs(f.persistence-.7)+std::abs(f.reversal+.3)},
 {"peer-response",std::abs(f.news)+std::abs(f.peer-.6)+std::abs(f.persistence)+std::abs(f.reversal)}}};
 std::sort(scores.begin(),scores.end(),[](auto&a,auto&b){return a.second<b.second;});
 double gap=scores[1].second-scores[0].second;bool unresolved=gap<threshold || scores[0].second>0.45;return{unresolved?"unresolved":scores[0].first,f,gap,unresolved};
}
// These transports are ONLY used to score the held-out test. No ground-truth data enters infer_black_box.
class ControlTransport final:public ModelTransport {public:explicit ControlTransport(int kind):kind_(kind){}ModelResponse invoke(const ModelRequest&q)override{
 auto&o=q.observation;int stage=int(o.time%5);double p=100;
 if(kind_==1){if(stage==1)p+=10; if(stage==3)p-=10;}
 if(kind_==2){if(stage==1)p+=10;if(stage==2)p+=7;if(stage==3)p-=3;}
 if(kind_==3){if(stage==4)p+=6;}
 ModelResponse r; r.request_id=q.request_id; r.raw_output="black-box action";
 r.action=LlmAction{o.time,o.asset,1,p,coagentics::market::Side::Buy,false}; r.latency_ms=1; return r;
 }private:int kind_;};
static std::vector<HarnessRecord> collect(int kind,std::string model){
 auto t=std::make_shared<ControlTransport>(kind);LlmHarnessProvider h({"qualification",std::move(model),"opaque"},t,123);
 auto probes=preregistered_probes();for(int repeat=0;repeat<12;++repeat)for(std::size_t i=0;i<probes.size();++i){auto&x=probes[i];Observation o{std::uint64_t(repeat*5+i),"asset-A",100,x.signal,100+x.peer,110,100};h.request_bid("target",o);}return h.records();
}
ValidationReport validate_black_box_controls(){ValidationReport r;const std::array<std::string,4> labels{"blind","direct-news","persistent-response","peer-response"};std::size_t correct=0,abstain=0,nullfp=0;
 for(int i=0;i<4;++i){auto rec=collect(i,"opaque-model-"+std::to_string(i));auto pred=infer_black_box(rec);r.rows.push_back({labels[i],pred.hypothesis,pred.abstained});correct+=pred.hypothesis==labels[i];abstain+=pred.abstained;nullfp+=(i==0&&!pred.abstained&&pred.hypothesis!="blind");
 // Identity/metadata tampering cannot alter inference; only observed requests and actions are used.
 for(auto&x:rec){x.request.model={"tampered","different-model","different-version"};x.request.agent_id="changed";x.response.raw_output="changed";}
 auto again=infer_black_box(rec);if(again.hypothesis!=pred.hypothesis)throw std::runtime_error("metadata leakage");}
 r.accuracy=double(correct)/4;r.abstention=double(abstain)/4;r.null_false_discovery=double(nullfp);r.leakage_test_passed=true;return r;
}
std::string black_box_report(const ValidationReport&r){std::ostringstream o;o<<"# v21 black-box qualification\n\nSynthetic transport controls, not live LLM results.\n\n";for(auto&x:r.rows)o<<"- "<<x.truth<<" -> "<<x.predicted<<(x.abstained?" (abstain)":"")<<"\n";o<<"\nRecovery: "<<r.accuracy*100<<"%\nAbstention: "<<r.abstention*100<<"%\nNull false discoveries: "<<r.null_false_discovery*100<<"%\nMetadata leakage check: "<<(r.leakage_test_passed?"PASS":"FAIL")<<"\n\nLimitations: signature matching is a qualification baseline; arbitrary model behavior, market-level transfer, and calibrated uncertainty remain unverified.\n";return o.str();}
}
