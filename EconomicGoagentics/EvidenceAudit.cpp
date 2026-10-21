#include "coagentics/analysis/EvidenceAudit.hpp"
#include "coagentics/util/Json.hpp"
#include <sstream>
#include <unordered_set>

namespace coagentics::analysis {
namespace {
std::string esc(const std::string& s){ return coagentics::util::json_escape(s); }
bool nonempty(const std::string& s){ return !s.empty(); }
}

EvidenceAuditResult audit_llm_behavioral_campaign(const experiment::LlmBehavioralCampaignReport& report){
    EvidenceAuditResult out;
    out.models_checked=report.models.size();
    std::unordered_set<std::string> ids;
    for(const auto& m: report.models){
        out.evidence_records += m.evidence.size();
        for(const auto& e: m.evidence){
            bool ok=true;
            if(e.evidence_id.empty()) { out.errors.push_back("missing_evidence_id"); ok=false; }
            if(!ids.insert(e.evidence_id).second) { out.errors.push_back("duplicate_evidence_id:"+e.evidence_id); ok=false; }
            const std::string fields[] = {
                e.probe_id,e.control_run_id,e.treatment_run_id,
                e.control_observation_fingerprint,e.treatment_observation_fingerprint,
                e.control_action_fingerprint,e.treatment_action_fingerprint,
                e.control_outcome_fingerprint,e.treatment_outcome_fingerprint,
                e.control_canonical_request,e.treatment_canonical_request,
                e.control_action,e.treatment_action,
                e.control_validation,e.treatment_validation};
            for(const auto& f:fields) if(!nonempty(f)){ out.errors.push_back("incomplete_evidence:"+e.evidence_id); ok=false; break; }
            if(ok) ++out.complete_records;
        }
    }
    out.valid = out.errors.empty() && out.evidence_records==out.complete_records;
    return out;
}

std::string evidence_jsonl(const experiment::LlmBehavioralCampaignReport& report){
    std::ostringstream o;
    for(const auto& m:report.models){
        for(const auto& e:m.evidence){
            o << "{\"evidence_id\":\""<<esc(e.evidence_id)
              <<"\",\"campaign_id\":\""<<esc(report.spec.campaign_id)
              <<"\",\"provider\":\""<<esc(m.model.provider)
              <<"\",\"model\":\""<<esc(m.model.model)
              <<"\",\"version\":\""<<esc(m.model.version)
              <<"\",\"probe_id\":\""<<esc(e.probe_id)
              <<"\",\"seed\":"<<e.seed
              <<",\"control_run_id\":\""<<esc(e.control_run_id)
              <<"\",\"treatment_run_id\":\""<<esc(e.treatment_run_id)
              <<"\",\"control_observation_fingerprint\":\""<<esc(e.control_observation_fingerprint)
              <<"\",\"treatment_observation_fingerprint\":\""<<esc(e.treatment_observation_fingerprint)
              <<"\",\"control_action_fingerprint\":\""<<esc(e.control_action_fingerprint)
              <<"\",\"treatment_action_fingerprint\":\""<<esc(e.treatment_action_fingerprint)
              <<"\",\"control_outcome_fingerprint\":\""<<esc(e.control_outcome_fingerprint)
              <<"\",\"treatment_outcome_fingerprint\":\""<<esc(e.treatment_outcome_fingerprint)
              <<"\",\"control_canonical_request\":\""<<esc(e.control_canonical_request)
              <<"\",\"treatment_canonical_request\":\""<<esc(e.treatment_canonical_request)
              <<"\",\"control_raw_response\":\""<<esc(e.control_raw_response)
              <<"\",\"treatment_raw_response\":\""<<esc(e.treatment_raw_response)
              <<"\",\"control_action\":\""<<esc(e.control_action)
              <<"\",\"treatment_action\":\""<<esc(e.treatment_action)
              <<"\",\"control_validation\":\""<<esc(e.control_validation)
              <<"\",\"treatment_validation\":\""<<esc(e.treatment_validation)
              <<"\"}\n";
        }
    }
    return o.str();
}

std::string evidence_index_markdown(const experiment::LlmBehavioralCampaignReport& report){
    std::ostringstream o;
    o << "# CoAgentics Evidence Index\n\n";
    o << "Campaign: **"<<report.spec.campaign_id<<"**\n\n";
    o << "Every behavioral result must be traceable to a matched control/treatment pair. This index is a navigation aid; it does not itself establish a causal claim.\n\n";
    for(const auto& m:report.models){
        o << "## "<<m.model.provider<<" / "<<m.model.model<<" / "<<m.model.version<<"\n\n";
        o << "| Evidence | Probe | Seed | Control run | Treatment run | Control action | Treatment action |\n|---|---|---:|---|---|---|---|\n";
        for(const auto& e:m.evidence){
            o << "| "<<e.evidence_id<<" | "<<e.probe_id<<" | "<<e.seed<<" | "<<e.control_run_id<<" | "<<e.treatment_run_id<<" | "<<e.control_action<<" | "<<e.treatment_action<<" |\n";
        }
        o << "\n";
    }
    return o.str();
}
} // namespace coagentics::analysis
