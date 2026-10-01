#pragma once
#include <map>
#include <string>
#include <vector>
namespace coagentics::analysis {
struct BehavioralFeatureVector { double information_sensitivity{}; double peer_sensitivity{}; double persistence{}; double efficiency_delta{}; };
struct Classification { std::string label; double score{}; std::map<std::string,double> evidence; };
class BehavioralClassifier { public: virtual ~BehavioralClassifier()=default; virtual Classification classify(const BehavioralFeatureVector&) const=0; };
class MechanismClassifier final : public BehavioralClassifier { public: Classification classify(const BehavioralFeatureVector&) const override; };
}
