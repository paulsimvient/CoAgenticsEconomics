#include "coagentics/analysis/HumanReference.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
namespace coagentics::analysis {
ReferenceComparison HumanReferenceSet::compare(const std::string& metric,double observed)const{
 auto it=refs_.find(metric); if(it==refs_.end()) throw std::runtime_error("missing reference metric: "+metric);
 const auto& r=it->second; double z=r.sd>0?(observed-r.mean)/r.sd:0.0;
 double pct=-1.0;
 if(!r.samples.empty()){
   auto le=std::count_if(r.samples.begin(),r.samples.end(),[&](double x){return x<=observed;});
   pct=100.0*static_cast<double>(le)/static_cast<double>(r.samples.size());
 }
 std::string interp=std::abs(z)>1.96?"outside reference 95% normal-approximation band":"inside reference 95% normal-approximation band";
 if(r.empirical) interp += "; empirical literature reference (condition-specific, not a universal human norm)";
 return {metric,observed,z,std::abs(z)>1.96,interp,pct,r.empirical};
}
std::vector<std::string> HumanReferenceSet::metrics()const{std::vector<std::string> out; for(const auto& [k,v]:refs_)out.push_back(k); return out;}
HumanReferenceSet qualification_reference_fixture(){
 HumanReferenceSet s;
 s.add({"allocative_efficiency","synthetic qualification fixture; replace with cited human experimental data",100,90.0,5.0,{},"synthetic","fixture","fixture",false});
 s.add({"bid_shading","synthetic qualification fixture; replace with cited human experimental data",100,0.10,0.08,{},"synthetic","fixture","fixture",false});
 s.add({"reaction_latency","synthetic qualification fixture; replace with cited human experimental data",100,2.0,1.0,{},"synthetic","fixture","fixture",false});
 return s;
}
HumanReferenceSet empirical_market_reference_catalog(){
 HumanReferenceSet s;
 // Gode & Sunder (1993), JPE 101(1), Table 2. These are the five reported HUMAN
 // mean-efficiency values for markets 1..5: 99.7, 99.1, 100.0, 99.1, 90.2.
 // We compute the mean and sample SD across those five market-level means; this is a
 // condition-specific literature benchmark, not an estimate of all human traders.
 std::vector<double> h={99.7,99.1,100.0,99.1,90.2};
 const double mean=97.62; const double sd=4.1661733041245395;
 s.add({"allocative_efficiency",
   "Gode & Sunder (1993), Journal of Political Economy 101(1):119-137, Table 2; human double-auction market mean efficiencies",
   h.size(),mean,sd,h,
   "human laboratory traders","five double-auction markets reported in Table 2","doi:10.1086/261868",true});
 // Second condition-tagged metric from the same Table 2: markets 1–4 (exclude market 5 = 90.2),
 // the high-efficiency laboratory DA subset. Mean/SD recomputed from the four reported values only.
 std::vector<double> h_hi={99.7,99.1,100.0,99.1};
 const double mean_hi=(99.7+99.1+100.0+99.1)/4.0;
 double var_hi=0;
 for(double x:h_hi){ double d=x-mean_hi; var_hi+=d*d; }
 const double sd_hi=std::sqrt(var_hi/static_cast<double>(h_hi.size()-1));
 s.add({"allocative_efficiency_high_condition",
   "Gode & Sunder (1993), JPE 101(1) Table 2 markets 1–4 only (exclude market 5=90.2); high-efficiency human DA subset",
   h_hi.size(),mean_hi,sd_hi,h_hi,
   "human laboratory traders","Table 2 markets 1-4 high-efficiency subset","doi:10.1086/261868",true});
 // Single reported human market mean from Table 2 market 5 (lowest). n=1 ⇒ sd=0 by convention.
 s.add({"allocative_efficiency_market5_low",
   "Gode & Sunder (1993), JPE 101(1) Table 2 market 5 only (90.2%); lowest reported human DA market mean",
   1,90.2,0.0,{90.2},
   "human laboratory traders","Table 2 market 5","doi:10.1086/261868",true});
 // Institutional ZI-C floor from the same Table 2 (not human traders). Kept for attribution baselines.
 std::vector<double> zi={99.9,99.2,99.0,98.2,97.1};
 const double zi_mean=98.68;
 double zi_var=0; for(double x:zi){ double d=x-zi_mean; zi_var+=d*d; }
 const double zi_sd=std::sqrt(zi_var/static_cast<double>(zi.size()-1));
 s.add({"zi_c_allocative_efficiency",
   "Gode & Sunder (1993), JPE 101(1) Table 2; ZI-C (zero-intelligence constrained) market mean efficiencies",
   zi.size(),zi_mean,zi_sd,zi,
   "ZI-C robots","five double-auction markets Table 2 ZI-C column","doi:10.1086/261868",true});
 // bid_shading / information_response / adaptation / social_influence: intentionally absent —
 // no source-pinnable human distributions in-repo; HumanComparison reports NO_EMPIRICAL_HUMAN_REFERENCE.
 return s;
}
}
