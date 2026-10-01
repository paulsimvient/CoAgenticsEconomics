#include "coagentics/analysis/Discrimination.hpp"
namespace coagentics::analysis {
std::vector<DiscriminationCell> run_discrimination_matrix(std::uint64_t seed){std::vector<DiscriminationCell> out;MechanismClassifier cl;for(bool info:{false,true})for(bool peer:{false,true})for(bool het:{false,true})for(bool incentive:{false,true}){auto m=probe_mechanism(het?Mechanism::Adaptive:(peer?Mechanism::PeerResponsive:Mechanism::SignalResponsive),seed);BehavioralFeatureVector f;f.information_sensitivity=info?m.information_sensitivity:0;f.peer_sensitivity=peer?m.peer_sensitivity:0;if(incentive)f.information_sensitivity*=.75;f.persistence=het?.25:0;out.push_back({info,peer,het,incentive,f,cl.classify(f)});}return out;}
}
