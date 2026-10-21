#include "coagentics/agents/LlmHarness.hpp"
#include <cassert>
#include <filesystem>
#include <iostream>
using namespace coagentics::agents; using coagentics::market::Side;
int main(){
 ModelRegistry reg; for(int i=0;i<10;i++)reg.add({"provider"+std::to_string(i%3),"model"+std::to_string(i),"v1"}); assert(reg.models().size()==10);
 Observation o{7,"asset-A",100,4,98,102,100};
 std::vector<LlmAction> actions{{7,"asset-A",2,101,Side::Buy,false},{8,"WRONG",1,100,Side::Buy,false},{7,"asset-A",0,0,Side::Buy,true}};
 auto t=std::make_shared<ScriptedTransport>(actions); std::string log="llm_harness_test.jsonl"; std::filesystem::remove(log);
 LlmHarnessProvider p({"test-provider","test-model","2026-09"},t,42,log);
 auto b1=p.request_bid("llm-1",o); assert(b1.quantity==2&&b1.price==101); auto b2=p.request_bid("llm-1",o); assert(b2.quantity==0); auto b3=p.request_bid("llm-1",o); assert(b3.quantity==0); assert(p.failures()==1);assert(p.abstentions()==1);assert(p.records().size()==3);assert(std::filesystem::file_size(log)>0);
 std::vector<ModelResponse> rr; rr.push_back({"","replayed",LlmAction{7,"asset-A",1,99,Side::Sell,false},"",3}); auto rt=std::make_shared<ReplayTransport>(rr);LlmHarnessProvider replay({"replay","m","v"},rt,42);auto rb=replay.request_bid("r",o);assert(rb.quantity==1&&rb.price==99&&rb.side==Side::Sell);
 std::cout<<"registry_models="<<reg.models().size()<<" failures="<<p.failures()<<" abstentions="<<p.abstentions()<<" records="<<p.records().size()<<"\n";
}
