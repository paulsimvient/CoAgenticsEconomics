#include "coagentics/analysis/Phenotype.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace coagentics::analysis;
int main(){auto r=run_behavioral_phenotyping(26026);assert(r.agents.size()==5);assert(!r.live_llms_used);auto get=[&](const char*m)->const BehavioralFingerprint&{for(auto&x:r.agents)if(x.fingerprint.mechanism==m)return x.fingerprint;assert(false);return r.agents[0].fingerprint;};auto&blind=get("information-blind");auto&sig=get("signal-responsive");auto&peer=get("peer-responsive");auto&adapt=get("adaptive");assert(std::abs(blind.news_sensitivity)<1e-9);assert(std::abs(blind.peer_susceptibility)<1e-9);assert(sig.news_sensitivity>.9);assert(std::abs(sig.peer_susceptibility)<1e-9);assert(peer.peer_susceptibility>.4);assert(std::abs(peer.news_sensitivity)<1e-9);assert(adapt.adaptation>0);for(auto&x:r.agents){assert(x.explanations.size()==4);assert(!x.next_experiment.empty());}std::cout<<"phenotypes="<<r.agents.size()<<" signal="<<sig.news_sensitivity<<" peer="<<peer.peer_susceptibility<<" adaptive="<<adapt.adaptation<<"\n";}
