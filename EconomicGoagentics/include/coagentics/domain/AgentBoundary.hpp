#pragma once
#include "coagentics/domain/AdvancedContracts.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace coagentics::domain {

// Section 3: a domain-neutral boundary between an agent (LLM, reference model,
// optimizer, human proxy, etc.) and the executable world. The agent receives an
// invocation; it never receives a DomainEnvironment pointer and can only return
// a proposed Action.

enum class InvocationStatus {
    Proposed,
    Abstained,
    Rejected,
    Accepted,
    ProviderFailure,
    ParseFailure
};

struct ActionValidationPolicy {
    std::vector<std::string> allowed_action_types;
    std::size_t max_numeric_parameters{32};
    std::size_t max_categorical_parameters{32};
    double max_abs_numeric_parameter{1.0e12};
};

struct ActionValidationReport {
    bool valid{false};
    bool abstained{false};
    std::vector<std::string> errors;
};

struct AgentTurn {
    std::string run_id;
    std::string request_id;
    AgentRef agent;
    WorldRef world;
    std::uint64_t request_seed{};
    std::string observation_fingerprint;
    std::string action_fingerprint;
    Action proposed_action;
    ActionValidationReport validation;
    InvocationStatus status{InvocationStatus::Proposed};
    std::string failure_reason;
};

using ActionProducer = std::function<Action(const AgentInvocation&)>;

class AgentDecisionError : public std::runtime_error {
public:
    AgentDecisionError(InvocationStatus status, std::string reason)
        : std::runtime_error(reason), status_(status) {}
    InvocationStatus status() const noexcept { return status_; }
private:
    InvocationStatus status_;
};

class ReferenceAgentAdapter final : public AgentAdapter {
public:
    ReferenceAgentAdapter(std::string adapter_id, ActionProducer producer);
    Action decide(const AgentInvocation&) override;
    const std::string& adapter_id() const { return adapter_id_; }
private:
    std::string adapter_id_;
    ActionProducer producer_;
};

class ActionBoundary {
public:
    explicit ActionBoundary(ActionValidationPolicy policy = {});

    AgentTurn invoke(const std::string& run_id,
                     const AgentInvocation& invocation,
                     AgentAdapter& adapter) const;

    const ActionValidationPolicy& policy() const { return policy_; }

    static std::string fingerprint_observation(const Observation&);
    static std::string fingerprint_action(const Action&);

private:
    ActionValidationPolicy policy_;
    ActionValidationReport validate(const Action&) const;
};

const char* invocation_status_name(InvocationStatus);

} // namespace coagentics::domain
