#pragma once

#include "coagentics/domain/AgentBoundary.hpp"
#include "coagentics/llm/LlmHarnessV2.hpp"

#include <string>

namespace coagentics::domain {

// Section 8: bridge between the research-grade LLM harness and the
// domain-neutral AgentAdapter boundary. The adapter exposes only the
// canonical observation supplied in AgentInvocation and converts a parsed
// BUY/SELL/HOLD response into a domain Action. Provider failures and parse
// failures become explicit actions/statuses rather than silent substitutions.
class LlmAgentAdapterV2 final : public AgentAdapter {
public:
    LlmAgentAdapterV2(llm::LlmHarness& harness,
                      llm::ModelIdentity model,
                      llm::InferenceConfig config = {});

    Action decide(const AgentInvocation& invocation) override;

    const llm::LlmResponse& last_response() const { return last_response_; }
    const std::string& last_parse_error() const { return last_parse_error_; }

private:
    llm::LlmHarness* harness_;
    llm::ModelIdentity model_;
    llm::InferenceConfig config_;
    llm::LlmResponse last_response_{};
    std::string last_parse_error_;

    static std::string canonical_observation(const Observation& observation);
    static Action parse_action(const std::string& raw, std::string& error);
};

} // namespace coagentics::domain
