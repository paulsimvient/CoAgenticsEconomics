#include "coagentics/analysis/HumanComparison.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <numeric>
#include <sstream>
namespace coagentics::analysis {
namespace {
std::string escaped(const std::string& s){std::string r;for(char c:s){if(c=='"'||c=='\\')r+='\\';if(c=='\n')r+="\\n";else r+=c;}return r;}
}
HumanComparisonReport compare_human_behavior(const HumanReferenceSet& refs,const std::vector<ModelObservation>& observations){
 HumanComparisonReport report;
 report.live_model_evidence=std::any_of(observations.begin(),observations.end(),[](const auto& o){return o.origin==EvidenceOrigin::LiveProvider;});
 // Explicitly report missing human baselines, rather than substitute qualification fixtures.
 const std::vector<std::string> required={"allocative_efficiency","bid_shading","information_response","adaptation","social_influence"};
 for(const auto& metric:required){
  HumanComparison c; c.metric=metric;
  if(!refs.has(metric)||!refs.get(metric).empirical){c.status="NO_EMPIRICAL_HUMAN_REFERENCE";c.explanation="No documented human distribution for this metric; no comparison performed.";report.comparisons.push_back(c);continue;}
  const auto& ref=refs.get(metric);c.source_id=ref.source_id;c.human_units=ref.samples.empty()?ref.n:ref.samples.size();c.human_mean=ref.mean;
  std::vector<double> values;
  bool has_live=false;
  for(const auto& o:observations) if(o.metric==metric && o.condition==ref.condition && std::isfinite(o.value)){
    values.push_back(o.value);has_live|=o.origin==EvidenceOrigin::LiveProvider;
  }
  c.model_runs=values.size();
  if(values.empty()){c.status="NO_CONDITION_MATCHED_OBSERVATIONS";c.explanation="No model/control observations match the source's experimental condition.";}
  else if(!has_live){c.status="SCRIPTED_CONTROL_ONLY";c.explanation="Condition-matched scripted observations; not evidence about live LLM behavior.";}
  else if(std::any_of(observations.begin(),observations.end(),[&](const auto& o){return o.metric==metric && o.condition==ref.condition && o.origin!=EvidenceOrigin::LiveProvider;})){
    c.status="MIXED_ORIGIN_REJECTED";c.explanation="Do not pool live model runs with scripted controls.";
  }else{c.status="DESCRIPTIVE_COMPARISON";c.explanation="Descriptive comparison only; source units may be market means, not independent human participants.";}
  if(!values.empty()){
    c.model_mean=std::accumulate(values.begin(),values.end(),0.0)/values.size();c.difference=c.model_mean-c.human_mean;
    if(!ref.samples.empty()) c.empirical_percentile=100.0*std::count_if(ref.samples.begin(),ref.samples.end(),[&](double v){return v<=c.model_mean;})/ref.samples.size();
  }
  report.comparisons.push_back(c);
 }
 report.human_behavioral_coverage=std::all_of(report.comparisons.begin(),report.comparisons.end(),[](const auto& c){return c.status=="DESCRIPTIVE_COMPARISON";});
 report.darpa_claim_ready=report.live_model_evidence&&report.human_behavioral_coverage;
 return report;
}
std::string human_comparison_markdown(const HumanComparisonReport& r){
 std::ostringstream o;o<<"# v23 — Human behavioral comparison\n\n";
 o<<"This is a provenance- and condition-gated comparison, not a claim of human equivalence. Published market means are not independent participant observations.\n\n";
 o<<"| Metric | Status | Human units | Matched runs | Human mean | Observed mean | Difference | Source |\n|---|---|---:|---:|---:|---:|---:|---|\n";
 o<<std::fixed<<std::setprecision(3);
 for(const auto& c:r.comparisons)o<<"| "<<c.metric<<" | "<<c.status<<" | "<<c.human_units<<" | "<<c.model_runs<<" | "<<c.human_mean<<" | "<<c.model_mean<<" | "<<c.difference<<" | "<<c.source_id<<" |\n";
 o<<"\nLive model evidence: "<<(r.live_model_evidence?"YES":"NO")<<"  \nAll behavioral human references available and matched: "<<(r.human_behavioral_coverage?"YES":"NO")<<"  \nClaim gate: "<<(r.darpa_claim_ready?"READY FOR FURTHER REVIEW":"NOT READY")<<"\n\n";
 for(const auto& c:r.comparisons)o<<"- "<<c.metric<<": "<<c.explanation<<"\n";
 return o.str();
}
std::string human_comparison_json(const HumanComparisonReport& r){
 std::ostringstream o;o<<std::fixed<<std::setprecision(6)<<"{\"live_model_evidence\":"<<(r.live_model_evidence?"true":"false")<<",\"human_behavioral_coverage\":"<<(r.human_behavioral_coverage?"true":"false")<<",\"darpa_claim_ready\":"<<(r.darpa_claim_ready?"true":"false")<<",\"comparisons\":[";
 for(std::size_t i=0;i<r.comparisons.size();++i){const auto& c=r.comparisons[i];if(i)o<<",";o<<"{\"metric\":\""<<escaped(c.metric)<<"\",\"status\":\""<<escaped(c.status)<<"\",\"human_units\":"<<c.human_units<<",\"model_runs\":"<<c.model_runs<<",\"human_mean\":"<<c.human_mean<<",\"model_mean\":"<<c.model_mean<<",\"difference\":"<<c.difference<<",\"source_id\":\""<<escaped(c.source_id)<<"\"}";}
 o<<"]}\n";return o.str();
}
}
