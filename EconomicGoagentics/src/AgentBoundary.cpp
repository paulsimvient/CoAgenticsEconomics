#include "coagentics/domain/AgentBoundary.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace coagentics::domain {
namespace {

std::uint64_t fnv1a(const std::string& s) {
    std::uint64_t h = 1469598103934665603ULL;
    for (unsigned char c : s) { h ^= c; h *= 1099511628211ULL; }
    return h;
}

std::string hex64(std::uint64_t x) {
    std::ostringstream out;
    out << std::hex << std::setw(16) << std::setfill('0') << x;
    return out.str();
}

std::string canonical(const Observation& o) {
    std::ostringstream s;
    s << "world=" << o.world.scenario_id << "@" << o.world.scenario_version
      << ":" << o.world.seed << ":" << o.world.tick
      << "|agent=" << o.agent.agent_id << ":" << o.agent.role
      << ":" << o.agent.provider << ":" << o.agent.model << ":" << o.agent.model_version
      << "|policy=" << o.policy.allow_public_news << "," << o.policy.allow_price_history
      << "," << o.policy.allow_peer_actions << "," << o.policy.allow_private_state
      << "," << o.policy.history_window << "," << o.policy.information_delay;
    for (const auto& [k,v] : o.public_fields) s << "|pub:" << k << "=" << v;
    for (const auto& [k,v] : o.private_fields) s << "|priv:" << k << "=" << v;
    for (const auto& e : o.information) {
        s << "|info:" << e.event_id << ":" << e.entity_id << ":" << e.observed_at
          << ":" << e.content << ":" << std::setprecision(17) << e.reliability
          << ":" << e.relevance;
    }
    return s.str();
}

std::string canonical(const Action& a) {
    std::ostringstream s;
    s << "type=" << a.action_type;
    for (const auto& [k,v] : a.numeric_parameters) s << "|n:" << k << "=" << std::setprecision(17) << v;
    for (const auto& [k,v] : a.categorical_parameters) s << "|c:" << k << "=" << v;
    return s.str();
}

} // namespace

ReferenceAgentAdapter::ReferenceAgentAdapter(std::string adapter_id, ActionProducer producer)
    : adapter_id_(std::move(adapter_id)), producer_(std::move(producer)) {
    if (adapter_id_.empty()) throw std::invalid_argument("ReferenceAgentAdapter requires adapter_id");
    if (!producer_) throw std::invalid_argument("ReferenceAgentAdapter requires producer");
}

Action ReferenceAgentAdapter::decide(const AgentInvocation& invocation) {
    return producer_(invocation);
}

ActionBoundary::ActionBoundary(ActionValidationPolicy policy) : policy_(std::move(policy)) {}

ActionValidationReport ActionBoundary::validate(const Action& a) const {
    ActionValidationReport r;
    if (a.action_type.empty()) { r.errors.push_back("missing_action_type"); return r; }
    if (!policy_.allowed_action_types.empty() &&
        std::find(policy_.allowed_action_types.begin(), policy_.allowed_action_types.end(), a.action_type)
            == policy_.allowed_action_types.end()) {
        r.errors.push_back("action_type_not_allowed");
    }
    if (a.numeric_parameters.size() > policy_.max_numeric_parameters)
        r.errors.push_back("too_many_numeric_parameters");
    if (a.categorical_parameters.size() > policy_.max_categorical_parameters)
        r.errors.push_back("too_many_categorical_parameters");
    for (const auto& [name, value] : a.numeric_parameters) {
        if (name.empty()) r.errors.push_back("empty_numeric_parameter_name");
        if (!std::isfinite(value)) r.errors.push_back("nonfinite_numeric_parameter:" + name);
        else if (std::abs(value) > policy_.max_abs_numeric_parameter)
            r.errors.push_back("numeric_parameter_out_of_bounds:" + name);
    }
    for (const auto& [name, value] : a.categorical_parameters) {
        if (name.empty()) r.errors.push_back("empty_categorical_parameter_name");
        (void)value;
    }
    r.abstained = (a.action_type == "HOLD" || a.action_type == "ABSTAIN");
    r.valid = r.errors.empty();
    return r;
}

AgentTurn ActionBoundary::invoke(const std::string& run_id,
                                 const AgentInvocation& invocation,
                                 AgentAdapter& adapter) const {
    AgentTurn turn;
    turn.run_id = run_id;
    turn.request_id = invocation.request_id;
    turn.agent = invocation.agent;
    turn.world = invocation.observation.world;
    turn.request_seed = invocation.request_seed;
    turn.observation_fingerprint = fingerprint_observation(invocation.observation);

    try {
        turn.proposed_action = adapter.decide(invocation);
    } catch (const AgentDecisionError& ex) {
        turn.status = ex.status();
        turn.failure_reason = ex.what();
        return turn;
    } catch (const std::exception& ex) {
        turn.status = InvocationStatus::ProviderFailure;
        turn.failure_reason = ex.what();
        return turn;
    } catch (...) {
        turn.status = InvocationStatus::ProviderFailure;
        turn.failure_reason = "unknown_agent_failure";
        return turn;
    }

    turn.action_fingerprint = fingerprint_action(turn.proposed_action);
    turn.validation = validate(turn.proposed_action);
    if (!turn.validation.valid) {
        turn.status = InvocationStatus::Rejected;
        turn.failure_reason = turn.validation.errors.front();
    } else if (turn.validation.abstained) {
        turn.status = InvocationStatus::Abstained;
    } else {
        turn.status = InvocationStatus::Accepted;
    }
    return turn;
}

std::string ActionBoundary::fingerprint_observation(const Observation& o) {
    return hex64(fnv1a(canonical(o)));
}

std::string ActionBoundary::fingerprint_action(const Action& a) {
    return hex64(fnv1a(canonical(a)));
}

const char* invocation_status_name(InvocationStatus s) {
    switch (s) {
        case InvocationStatus::Proposed: return "proposed";
        case InvocationStatus::Abstained: return "abstained";
        case InvocationStatus::Rejected: return "rejected";
        case InvocationStatus::Accepted: return "accepted";
        case InvocationStatus::ProviderFailure: return "provider_failure";
        case InvocationStatus::ParseFailure: return "parse_failure";
    }
    return "unknown";
}

} // namespace coagentics::domain
