#include "coagentics/analysis/DecisionTraceValidation.hpp"
#include "coagentics/experiment/MarketScientist.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
namespace coagentics::analysis {
namespace {
constexpr std::array<const char*,5> names={"information-blind","signal-responsive","risk-sensitive","peer-responsive","adaptive"};
constexpr std::array<double,5> news={8,8,0,-8,0};
constexpr std::array<double,5> peer={0,0,0,0,8};
using F=std::array<double,5>;
double distance(const F&a,const F&b){double x=0;for(size_t j=0;j<5;++j)x+=(a[j]-b[j])*(a[j]-b[j]);return x;}
}
TraceFeatures decision_trace_trajectory(const std::string& mechanism,std::uint64_t seed,int quote_events){
 if(quote_events<1||std::find_if(names.begin(),names.end(),[&](const char* n){return mechanism==n;})==names.end())throw std::invalid_argument("invalid trace experiment");
 TraceFeatures out;double memory=0;
 for(size_t k=0;k<5;++k){
  experiment::Probe p{"trace-temporal",news[k],peer[k],1};std::string policy=mechanism;
  if(mechanism=="adaptive"){
   memory=.65*memory+.35*news[k];p.news=news[k]+.8*memory;policy="signal-responsive";
  }
  experiment::CdaMarketDomain domain(policy,quote_events);
  auto pair=domain.run_traced(p,seed+7919*k);
  // Event-aligned comparisons remove changing quote composition. An emitted
  // desired action is measured even when the market clamps/rejects its quote.
  // No fabricated response when one side has no legal action at that event.
  size_t i=0,j=0;double sum=0;
  for(const auto& x:pair.control){if(x.actor!=0)continue;if(x.legal)++out.control_legal[k];if(x.attempted)++out.control_attempts[k];}
  for(const auto& x:pair.treatment){if(x.actor!=0)continue;if(x.legal)++out.treatment_legal[k];if(x.attempted)++out.treatment_attempts[k];}
  while(i<pair.control.size()&&j<pair.treatment.size()){
   const auto&a=pair.control[i];const auto&b=pair.treatment[j];
   if(a.event<b.event){++i;continue;}if(b.event<a.event){++j;continue;}
   if(a.actor==0&&b.actor==0&&a.legal&&b.legal&&a.attempted&&b.attempted){
    // Reconstruct common random-number baseline from each branch's own
    // prevailing bid/ask and private limit, not a counterfactual hidden state.
    double alo=a.buyer?a.standing_bid:a.private_limit;
    double ahi=a.buyer?a.private_limit:a.standing_ask;
    double blo=b.buyer?b.standing_bid:b.private_limit;
    double bhi=b.buyer?b.private_limit:b.standing_ask;
    double da=a.desired_price-(alo+(ahi-alo)*a.draw);
    double db=b.desired_price-(blo+(bhi-blo)*b.draw);
    sum+=db-da;++out.matched[k];
   }
   ++i;++j;
  }
  if(out.matched[k])out.response[k]=sum/(8.*out.matched[k]);
 }
 return out;
}
TraceReport validate_decision_traces(const TraceConfig&cfg){
 if(!cfg.training_trials||!cfg.heldout_trials||cfg.training_seed==cfg.heldout_seed||cfg.abstain_margin<0||!cfg.min_matched)throw std::invalid_argument("invalid trace validation configuration");
 TraceReport r;r.protocol="Five full-CDA temporal sessions with event-level decision traces. The observation is each agent's emitted desired quote before institutional clamping, aligned on identical activation events. Local market-state and random-draw effects are subtracted separately within each paired branch. Train-only centroids are frozen before disjoint held-out seeds. Missing matched legal actions cause abstention. This is controlled-mechanism recovery, not live LLM validation.";
 std::array<F,5> centers{};
 for(size_t m=0;m<5;++m)for(size_t t=0;t<cfg.training_trials;++t){auto x=decision_trace_trajectory(names[m],cfg.training_seed+100000*m+t,cfg.quote_events);for(size_t k=0;k<5;++k){if(x.matched[k]<cfg.min_matched)throw std::runtime_error("insufficient training matched actions");centers[m][k]+=x.response[k]/cfg.training_trials;}}
 size_t correct=0,abstained=0,fp=0;double coverage=0;
 for(size_t m=0;m<5;++m){TraceRow row;row.mechanism=names[m];row.total=cfg.heldout_trials;
  for(size_t t=0;t<cfg.heldout_trials;++t){auto x=decision_trace_trajectory(names[m],cfg.heldout_seed+100000*m+t,cfg.quote_events);bool enough=true;for(size_t k=0;k<5;++k){enough&=x.matched[k]>=cfg.min_matched;coverage+=double(x.matched[k])/std::max<std::size_t>(1,x.control_attempts[k]);}
   if(!enough){++row.abstained;++abstained;continue;}
   std::array<std::pair<double,size_t>,5> scores{};for(size_t j=0;j<5;++j)scores[j]={distance(x.response,centers[j]),j};std::sort(scores.begin(),scores.end());
   if(scores[1].first-scores[0].first<cfg.abstain_margin){++row.abstained;++abstained;continue;}
   if(scores[0].second==m){++row.correct;++correct;}else if(m==0){++row.false_positive;++fp;}
  }
  row.recovery=double(row.correct)/row.total;r.rows.push_back(row);
 }
 r.recovery=double(correct)/(5*cfg.heldout_trials);r.null_false_discovery=double(fp)/cfg.heldout_trials;r.abstention=double(abstained)/(5*cfg.heldout_trials);r.mean_matched_coverage=coverage/(25*cfg.heldout_trials);r.recovery_gate=r.recovery>=.8;r.null_gate=r.null_false_discovery<=.05;return r;
}
std::string trace_validation_markdown(const TraceReport&r){std::ostringstream o;o<<"# CoAgentics v20 — Full-market decision observability\n\n"<<r.protocol<<"\n\n| Mechanism | Held-out recovery | Abstained | Trials |\n|---|---:|---:|---:|\n";for(auto&x:r.rows)o<<"| "<<x.mechanism<<" | "<<std::fixed<<std::setprecision(1)<<100*x.recovery<<"% | "<<x.abstained<<" | "<<x.total<<" |\n";o<<"\nOverall recovery "<<100*r.recovery<<"%; null false discoveries "<<100*r.null_false_discovery<<"%; abstention "<<100*r.abstention<<"%; matched legal-action coverage "<<100*r.mean_matched_coverage<<"%.\n\nGates: recovery "<<(r.recovery_gate?"PASS":"FAIL")<<", null "<<(r.null_gate?"PASS":"FAIL")<<".\n\n**Important:** The emitted desired quote is observable in this controlled agent adapter, not inferred from executed trades. Live-model integration must log raw actions before validation. The intervention signal is known to the experimenter. Five market schedules remain analogues. No live LLMs, human subjects, or exact historical market replication.\n";return o.str();}
std::string trace_validation_json(const TraceReport&r){std::ostringstream o;o<<std::boolalpha<<"{\"recovery\":"<<r.recovery<<",\"null_false_discovery\":"<<r.null_false_discovery<<",\"abstention\":"<<r.abstention<<",\"matched_coverage\":"<<r.mean_matched_coverage<<",\"recovery_gate\":"<<r.recovery_gate<<",\"null_gate\":"<<r.null_gate<<",\"mechanisms\":[";for(size_t i=0;i<r.rows.size();++i){if(i)o<<",";const auto&x=r.rows[i];o<<"{\"name\":\""<<x.mechanism<<"\",\"correct\":"<<x.correct<<",\"total\":"<<x.total<<",\"abstained\":"<<x.abstained<<"}";}o<<"]}\n";return o.str();}
}
