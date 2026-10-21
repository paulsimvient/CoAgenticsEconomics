#include "coagentics/experiment/Experiment.hpp"
#include <cmath>
namespace coagentics::experiment {
Summary summarize(const std::vector<TrialResult>& xs){
 Summary s; s.n=xs.size(); if(xs.empty()) return s;
 for (auto &x : xs) s.mean_delta += x.delta;
 s.mean_delta /= xs.size();
 if(xs.size()>1){ double ss=0; for(auto&x:xs){double d=x.delta-s.mean_delta;ss+=d*d;} double sd=std::sqrt(ss/(xs.size()-1)); s.standard_error=sd/std::sqrt((double)xs.size()); }
 s.ci95_low=s.mean_delta-1.96*s.standard_error; s.ci95_high=s.mean_delta+1.96*s.standard_error; return s;
}
}
