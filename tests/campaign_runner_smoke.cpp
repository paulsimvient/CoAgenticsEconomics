#include "coagentics/experiment/Campaign.hpp"
#include <cassert>
#include <iostream>
int main(){coagentics::experiment::CampaignConfig c;c.trials=12;c.first_seed=42;auto s=coagentics::experiment::run_behavior_campaign(c);assert(s.evidence.records().size()==12);assert(s.control_mean_efficiency>=0&&s.control_mean_efficiency<=100);assert(s.treatment_mean_efficiency>=0&&s.treatment_mean_efficiency<=100);assert(s.supports+s.challenges+s.inconclusive==12);auto j=coagentics::experiment::campaign_json(s);assert(j.find("targeted_mean_abs_shift")!=std::string::npos);std::cout<<j;}
