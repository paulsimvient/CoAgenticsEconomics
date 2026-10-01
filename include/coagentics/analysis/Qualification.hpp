#pragma once
#include "coagentics/analysis/HumanReference.hpp"
#include <string>
#include <vector>
namespace coagentics::analysis {
struct QualificationCheck {std::string id; std::string status; std::string evidence; std::string limitation;};
struct QualificationReport {std::string version; std::vector<QualificationCheck> checks; bool software_green{}; bool darpa_claim_ready{};};
QualificationReport build_v10_qualification_report(double observed_efficiency,const HumanReferenceSet& refs);
std::string qualification_markdown(const QualificationReport& r);
std::string qualification_json(const QualificationReport& r);
}
