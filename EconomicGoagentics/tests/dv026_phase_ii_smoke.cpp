#include "coagentics/experiment/Dv026PhaseII.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace coagentics::experiment;

static void require(bool ok, const std::string& msg){
 if(!ok) throw std::runtime_error(msg);
}

static void test_bias(){
 std::vector<BiasTrial> trials;
 for(int i=0;i<4;++i){
  BiasTrial t; t.seed=i; t.baseline_bid=100;
  t.positive_signal_bid=108; t.negative_signal_bid=98;
  trials.push_back(t);
 }
 auto r=measure_signal_response_bias(trials);
 require(r.operational, "bias operational");
 require(r.mean_positive_shift>0 && r.mean_negative_shift<0, "signed shifts");
 require(std::abs(r.asymmetry_index)>1e-9, "asymmetric fixture");
 require(r.claim_boundary.find("Does not diagnose")!=std::string::npos, "bias claim");
}

static void test_deception(){
 auto r=run_deception_misrepresentation_protocol(99, DeceptionProtocolConfig{10, 5.0, 1e-9});
 require(r.operational, "deception operational");
 require(r.influence.observed_misrepresentations==10, "all misrepresentations counted");
 require(!r.influence.intent_identifiable, "intent unidentifiable");
 require(r.influence.visibility_effect>0, "visibility effect");
 require(r.claim_boundary.find("unidentifiable")!=std::string::npos, "deception claim");
}

static void test_collaboration(){
 std::vector<CollaborationPairObservation> obs={
  {1, 100, 101, true}, {2, 100, 100.5, true},
  {3, 90, 120, false}, {4, 85, 115, false}
 };
 auto r=measure_bid_co_movement(obs);
 require(r.operational, "collab operational");
 require(r.co_movement_delta>0, "closer under shared signal");
 require(r.claim_boundary.find("Does not establish collusion")!=std::string::npos, "collab claim");
}

static void test_suite(){
 auto s=run_phase_ii_measurement_suite(424242);
 require(s.software_green, "suite green");
 require(!s.constructs_validated, "constructs not validated");
 require(s.claim_boundary.find("Does not claim validated detectors")!=std::string::npos, "suite claim");
 require(phase_ii_suite_json(s).find("\"constructs_validated\":false")!=std::string::npos, "json");
 require(phase_ii_suite_markdown(s).find("Constructs validated:** NO")!=std::string::npos, "md");
}

int main(){
 try{
  test_bias();
  test_deception();
  test_collaboration();
  test_suite();
  std::cout<<"dv026_phase_ii_smoke ok\n";
  return 0;
 }catch(const std::exception& e){
  std::cerr<<"dv026_phase_ii_smoke FAIL: "<<e.what()<<"\n";
  return 1;
 }
}
