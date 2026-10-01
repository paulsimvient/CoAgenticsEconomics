#pragma once
#include "coagentics/analysis/Evidence.hpp"
#include <cstdint>
#include <string>
#include <vector>
namespace coagentics::experiment {
// Domain-independent experiment contract. An adapter supplies paired observations;
// the scientist never sees the mechanism's identity.
struct Probe {std::string id; double news{}; double peer{}; int repetitions{1};};
struct PairedObservation {double control{}; double treatment{}; std::uint64_t seed{};};
class ExperimentalDomain {
public:
 virtual ~ExperimentalDomain()=default;
 virtual PairedObservation run(const Probe&,std::uint64_t seed) const=0;
 virtual std::string name() const=0;
};
struct Candidate {std::string id; double news_gain{}; double peer_gain{}; double adaptation_gain{}; double prior{.25};};
struct ScientistConfig {std::size_t replicates{12}; std::size_t max_experiments{4}; double noise_sd{.2}; double stop_posterior{.95}; double minimum_separation{.3};};
struct ExperimentStep {Probe probe; double observed_delta{}; double standard_error{}; std::vector<double> posterior; std::string decision;};
struct ScientistReport {std::string domain; std::vector<Candidate> candidates; std::vector<ExperimentStep> steps; std::string status; std::string leading_hypothesis; analysis::EvidenceStore evidence;};
ScientistReport run_scientist(const ExperimentalDomain&,const std::vector<Candidate>&,const std::vector<Probe>&,std::uint64_t seed,const ScientistConfig& cfg=ScientistConfig{});
std::string scientist_markdown(const ScientistReport&);
// A market-agent policy probe adapter, used for mechanism-recovery qualification.
class AgentProbeDomain final:public ExperimentalDomain {
public:
 explicit AgentProbeDomain(std::string mechanism):mechanism_(std::move(mechanism)){}
 PairedObservation run(const Probe&,std::uint64_t seed) const override;
 std::string name()const override{return "DV026 market-agent probe";}
private:std::string mechanism_;
};
}
