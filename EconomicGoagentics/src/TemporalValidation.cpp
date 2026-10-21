#include "coagentics/analysis/TemporalValidation.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
namespace coagentics::analysis {
namespace {
// This independent controlled temporal-policy chamber is a diagnostic adapter;
// the v16 full CDA remains the market-level validation environment.
constexpr std::array<const char*,5> names={"information-blind","signal-responsive","risk-sensitive","peer-responsive","adaptive"};
using Features=std::array<double,5>;
// Paired, common-random-number probes: initial pulse, repeated exposure,
// washout, reversed signal, and peer-only. Measurements are changes from
// matched no-intervention control at the same time index.
Features trajectory(std::size_t mechanism,std::uint64_t seed){
 std::mt19937_64 rng(seed);std::normal_distribution<double> observation(0.,.10);
 Features out{};double memory=0;constexpr std::array<double,5> news={8.,8.,0.,-8.,0.};constexpr std::array<double,5> peer={0.,0.,0.,0.,8.};
 for(std::size_t k=0;k<out.size();++k){
  double response=0;
  switch(mechanism){
   case 0:response=0;break;
   case 1:response=news[k];break;
   case 2:response=.75*news[k];break;
   case 3:response=.6*peer[k];break;
   case 4:memory=.65*memory+.35*news[k];response=news[k]+.8*memory;break;
   default:throw std::invalid_argument("unknown mechanism");
  }
  // Matched noise is intentionally shared between control and treatment;
  // residual observation error is retained to prevent perfect fixtures.
  double common=observation(rng);double residual=observation(rng);
  out[k]=((100+common+response+residual)-(100+common))/8.;
 }
 return out;
}
double distance(const Features&a,const Features&b){double s=0;for(size_t i=0;i<a.size();++i){double d=a[i]-b[i];s+=d*d;}return s;}
}
TemporalReport validate_temporal_scientist(const TemporalConfig&cfg){
 if(!cfg.train_trials||!cfg.heldout_trials||cfg.train_seed==cfg.heldout_seed||cfg.abstain_margin<0)throw std::invalid_argument("invalid temporal validation configuration");
 TemporalReport report;report.protocol="Train-only centroids; frozen before disjoint held-out seeds. Five paired temporal probes: pulse, repeated news, washout, reversal, peer-only. Abstain on small nearest-vs-second-nearest distance margin. Controlled policy adapter, not live LLM or full CDA.";
 std::array<Features,5> centroids{};
 for(size_t m=0;m<5;++m)for(size_t t=0;t<cfg.train_trials;++t){auto x=trajectory(m,cfg.train_seed+100000*m+t);for(size_t j=0;j<5;++j)centroids[m][j]+=x[j]/cfg.train_trials;}
 size_t correct=0,false_positive=0,abstained=0;
 for(size_t m=0;m<5;++m){TemporalMechanism row;row.name=names[m];row.total=cfg.heldout_trials;
  for(size_t t=0;t<cfg.heldout_trials;++t){auto x=trajectory(m,cfg.heldout_seed+100000*m+t);std::array<std::pair<double,size_t>,5> scores{};for(size_t j=0;j<5;++j)scores[j]={distance(x,centroids[j]),j};std::sort(scores.begin(),scores.end());bool abstain=(scores[1].first-scores[0].first)<cfg.abstain_margin;
   if(abstain){++row.abstained;++abstained;continue;}
   if(scores[0].second==m){++row.correct;++correct;}
   if(m==0&&scores[0].second!=0)++false_positive;
  }
  row.recovery=double(row.correct)/row.total;report.mechanisms.push_back(row);
 }
 report.recovery=double(correct)/(5*cfg.heldout_trials);report.abstention=double(abstained)/(5*cfg.heldout_trials);report.null_false_discovery=double(false_positive)/cfg.heldout_trials;
 report.recovery_gate=report.recovery>=.80;report.null_gate=report.null_false_discovery<=.05;return report;
}
std::string temporal_markdown(const TemporalReport&r){std::ostringstream o;o<<"# CoAgentics v18 — Temporal identifiability qualification\n\n"<<r.protocol<<"\n\n| Mechanism | Recovery | Abstentions | Trials |\n|---|---:|---:|---:|\n";for(auto&m:r.mechanisms)o<<"| "<<m.name<<" | "<<std::fixed<<std::setprecision(1)<<100*m.recovery<<"% | "<<m.abstained<<" | "<<m.total<<" |\n";o<<"\nOverall held-out recovery: "<<100*r.recovery<<"%; null false discoveries: "<<100*r.null_false_discovery<<"%; abstention: "<<100*r.abstention<<"%.\n\nGates: >=80% recovery "<<(r.recovery_gate?"PASS":"FAIL")<<"; <=5% null false discovery "<<(r.null_gate?"PASS":"FAIL")<<".\n\n**Scope:** This validates the discriminating temporal protocol against controlled mechanisms in an independent policy adapter. It does not replace v17 full-market held-out results; temporal interventions still need to be integrated into and revalidated in the nonlinear CDA. No live LLMs or human participants were used.\n";return o.str();}
std::string temporal_json(const TemporalReport&r){std::ostringstream o;o<<std::boolalpha<<"{\"heldout_recovery\":"<<r.recovery<<",\"null_false_discovery\":"<<r.null_false_discovery<<",\"abstention\":"<<r.abstention<<",\"recovery_gate\":"<<r.recovery_gate<<",\"null_gate\":"<<r.null_gate<<",\"mechanisms\":[";for(size_t i=0;i<r.mechanisms.size();++i){auto&m=r.mechanisms[i];if(i)o<<",";o<<"{\"name\":\""<<m.name<<"\",\"correct\":"<<m.correct<<",\"total\":"<<m.total<<",\"abstained\":"<<m.abstained<<"}";}o<<"]}\n";return o.str();}
}
