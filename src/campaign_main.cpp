#include "coagentics/experiment/Campaign.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
int main(int argc,char**argv){coagentics::experiment::CampaignConfig c;c.id="dv026-private-news";if(argc>1)c.trials=std::stoull(argv[1]);auto s=coagentics::experiment::run_behavior_campaign(c);std::filesystem::create_directories("results");std::ofstream("results/campaign-summary.json")<<coagentics::experiment::campaign_json(s);std::ofstream("results/evidence.jsonl")<<s.evidence.jsonl();std::ofstream("results/science.html")<<coagentics::experiment::campaign_html(s);std::cout<<coagentics::experiment::campaign_json(s);return 0;}
