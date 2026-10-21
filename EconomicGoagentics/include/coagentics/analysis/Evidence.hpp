#pragma once
#include "coagentics/analysis/Hypothesis.hpp"
#include <string>
#include <vector>
namespace coagentics::analysis {
struct EvidenceEnvelope {
 std::string experiment_id;
 std::string control_run_id;
 std::string treatment_run_id;
 std::uint64_t seed{};
 EvidenceRecord evidence;
};
class EvidenceStore {
public:
 void append(EvidenceEnvelope e){records_.push_back(std::move(e));}
 const std::vector<EvidenceEnvelope>& records() const{return records_;}
 std::string jsonl() const;
private: std::vector<EvidenceEnvelope> records_;
};
}
