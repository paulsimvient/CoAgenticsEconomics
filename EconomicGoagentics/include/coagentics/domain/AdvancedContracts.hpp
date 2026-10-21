#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace coagentics::domain {

// Section 1: domain-neutral contracts. These are intentionally additive to the
// existing economic lab; they do not replace the current Market/Agents APIs yet.

struct WorldRef {
    std::string scenario_id;
    std::string scenario_version;
    std::uint64_t seed{};
    std::uint64_t tick{};
};

struct AgentRef {
    std::string agent_id;
    std::string role;
    std::string provider;
    std::string model;
    std::string model_version;
};

struct InformationEvent {
    std::string event_id;
    std::string entity_id;
    std::uint64_t observed_at{};
    std::string content;
    double reliability{1.0};
    double relevance{1.0};
    std::optional<std::uint64_t> visible_after;
    std::optional<std::uint64_t> expires_at;
    std::optional<std::string> visible_to;
    std::string source_class;
};

struct InformationPolicy {
    bool allow_public_news{true};
    bool allow_price_history{true};
    bool allow_peer_actions{false};
    bool allow_private_state{true};
    std::uint64_t history_window{0};
    std::uint64_t information_delay{0};
};

struct Observation {
    WorldRef world;
    AgentRef agent;
    InformationPolicy policy;
    std::map<std::string, std::string> public_fields;
    std::map<std::string, std::string> private_fields;
    std::vector<InformationEvent> information;
};

struct Action {
    std::string action_id;
    std::string action_type;
    std::map<std::string, double> numeric_parameters;
    std::map<std::string, std::string> categorical_parameters;
};

struct AgentInvocation {
    std::string request_id;
    AgentRef agent;
    Observation observation;
    std::uint64_t request_seed{};
    std::string configuration_fingerprint;
};

struct ActionResult {
    bool accepted{false};
    bool abstained{false};
    std::string status;
    std::string rejection_reason;
    Action action;
};

struct TransitionResult {
    WorldRef next_world;
    std::vector<std::string> execution_ids;
    std::map<std::string, double> outcomes;
};

struct ExperimentSpec {
    std::string experiment_id;
    std::string experiment_version;
    std::string question;
    std::string domain_id;
    std::string design;
    std::vector<std::string> hypotheses;
    std::vector<std::string> metrics;
    std::vector<std::uint64_t> seeds;
    bool preregistered{false};
    bool held_out{false};
};

struct RunSpec {
    std::string run_id;
    std::string experiment_id;
    std::uint64_t seed{};
    std::string condition_id;
    std::string group; // discovery | development | confirmation
};

struct EvidenceRecord {
    std::string run_id;
    std::string observation_fingerprint;
    std::string action_fingerprint;
    std::string outcome_fingerprint;
    std::map<std::string, double> metrics;
    std::string claim_status; // supports | challenges | inconclusive
};

class DomainEnvironment {
public:
    virtual ~DomainEnvironment() = default;
    virtual WorldRef reset(const RunSpec&) = 0;
    virtual Observation observe(const AgentRef&, const InformationPolicy&) const = 0;
    virtual ActionResult validate(const AgentInvocation&, const Action&) const = 0;
    virtual TransitionResult step(const AgentInvocation&, const ActionResult&) = 0;
};

class AgentAdapter {
public:
    virtual ~AgentAdapter() = default;
    virtual Action decide(const AgentInvocation&) = 0;
};

class InformationEnvironment {
public:
    virtual ~InformationEnvironment() = default;
    virtual void publish(InformationEvent) = 0;
    virtual std::vector<InformationEvent> visible_to(const AgentRef&, std::uint64_t tick) const = 0;
};

inline bool valid_information_event(const InformationEvent& e, std::string* why = nullptr) {
    auto fail = [&](const char* reason) {
        if (why) *why = reason;
        return false;
    };
    if (e.event_id.empty()) return fail("missing_event_id");
    if (e.entity_id.empty()) return fail("missing_entity_id");
    if (e.content.empty()) return fail("missing_content");
    if (e.reliability < 0.0 || e.reliability > 1.0) return fail("reliability_out_of_range");
    if (e.relevance < 0.0 || e.relevance > 1.0) return fail("relevance_out_of_range");
    if (e.expires_at && *e.expires_at <= e.observed_at) return fail("invalid_expiration");
    if (e.visible_after && *e.visible_after < e.observed_at) return fail("invalid_visibility_time");
    return true;
}

inline bool valid_run_spec(const RunSpec& r, std::string* why = nullptr) {
    auto fail = [&](const char* reason) {
        if (why) *why = reason;
        return false;
    };
    if (r.run_id.empty()) return fail("missing_run_id");
    if (r.experiment_id.empty()) return fail("missing_experiment_id");
    if (r.condition_id.empty()) return fail("missing_condition_id");
    if (r.group != "discovery" && r.group != "development" && r.group != "confirmation") {
        return fail("invalid_run_group");
    }
    return true;
}

} // namespace coagentics::domain
