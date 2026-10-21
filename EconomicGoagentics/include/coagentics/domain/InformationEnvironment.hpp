#pragma once

#include "coagentics/domain/AdvancedContracts.hpp"

#include <map>
#include <mutex>
#include <vector>

namespace coagentics::domain {

// Section 2: explicit information environment.
//
// The environment owns the information ledger and is the only component that
// decides which information is observable to an agent at a given tick.
// Policies are applied at observation time; the caller never receives the
// complete ledger and then filters it itself.
class ExplicitInformationEnvironment final : public InformationEnvironment {
public:
    void publish(InformationEvent event) override;

    std::vector<InformationEvent> visible_to(
        const AgentRef& agent,
        std::uint64_t tick) const override;

    std::size_t size() const;

    std::vector<InformationEvent> visible_to(
        const AgentRef& agent,
        std::uint64_t tick,
        const InformationPolicy& policy) const;

private:
    mutable std::mutex mutex_;
    std::vector<InformationEvent> events_;
};

// Build an observation from an existing world reference and agent policy.
// World/private/public fields remain caller-owned; information is supplied only
// by the explicit information environment.
Observation observe_with_information(
    const WorldRef& world,
    const AgentRef& agent,
    const InformationPolicy& policy,
    const InformationEnvironment& information,
    std::map<std::string, std::string> public_fields = {},
    std::map<std::string, std::string> private_fields = {});

} // namespace coagentics::domain
