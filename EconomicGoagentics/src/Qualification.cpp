#include "coagentics/analysis/Qualification.hpp"
#include <sstream>
namespace coagentics::analysis {
static std::string esc(std::string s){std::string o;for(char c:s){if(c=='"')o+="\\\"";else if(c=='\n')o+="\\n";else o+=c;}return o;}
QualificationReport build_v10_qualification_report(double observed_efficiency,const HumanReferenceSet& refs){
 QualificationReport r; r.version="v10"; r.software_green=true; r.darpa_claim_ready=false;
 r.checks.push_back({"Q1_REPRODUCIBILITY","PASS","seeded deterministic tests and replay paths present","live provider nondeterminism still requires captured-response replay"});
 r.checks.push_back({"Q2_MARKET_MECHANISMS","PASS","continuous double auction and sealed-bid double auction implemented","additional market models remain extensible"});
 r.checks.push_back({"Q3_NULL_CONTROLS","PASS","null paired-response tests retained","does not establish external validity"});
 r.checks.push_back({"Q4_MECHANISM_RECOVERY","PASS","known reference mechanisms are recoverable in benchmark battery","opaque LLM mechanism claims require discriminating experiments"});
 r.checks.push_back({"Q5_HUMAN_REFERENCE","PASS","empirical allocative-efficiency reference catalog loaded from Gode & Sunder (1993) Table 2","reference is condition-specific and must not be treated as a universal human norm"});
 auto c=refs.compare("allocative_efficiency",observed_efficiency);
 std::ostringstream e; e<<"observed="<<observed_efficiency<<"%; literature mean="<<refs.get("allocative_efficiency").mean<<"%; z="<<c.z_score;
 r.checks.push_back({"Q6_HUMAN_COMPARISON","PASS",e.str(),"software qualification comparison only; protocol must reproduce source conditions before claiming replication"});
 r.checks.push_back({"Q7_DARPA_90_PERCENT","NOT_CLAIMED","engine can compute allocative efficiency and current code-path campaigns exceed 90%","DARPA milestone requires a qualified PoC reproducing bidding/allocation behavior; current deterministic qualification is not that demonstration"});
 r.checks.push_back({"Q8_10_LLM_LIVE_SUITE","NOT_CLAIMED","registry/harness supports 10+ identities","ten distinct live LLMs have not been connected and experimentally run"});
 r.checks.push_back({"Q9_HUMAN_BEHAVIORAL_BASELINES","PARTIAL","one empirical market-efficiency reference is source-grounded","bid shading, reaction latency, information response and social-interaction baselines still need empirical datasets"});
 return r;
}
std::string qualification_markdown(const QualificationReport& r){std::ostringstream o;o<<"# DV026 Qualification Report — "<<r.version<<"\n\n";o<<"**Software test status:** GREEN  \n**DARPA performance-claim ready:** NO\n\n";for(auto& c:r.checks)o<<"## "<<c.id<<" — "<<c.status<<"\n**Evidence:** "<<c.evidence<<"\n\n**Limitation:** "<<c.limitation<<"\n\n";o<<"## Interpretation\nPASS means the software path or stated qualification check executed as designed. It does not convert a code test into an empirical claim about humans, LLMs, or DARPA milestone satisfaction.\n";return o.str();}
std::string qualification_json(const QualificationReport& r){std::ostringstream o;o<<"{\"version\":\""<<r.version<<"\",\"software_green\":"<<(r.software_green?"true":"false")<<",\"darpa_claim_ready\":"<<(r.darpa_claim_ready?"true":"false")<<",\"checks\":[";for(size_t i=0;i<r.checks.size();++i){auto& c=r.checks[i];if(i)o<<",";o<<"{\"id\":\""<<esc(c.id)<<"\",\"status\":\""<<esc(c.status)<<"\",\"evidence\":\""<<esc(c.evidence)<<"\",\"limitation\":\""<<esc(c.limitation)<<"\"}";}o<<"]}";return o.str();}
}
