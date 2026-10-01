#include "coagentics/analysis/ScientistCalibration.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <numeric>
#include <sstream>
namespace coagentics::analysis {
using namespace coagentics::experiment;
namespace {
std::vector<Candidate> candidates(){return {{"blind",0,0,0,.2},{"news",1,0,0,.2},{"risk",.75,0,0,.2},{"peer",0,.6,0,.2},{"adaptive",1,0,.15,.2}};}
std::vector<Probe> probes(){return {{"news-only",8,0,1},{"peer-only",0,8,1},{"repeated-news",8,0,4},{"news-peer",8,8,1}};}
struct Truth {const char* policy;const char* hypothesis;};
constexpr Truth truths[]={{"information-blind","blind"},{"signal-responsive","news"},{"risk-sensitive","risk"},{"peer-responsive","peer"},{"adaptive","adaptive"}};
struct Trial {std::string mechanism,truth,pred;double ptrue{},brier{};bool separated{};};
Trial run_trial(const Truth&t,std::uint64_t seed,double noise,const CalibrationConfig&cc){
 CdaMarketDomain d(t.policy,cc.quote_events);ScientistConfig sc;sc.replicates=cc.replicates;sc.max_experiments=4;sc.noise_sd=noise;sc.stop_posterior=cc.decision_threshold;sc.minimum_separation=.05;
 auto r=run_market_scientist(d,candidates(),probes(),seed,sc);auto cs=candidates();std::vector<double> post(cs.size(),1.0/cs.size());if(!r.scientist.steps.empty())post=r.scientist.steps.back().posterior;
 size_t ti=0;for(;ti<cs.size();++ti)if(cs[ti].id==t.hypothesis)break;size_t pi=std::distance(post.begin(),std::max_element(post.begin(),post.end()));double b=0;for(size_t i=0;i<post.size();++i){double y=i==ti?1.:0.;b+=(post[i]-y)*(post[i]-y);}b/=post.size();
 return {t.policy,t.hypothesis,cs[pi].id,post[ti],b,!r.scientist.steps.empty()&&*std::max_element(post.begin(),post.end())>=cc.decision_threshold};
}
double training_score(double noise,const CalibrationConfig&cc){double sum=0;size_t n=0;for(const auto&t:truths)for(size_t k=0;k<cc.train_trials;++k){auto x=run_trial(t,cc.train_seed+10000*n+k,noise,cc);sum+=x.brier;++n;}return sum/n;}
std::string esc(const std::string&s){std::string o;for(char c:s){if(c=='"'||c=='\\')o+='\\';o+=c;}return o;}
}
ScientistCalibrationReport validate_market_scientist(const CalibrationConfig&cc){
 ScientistCalibrationReport r;const double grid[]={.5,1.,1.5,2.,3.,4.,6.,8.};r.training_brier=std::numeric_limits<double>::infinity();for(double n:grid){double s=training_score(n,cc);if(s<r.training_brier){r.training_brier=s;r.calibrated_noise_sd=n;}}
 std::vector<Trial> all;size_t idx=0;for(const auto&t:truths)for(size_t k=0;k<cc.test_trials;++k)all.push_back(run_trial(t,cc.test_seed+10000*idx+k,r.calibrated_noise_sd,cc)),++idx;
 r.heldout_trials=all.size();double bsum=0;size_t correct=0,null_fp=0,null_n=0;for(const auto&x:all){bsum+=x.brier;if(x.pred==x.truth)++correct;if(x.truth=="blind"){++null_n;if(x.separated&&x.pred!="blind")++null_fp;}}
 r.heldout_brier=bsum/all.size();r.heldout_recovery_rate=double(correct)/all.size();r.null_false_discovery_rate=null_n?double(null_fp)/null_n:0;
 for(const auto&t:truths){MechanismValidation m;m.mechanism=t.policy;m.expected_hypothesis=t.hypothesis;double pb=0,bb=0;for(const auto&x:all)if(x.mechanism==t.policy){++m.trials;if(x.pred==x.truth)++m.correct;if(x.separated)++m.separated;pb+=x.ptrue;bb+=x.brier;}m.recovery_rate=m.trials?double(m.correct)/m.trials:0;m.mean_true_probability=m.trials?pb/m.trials:0;m.brier_score=m.trials?bb/m.trials:0;r.mechanisms.push_back(m);}
 r.calibration_pass=r.heldout_brier<=r.training_brier+.05;r.null_control_pass=r.null_false_discovery_rate<=.05;r.recovery_pass=r.heldout_recovery_rate>=.80;return r;
}
std::string scientist_calibration_markdown(const ScientistCalibrationReport&r){std::ostringstream o;o<<"# CoAgentics v17 — Scientist calibration and held-out validation\n\n";
 o<<"The automated scientist is calibrated on one seed partition and evaluated on a disjoint held-out partition. No live LLMs or human subjects are used in this qualification.\n\n";
 o<<"| Measure | Result |\n|---|---:|\n"<<"| Calibrated measurement noise SD | "<<r.calibrated_noise_sd<<" |\n| Training Brier score | "<<std::fixed<<std::setprecision(4)<<r.training_brier<<" |\n| Held-out Brier score | "<<r.heldout_brier<<" |\n| Held-out mechanism recovery | "<<100*r.heldout_recovery_rate<<"% |\n| Null false-discovery rate | "<<100*r.null_false_discovery_rate<<"% |\n\n";
 o<<"| Controlled mechanism | Expected explanation | Recovery | Mean P(true) | Brier |\n|---|---|---:|---:|---:|\n";for(const auto&m:r.mechanisms)o<<"| "<<m.mechanism<<" | "<<m.expected_hypothesis<<" | "<<100*m.recovery_rate<<"% | "<<m.mean_true_probability<<" | "<<m.brier_score<<" |\n";
 o<<"\nQualification gates: calibration="<<(r.calibration_pass?"PASS":"FAIL")<<", null false-discovery="<<(r.null_control_pass?"PASS":"FAIL")<<", mechanism recovery="<<(r.recovery_pass?"PASS":"FAIL")<<".\n\n";
 o<<"Interpretation boundary: these rates validate recovery of five known in-process mechanisms under the current CDA, probe set, priors, and seed distribution. They do not validate inference of latent intent in arbitrary models. A failure to separate remains a valid scientific outcome.\n";return o.str();}
std::string scientist_calibration_json(const ScientistCalibrationReport&r){std::ostringstream o;o<<std::boolalpha<<"{\n  \"calibrated_noise_sd\": "<<r.calibrated_noise_sd<<",\n  \"training_brier\": "<<r.training_brier<<",\n  \"heldout_brier\": "<<r.heldout_brier<<",\n  \"heldout_recovery_rate\": "<<r.heldout_recovery_rate<<",\n  \"null_false_discovery_rate\": "<<r.null_false_discovery_rate<<",\n  \"heldout_trials\": "<<r.heldout_trials<<",\n  \"calibration_pass\": "<<r.calibration_pass<<",\n  \"null_control_pass\": "<<r.null_control_pass<<",\n  \"recovery_pass\": "<<r.recovery_pass<<",\n  \"mechanisms\": [\n";for(size_t i=0;i<r.mechanisms.size();++i){auto&m=r.mechanisms[i];o<<"    {\"mechanism\": \""<<esc(m.mechanism)<<"\", \"truth\": \""<<esc(m.expected_hypothesis)<<"\", \"recovery_rate\": "<<m.recovery_rate<<", \"mean_true_probability\": "<<m.mean_true_probability<<", \"brier\": "<<m.brier_score<<"}"<<(i+1<r.mechanisms.size()?",":"")<<"\n";}o<<"  ]\n}\n";return o.str();}
}
