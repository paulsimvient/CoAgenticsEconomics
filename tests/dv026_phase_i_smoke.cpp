#include "coagentics/experiment/Dv026PhaseI.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
using namespace coagentics::experiment;

static void require(bool ok, const std::string& msg){
 if(!ok) throw std::runtime_error(msg);
}

int main(){
 try{
  auto rep=run_phase_i_qualification_suite(424242);
  require(rep.software_green, "software_green");
  require(rep.phase_ii_deferred, "phase II deferred");
  require(rep.checks.size()>=12, "expected A/B/C/D checks");
  // darpa_claim_ready may be true when results/dv026_ollama_campaign/summary.json
  // already passed the local_ollama_poc gate (host evidence). CI without artifacts stays false.

  bool saw_a=false, saw_b=false, saw_c=false, saw_d=false;
  bool saw_d6=false, saw_not_claimed_p2=false;
  for(const auto& c:rep.checks){
   if(c.status=="FAIL") throw std::runtime_error("failed check: "+c.id);
   if(c.group==PhaseIGroup::MarketMechanics) saw_a=true;
   if(c.group==PhaseIGroup::LlmInterface) saw_b=true;
   if(c.group==PhaseIGroup::MultiAgent) saw_c=true;
   if(c.group==PhaseIGroup::Dv026Qualification) saw_d=true;
   if(c.id=="D6_LIVE_10_LLM_ECONOMIC"){
    require(c.status=="NOT_CLAIMED"||c.status=="PARTIAL"||c.status=="PASS",
     "D6 status must be NOT_CLAIMED|PARTIAL|PASS");
    if(c.status=="PASS"){
     require(c.evidence.find("local_ollama_poc")!=std::string::npos, "PASS implies local_ollama_poc");
     require(rep.darpa_claim_ready, "D6 PASS implies suite darpa_claim_ready");
    }else{
     require(!rep.darpa_claim_ready, "without D6 PASS suite stays not ready");
    }
    saw_d6=true;
   }
   if(c.id=="D7_PHASE_II_BIAS_DECEPTION"){
    require(c.status=="NOT_CLAIMED", "phase II remains NOT_CLAIMED");
    saw_not_claimed_p2=true;
   }
  }
  require(saw_a && saw_b && saw_c && saw_d, "all four groups present");
  require(saw_d6 && saw_not_claimed_p2, "D6 present and D7 NOT_CLAIMED");
  require(rep.claim_boundary.find("FAQ 31")!=std::string::npos, "faq31 boundary");
  require(rep.evidence.records().size()==1, "suite evidence");

  auto md=phase_i_suite_markdown(rep);
  require(md.find("DARPA claim ready:")!=std::string::npos, "markdown claim line");
  auto js=phase_i_suite_json(rep);
  if(rep.darpa_claim_ready){
   require(js.find("\"darpa_claim_ready\":true")!=std::string::npos, "json claim true");
   require(js.find("\"scope\":\"local_ollama_poc\"")!=std::string::npos ||
           js.find("local_ollama_poc")!=std::string::npos, "scope local_ollama_poc");
  }else{
   require(js.find("\"darpa_claim_ready\":false")!=std::string::npos, "json claim false");
  }

  std::cout<<"dv026_phase_i_smoke ok checks="<<rep.checks.size()
   <<" darpa_claim_ready="<<std::boolalpha<<rep.darpa_claim_ready<<"\n";
  return 0;
 }catch(const std::exception& e){
  std::cerr<<"dv026_phase_i_smoke FAIL: "<<e.what()<<"\n";
  return 1;
 }
}
