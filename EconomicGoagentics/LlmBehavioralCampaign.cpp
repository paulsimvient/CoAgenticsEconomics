#include "coagentics/experiment/LlmBehavioralCampaign.hpp"
#include "coagentics/util/Json.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <cstdint>
#include <sstream>
#include <stdexcept>

namespace coagentics::experiment {
namespace {
std::string esc(const std::string& s){ return coagentics::util::json_escape(s); }

std::string fingerprint(const std::string& s){
    std::uint64_t h=1469598103934665603ULL;
    for(unsigned char c: s){ h ^= c; h *= 1099511628211ULL; }
    std::ostringstream o; o << std::hex << h; return o.str();
}

std::string action_string(const std::optional<MarketAction>& a){
    if(!a) return "NONE";
    switch(a->action){ case ActionType::Buy:return "BUY"; case ActionType::Sell:return "SELL"; case ActionType::Hold:return "HOLD"; }
    return "UNKNOWN";
}

std::string validation_string(const ActionValidationResult& v){
    if(v.valid) return "valid";
    std::ostringstream o; o << "invalid";
    if(!v.errors.empty()){ o << ":"; for(std::size_t i=0;i<v.errors.size();++i){ if(i)o<<"|"; o<<v.errors[i]; } }
    return o.str();
}

EvidenceTrace make_evidence(const std::string& probe_id, std::uint64_t seed, const RunResult& c, const RunResult& t){
    EvidenceTrace e;
    e.probe_id=probe_id; e.seed=seed;
    e.control_run_id=c.run_id; e.treatment_run_id=t.run_id;
    e.evidence_id="ev:"+probe_id+":"+std::to_string(seed);
    if(!c.turns.empty()){ const auto& x=c.turns.front(); e.control_observation_fingerprint=fingerprint(x.canonical_request); e.control_action_fingerprint=fingerprint(action_string(x.parsed_action)); e.control_outcome_fingerprint=fingerprint(std::to_string(x.submission.accepted)+":"+std::to_string(c.trades.size())); e.control_canonical_request=x.canonical_request; e.control_raw_response=x.raw_provider_response; e.control_action=action_string(x.parsed_action); e.control_validation=validation_string(x.action_validation); }
    if(!t.turns.empty()){ const auto& x=t.turns.front(); e.treatment_observation_fingerprint=fingerprint(x.canonical_request); e.treatment_action_fingerprint=fingerprint(action_string(x.parsed_action)); e.treatment_outcome_fingerprint=fingerprint(std::to_string(x.submission.accepted)+":"+std::to_string(t.trades.size())); e.treatment_canonical_request=x.canonical_request; e.treatment_raw_response=x.raw_provider_response; e.treatment_action=action_string(x.parsed_action); e.treatment_validation=validation_string(x.action_validation); }
    return e;
}

bool response_is_target(const RunResult& r, AgentRole role){
    if(r.turns.empty() || !r.turns.front().parse.success || !r.turns.front().parsed_action) return false;
    const auto a=*r.turns.front().parsed_action;
    return role==AgentRole::Buyer ? a.action==ActionType::Buy : a.action==ActionType::Sell;
}

ExperimentSpec make_probe_experiment(const BehavioralProbeSpec& p, bool treatment, int rounds){
    ExperimentSpec e;
    e.experiment_id="behavior-probe-"+p.probe_id+(treatment?"-treatment":"-control");
    e.asset="ASSET"; e.fundamental=100; e.starting_cash=10000; e.llm_private_value=120;
    e.seller_cost=80; e.rounds=rounds; e.llm_role=AgentRole::Buyer;
    e.mechanism=market::MechanismKind::ContinuousDoubleAuction;
    e.deterministic_counterparty=true; e.counterparty_limit_price=90; e.counterparty_quantity=1;
    e.information.constraints=ActionConstraints::for_role(AgentRole::Buyer);
    e.information.information_condition=p.probe_id+(treatment?"_treatment":"_control");
    e.information.news={false,"",0.0,0.0};
    e.information.history_visible=true;
    e.information.history={{0,100.0,98.0,102.0}};
    e.information.peer={false,{},std::nullopt};
    if(p.kind==BehavioralProbeKind::Information){
        if(treatment) e.information.news={true,"Value-relevant positive market information",0.95,1.0};
    } else if(p.kind==BehavioralProbeKind::RecentPrice){
        if(treatment) e.information.history={{0,96.0,94.0,98.0},{1,99.0,97.0,101.0},{2,103.0,101.0,105.0}};
        else e.information.history={{0,100.0,98.0,102.0},{1,100.0,98.0,102.0},{2,100.0,98.0,102.0}};
    } else if(p.kind==BehavioralProbeKind::Peer){
        if(treatment) e.information.peer={true,{"PUBLIC-PEER-1"},100.0};
    }
    return e;
}

ProbeRunPair run_pair(const agents::ModelIdentity& model, const BehavioralProbeSpec& probe,
                      std::uint64_t seed, bool live, int rounds){
    auto run_one=[&](bool treatment){
        ExperimentSpec e=make_probe_experiment(probe,treatment,rounds);
        std::shared_ptr<agents::ModelTransport> transport;
        if(live) transport=make_live_openai_compatible_transport(model,default_ollama_transport_config(model.model));
        else {
            const bool buy=treatment; // smoke fixture intentionally responds differently to the treatment.
            const std::string payload=std::string("{\"action\":\"")+(buy?"BUY":"HOLD")+"\",\"asset\":\"ASSET\",\"quantity\":"+(buy?"1":"0")+",\"price\":"+(buy?"101":"null")+",\"time\":0}";
            transport=std::make_shared<RawJsonTransport>(std::vector<std::string>{payload});
        }
        RunSpec rs; rs.seed=seed; rs.model=model; rs.transport=transport; rs.llm_agent_id="LLM-0";
        rs.seller_agent_id="S0"; rs.run_id="behavior:"+probe.probe_id+":"+std::to_string(seed)+(treatment?":t":":c");
        return run_llm_market_experiment(e,rs);
    };
    const auto c=run_one(false); const auto t=run_one(true);
    ProbeRunPair out; out.evidence=make_evidence(probe.probe_id,seed,c,t); out.seed=seed; out.control_response=response_is_target(c,AgentRole::Buyer); out.treatment_response=response_is_target(t,AgentRole::Buyer);
    out.control_valid=!c.turns.empty() && c.turns.front().parse.success && c.turns.front().action_validation.valid;
    out.treatment_valid=!t.turns.empty() && t.turns.front().parse.success && t.turns.front().action_validation.valid;
    out.paired=out.control_valid&&out.treatment_valid;
    out.control_status=out.control_valid?"valid":"invalid"; out.treatment_status=out.treatment_valid?"valid":"invalid";
    return out;
}

analysis::ProbeObservation aggregate_probe(const std::string& id,double expected,const std::vector<ProbeRunPair>& pairs){
    std::size_t n=0,treat=0,control=0,treat_only=0,control_only=0;
    for(const auto&p:pairs) if(p.paired){ ++n; treat+=p.treatment_response; control+=p.control_response; treat_only += p.treatment_response&&!p.control_response; control_only += p.control_response&&!p.treatment_response; }
    if(n==0) throw std::runtime_error("no_valid_matched_pairs_for_probe:"+id);
    return {id,static_cast<double>(treat)/n,static_cast<double>(control)/n,n,n,expected,treat_only,control_only,true};
}
}

const char* behavioral_probe_name(BehavioralProbeKind k){
    switch(k){case BehavioralProbeKind::Information:return "information";case BehavioralProbeKind::RecentPrice:return "recent_price";case BehavioralProbeKind::Peer:return "peer";} return "unknown";
}

LlmBehavioralCampaignReport run_llm_behavioral_campaign(const LlmBehavioralCampaignSpec& spec){
    if(spec.paired_trials_per_probe==0) throw std::invalid_argument("paired_trials_per_probe_required");
    LlmBehavioralCampaignReport report; report.spec=spec; report.preflight=preflight_ollama_live_catalog();
    std::vector<agents::ModelIdentity> models;
    if(spec.use_live_ollama){
        models=report.preflight.available;
        report.executed_live=!models.empty();
    } else {
        const std::size_t n=std::max<std::size_t>(2,spec.minimum_distinct_models);
        for(std::size_t i=0;i<n;++i) models.push_back({"mock","behavior-model-"+std::to_string(i),"v1","mock"});
        report.executed_live=false;
    }
    report.distinct_models_available=models.size();
    report.minimum_model_requirement_met=models.size()>=spec.minimum_distinct_models;
    if(spec.require_minimum_models && !report.minimum_model_requirement_met && !spec.smoke){
        report.campaign_ready=false;
        return report;
    }
    if(spec.smoke && models.size()>2) models.resize(2);
    report.campaign_ready=true;
    for(const auto& model:models){
        LlmModelBehaviorResult mr; mr.model=model;
        std::vector<analysis::ProbeObservation> obs;
        for(const auto& probe:spec.probes){
            std::vector<ProbeRunPair> pairs; pairs.reserve(spec.paired_trials_per_probe);
            for(std::size_t i=0;i<spec.paired_trials_per_probe;++i) pairs.push_back(run_pair(model,probe,spec.base_seed+i, spec.use_live_ollama, spec.rounds));
            mr.valid_pairs += std::count_if(pairs.begin(),pairs.end(),[](const auto&p){return p.paired;});
            mr.invalid_pairs += pairs.size()-std::count_if(pairs.begin(),pairs.end(),[](const auto&p){return p.paired;});
            if(mr.valid_pairs==0) continue;
            for(const auto& pair:pairs) mr.evidence.push_back(pair.evidence);
            obs.push_back(aggregate_probe(probe.probe_id,probe.expected_delta,pairs));
        }
        if(!obs.empty()) mr.profile=analysis::build_llm_behavioral_profile(model.provider,model.model,model.version,obs,spec.minimum_effect,spec.alpha);
        report.models.push_back(std::move(mr));
    }
    std::filesystem::create_directories(spec.results_dir);
    std::ofstream(spec.results_dir+"/campaign.json")<<llm_behavioral_campaign_json(report);
    std::ofstream(spec.results_dir+"/campaign.md")<<llm_behavioral_campaign_markdown(report);
    return report;
}

std::string llm_behavioral_campaign_json(const LlmBehavioralCampaignReport&r){
    std::ostringstream o; o<<std::boolalpha<<"{\"campaign_id\":\""<<esc(r.spec.campaign_id)<<"\",\"use_live_ollama\":"<<(r.spec.use_live_ollama?"true":"false")<<",\"executed_live\":"<<(r.executed_live?"true":"false")<<",\"distinct_models_available\":"<<r.distinct_models_available<<",\"minimum_model_requirement_met\":"<<(r.minimum_model_requirement_met?"true":"false")<<",\"campaign_ready\":"<<(r.campaign_ready?"true":"false")<<",\"models\":[";
    for(std::size_t i=0;i<r.models.size();++i){if(i)o<<",";const auto&m=r.models[i];o<<"{\"provider\":\""<<esc(m.model.provider)<<"\",\"model\":\""<<esc(m.model.model)<<"\",\"version\":\""<<esc(m.model.version)<<"\",\"valid_pairs\":"<<m.valid_pairs<<",\"invalid_pairs\":"<<m.invalid_pairs<<",\"evidence_count\":"<<m.evidence.size()<<",\"evidence\":["; for(std::size_t j=0;j<m.evidence.size();++j){ if(j)o<<","; const auto&e=m.evidence[j]; o<<"{\"evidence_id\":\""<<esc(e.evidence_id)<<"\",\"probe_id\":\""<<esc(e.probe_id)<<"\",\"seed\":"<<e.seed<<",\"control_run_id\":\""<<esc(e.control_run_id)<<"\",\"treatment_run_id\":\""<<esc(e.treatment_run_id)<<"\",\"control_observation_fingerprint\":\""<<e.control_observation_fingerprint<<"\",\"treatment_observation_fingerprint\":\""<<e.treatment_observation_fingerprint<<"\",\"control_action_fingerprint\":\""<<e.control_action_fingerprint<<"\",\"treatment_action_fingerprint\":\""<<e.treatment_action_fingerprint<<"\",\"control_outcome_fingerprint\":\""<<e.control_outcome_fingerprint<<"\",\"treatment_outcome_fingerprint\":\""<<e.treatment_outcome_fingerprint<<"\",\"control_action\":\""<<esc(e.control_action)<<"\",\"treatment_action\":\""<<esc(e.treatment_action)<<"\",\"control_validation\":\""<<esc(e.control_validation)<<"\",\"treatment_validation\":\""<<esc(e.treatment_validation)<<"\"}"; } o<<"],\"profile\":\""<<esc(analysis::llm_behavioral_profile_markdown(m.profile))<<"\"}";}o<<"],\"claim_boundary\":\""<<esc(r.claim_boundary)<<"\",\"limitation\":\""<<esc(r.limitation)<<"\"}";return o.str();
}

std::string llm_behavioral_campaign_markdown(const LlmBehavioralCampaignReport&r){
    std::ostringstream o; o<<"# CoAgentics live LLM behavioral campaign\n\n"<<"Campaign: **"<<r.spec.campaign_id<<"**\n\n"<<"Models available: **"<<r.distinct_models_available<<"**\n\n"<<"Minimum-model gate: **"<<(r.minimum_model_requirement_met?"PASS":"FAIL")<<"**\n\n"<<"Live execution: **"<<(r.executed_live?"YES":"NO")<<"**\n\n";
    if(!r.campaign_ready)o<<"## Campaign not executed\n\nThe required model catalog was not available. No substitutions were made.\n\n";
    for(const auto&m:r.models){o<<"## "<<m.model.provider<<" / "<<m.model.model<<" / "<<m.model.version<<"\n\n"<<analysis::llm_behavioral_profile_markdown(m.profile)<<"\n"; o<<"### Evidence index\n\n| Evidence | Probe | Seed | Control run | Treatment run | Control action | Treatment action |\n|---|---|---:|---|---|---|---|\n"; for(const auto&e:m.evidence){o<<"| "<<e.evidence_id<<" | "<<e.probe_id<<" | "<<e.seed<<" | "<<e.control_run_id<<" | "<<e.treatment_run_id<<" | "<<e.control_action<<" | "<<e.treatment_action<<" |\n";} o<<"\n"; }
    o<<"## Claim boundary\n\n"<<r.claim_boundary<<"\n\n## Limitation\n\n"<<r.limitation<<"\n";return o.str();
}
}
