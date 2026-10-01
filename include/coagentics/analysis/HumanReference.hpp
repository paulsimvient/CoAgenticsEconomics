#pragma once
#include <cmath>
#include <map>
#include <string>
#include <vector>
namespace coagentics::analysis {
struct ReferenceDistribution {
 std::string metric; std::string provenance; std::size_t n{}; double mean{}; double sd{};
 std::vector<double> samples; std::string population; std::string condition; std::string source_id;
 bool empirical{false};
};
struct ReferenceComparison {
 std::string metric; double observed{}; double z_score{}; bool outside_95{}; std::string interpretation;
 double empirical_percentile{-1.0}; bool empirical_reference{false};
};
class HumanReferenceSet {
public:
 void add(ReferenceDistribution d){ refs_[d.metric]=std::move(d); }
 bool has(const std::string& metric)const{return refs_.count(metric)>0;}
 const ReferenceDistribution& get(const std::string& metric)const{return refs_.at(metric);}
 ReferenceComparison compare(const std::string& metric,double observed)const;
 std::vector<std::string> metrics()const;
private: std::map<std::string,ReferenceDistribution> refs_;
};
HumanReferenceSet qualification_reference_fixture();
// Literature-grounded catalog. Values are entered only when directly reported by the cited source.
HumanReferenceSet empirical_market_reference_catalog();
}
