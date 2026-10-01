#include "coagentics/analysis/DecisionTraceValidation.hpp"
#include "coagentics/experiment/MarketScientist.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){using namespace coagentics;
 experiment::CdaMarketDomain market("signal-responsive",500);
 experiment::Probe p{"pulse",8,0,1};auto a=market.run_traced(p,1234),b=market.run_traced(p,1234);
 assert(a.control.size()==a.treatment.size());assert(a.control.size()==b.control.size());
 for(size_t i=0;i<a.control.size();++i){assert(a.control[i].event==b.control[i].event);assert(a.control[i].actor>=0&&a.control[i].actor<12);}
 auto x=analysis::decision_trace_trajectory("information-blind",4444,500);for(double v:x.response)assert(std::abs(v)<1e-9);
 auto y=analysis::decision_trace_trajectory("signal-responsive",4444,500);assert(y.response[0]>.5);assert(y.response[3]<-.5);
 analysis::TraceConfig cfg;cfg.training_trials=4;cfg.heldout_trials=6;cfg.quote_events=500;
 auto r=analysis::validate_decision_traces(cfg);assert(r.rows.size()==5);assert(r.recovery>=0&&r.recovery<=1);assert(r.null_false_discovery>=0&&r.null_false_discovery<=1);
 std::cout<<"v20 trace PASS recovery="<<r.recovery<<" null="<<r.null_false_discovery<<" coverage="<<r.mean_matched_coverage<<"\n";
}
