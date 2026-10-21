#include "coagentics/domain/LlmAgentAdapterV2.hpp"
#include "coagentics/agents/LlmHarness.hpp"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace coagentics::domain {

LlmAgentAdapterV2::LlmAgentAdapterV2(llm::LlmHarness& harness,
                                     llm::ModelIdentity model,
                                     llm::InferenceConfig config)
    : harness_(&harness), model_(std::move(model)), config_(std::move(config)) {
    if (model_.provider.empty() || model_.model.empty() || model_.version.empty())
        throw std::invalid_argument("LlmAgentAdapterV2 requires complete model identity");
}

std::string LlmAgentAdapterV2::canonical_observation(const Observation& o) {
    std::ostringstream s;
    s << "scenario=" << o.world.scenario_id << "@" << o.world.scenario_version
      << ";seed=" << o.world.seed << ";tick=" << o.world.tick
      << ";agent=" << o.agent.agent_id << ";role=" << o.agent.role;
    for (const auto& [k,v] : o.public_fields) s << ";pub." << k << "=" << v;
    for (const auto& [k,v] : o.private_fields) s << ";priv." << k << "=" << v;
    for (const auto& e : o.information) {
        s << ";info." << e.event_id << "=" << e.content
          << "|reliability=" << std::setprecision(17) << e.reliability
          << "|relevance=" << e.relevance;
    }
    return s.str();
}

Action LlmAgentAdapterV2::parse_action(const std::string& raw, std::string& error) {
    // Reuse the established market action parser so Section 8 does not create
    // a second, subtly different interpretation of BUY/SELL/HOLD.
    auto parsed = agents::parse_market_action(raw);
    if (!parsed.ok || !parsed.action) {
        error = parsed.error.empty() ? "llm_action_parse_failure" : parsed.error;
        return Action{};
    }
    const auto& a = *parsed.action;
    Action out;
    if (a.abstain) {
        out.action_type = "HOLD";
        return out;
    }
    out.action_type = (a.side == market::Side::Buy) ? "BUY" : "SELL";
    out.numeric_parameters["time"] = static_cast<double>(a.time);
    out.numeric_parameters["quantity"] = static_cast<double>(a.quantity);
    out.numeric_parameters["price"] = a.price;
    out.categorical_parameters["asset"] = a.asset;
    return out;
}

Action LlmAgentAdapterV2::decide(const AgentInvocation& invocation) {
    if (!harness_) throw std::runtime_error("llm_harness_missing");
    last_parse_error_.clear();

    llm::LlmRequest request;
    request.model = model_;
    request.config = config_;
    request.canonical_observation = canonical_observation(invocation.observation);
    request.observation_fingerprint = llm::LlmHarness::fingerprint(request.canonical_observation);
    request.run_id = invocation.request_id;

    last_response_ = harness_->invoke(request);
    if (last_response_.status != llm::InvocationStatus::Executed &&
        last_response_.status != llm::InvocationStatus::CacheHit) {
        throw AgentDecisionError(InvocationStatus::ProviderFailure, last_response_.error.empty()
            ? llm::invocation_status_name(last_response_.status)
            : last_response_.error);
    }

    auto action = parse_action(last_response_.raw_text, last_parse_error_);
    if (!last_parse_error_.empty()) throw AgentDecisionError(InvocationStatus::ParseFailure, last_parse_error_);
    return action;
}

} // namespace coagentics::domain
