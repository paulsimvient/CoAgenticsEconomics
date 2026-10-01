#include "coagentics/analysis/MarketTemporalValidation.hpp"
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
using Features=std::array<double,5>;
constexpr Features news={8,8,0,-8,0};
constexpr Features peer={0,0,0,0,8};
double squared_distance(const Features&a,const Features&b){double x=0;for(size_t j=0;j<5;++j)x+=(a[j]-b[j])*(a[j]-b[j]);return x;}
}
Features market_temporal_trajectory(const std::string& mechanism,std::uint64_t seed,int quote_events){
 if(quote_events<1)throw std::invalid_argument("quote_events must be positive");
 if(std::find_if(names.begin(),names.end(),[&](const char* n){return mechanism==n;})==names.end())throw std::invalid_argument("unknown mechanism");
 Features f{};double memory=0;
 for(size_t k=0;k<5;++k){
  experiment::Probe probe{"market-temporal-session",news[k],peer[k],1};
  std::string market_policy=mechanism;
  if(mechanism=="adaptive"){
   // The persistent policy state is updated by the actual treatment information.
   // Feed the effective response into the existing full-CDA signal-responsive target.
   memory=.65*memory+.35*news[k];probe.news=news[k]+.8*memory;market_policy="signal-responsive";
  }
  // Each session is an independent CDA with a matched activation/quote tape;
  // agent adaptation is the only state retained between sessions.
  experiment::CdaMarketDomain market(market_policy,quote_events);
  auto pair=market.run_market(probe,seed+7919*k);
  // Normalize by intervention size, as in v18. A missing target quote is an
  // observable abstention (NaN), not an invented zero effect.
  if(pair.control.target_quotes==0||pair.treatment.target_quotes==0)throw std::runtime_error("insufficient target quotes: increase quote_events");
  f[k]=(pair.treatment.mean_target_quote-pair.control.mean_target_quote)/8.;
 }
 return f;
}
MarketTemporalReport validate_market_temporal(const MarketTemporalConfig& cfg){
 if(!cfg.training_trials||!cfg.heldout_trials||cfg.training_seed==cfg.heldout_seed||cfg.abstain_margin<0)throw std::invalid_argument("invalid validation configuration");
 MarketTemporalReport r;r.protocol="Five sequential 12-trader CDA sessions: news pulse, repeated news, washout, reversal, peer-only. Paired common-random-number control/treatment within each session; persistent adaptive target state; train-only centroids frozen before disjoint held-out seeds. This is a controlled-mechanism qualification, not live LLM validation. Quote composition can change after interventions.";
 std::array<Features,5> centers{};
 for(size_t m=0;m<5;++m)for(size_t t=0;t<cfg.training_trials;++t){auto x=market_temporal_trajectory(names[m],cfg.training_seed+100000*m+t,cfg.quote_events);for(size_t k=0;k<5;++k)centers[m][k]+=x[k]/cfg.training_trials;}
 size_t correct=0,abstained=0,fp=0;
 for(size_t m=0;m<5;++m){MarketTemporalRow row;row.mechanism=names[m];row.total=cfg.heldout_trials;
  for(size_t t=0;t<cfg.heldout_trials;++t){auto x=market_temporal_trajectory(names[m],cfg.heldout_seed+100000*m+t,cfg.quote_events);std::array<std::pair<double,size_t>,5> scores{};for(size_t j=0;j<5;++j)scores[j]={squared_distance(x,centers[j]),j};std::sort(scores.begin(),scores.end());if(scores[1].first-scores[0].first<cfg.abstain_margin){++row.abstained;++abstained;continue;}if(scores[0].second==m){++row.correct;++correct;}else if(m==0){++row.null_false_positive;++fp;}
  }row.recovery=double(row.correct)/row.total;r.rows.push_back(row);
 }
 r.heldout_recovery=double(correct)/(5*cfg.heldout_trials);r.null_false_discovery=double(fp)/cfg.heldout_trials;r.abstention=double(abstained)/(5*cfg.heldout_trials);r.recovery_gate=r.heldout_recovery>=.8;r.null_gate=r.null_false_discovery<=.05;return r;
}
std::string market_temporal_markdown(const MarketTemporalReport&r){std::ostringstream o;o<<"# CoAgentics v19 — Temporal interventions inside the full CDA\n\n"<<r.protocol<<"\n\n| Mechanism | Held-out recovery | Abstained | Trials |\n|---|---:|---:|---:|\n";for(auto&x:r.rows)o<<"| "<<x.mechanism<<" | "<<std::fixed<<std::setprecision(1)<<100*x.recovery<<"% | "<<x.abstained<<" | "<<x.total<<" |\n";o<<"\nOverall recovery "<<100*r.heldout_recovery<<"%; null false discoveries "<<100*r.null_false_discovery<<"%; abstention "<<100*r.abstention<<"%.\n\nGates: recovery "<<(r.recovery_gate?"PASS":"FAIL")<<", null "<<(r.null_gate?"PASS":"FAIL")<<".\n\n**Limits:** Full market dynamics, but independent market resets between sessions; only the adaptive target's state persists. The five marginal schedules remain analogues rather than exact published schedules. No live models or human behavioral baseline are involved.\n";return o.str();}
std::string market_temporal_json(const MarketTemporalReport&r){std::ostringstream o;o<<std::boolalpha<<"{\"heldout_recovery\":"<<r.heldout_recovery<<",\"null_false_discovery\":"<<r.null_false_discovery<<",\"abstention\":"<<r.abstention<<",\"recovery_gate\":"<<r.recovery_gate<<",\"null_gate\":"<<r.null_gate<<",\"mechanisms\":[";for(size_t i=0;i<r.rows.size();++i){if(i)o<<",";auto&x=r.rows[i];o<<"{\"name\":\""<<x.mechanism<<"\",\"correct\":"<<x.correct<<",\"total\":"<<x.total<<",\"abstained\":"<<x.abstained<<"}";}o<<"]}\n";return o.str();}
}
