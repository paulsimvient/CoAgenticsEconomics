#include "coagentics/domain/InformationEnvironment.hpp"

#include <algorithm>
#include <stdexcept>

namespace coagentics::domain {

namespace {

bool source_allowed(const InformationEvent& event, const InformationPolicy& policy) {
    const std::string& source = event.source_class;
    if ((source == "public-news" || source == "news") && !policy.allow_public_news) return false;
    if ((source == "price-history" || source == "market-history") && !policy.allow_price_history) return false;
    if ((source == "peer-action" || source == "peer-actions") && !policy.allow_peer_actions) return false;
    if ((source == "private-state" || source == "private") && !policy.allow_private_state) return false;
    return true;
}

bool audience_allowed(const InformationEvent& event, const AgentRef& agent) {
    if (!event.visible_to || event.visible_to->empty()) return true;
    return *event.visible_to == agent.agent_id || *event.visible_to == agent.role;
}

bool time_allowed(const InformationEvent& event, std::uint64_t tick,
                  const InformationPolicy& policy) {
    // A delayed event cannot be observed until both its explicit visibility time
    // and the policy delay have elapsed.
    const std::uint64_t policy_visible_at =
        event.observed_at > UINT64_MAX - policy.information_delay
            ? UINT64_MAX
            : event.observed_at + policy.information_delay;
    const std::uint64_t visible_at =
        event.visible_after ? std::max(policy_visible_at, *event.visible_after)
                            : policy_visible_at;
    if (tick < visible_at) return false;

    if (event.expires_at && tick >= *event.expires_at) return false;

    if (policy.history_window > 0 && tick > event.observed_at &&
        tick - event.observed_at > policy.history_window) return false;
    return true;
}

} // namespace

void ExplicitInformationEnvironment::publish(InformationEvent event) {
    std::string why;
    if (!valid_information_event(event, &why)) {
        throw std::invalid_argument("invalid information event: " + why);
    }
    std::scoped_lock lock(mutex_);
    events_.push_back(std::move(event));
}

std::vector<InformationEvent> ExplicitInformationEnvironment::visible_to(
    const AgentRef& agent, std::uint64_t tick) const {
    return visible_to(agent, tick, InformationPolicy{});
}

std::vector<InformationEvent> ExplicitInformationEnvironment::visible_to(
    const AgentRef& agent, std::uint64_t tick, const InformationPolicy& policy) const {
    std::scoped_lock lock(mutex_);
    std::vector<InformationEvent> result;
    result.reserve(events_.size());
    for (const auto& event : events_) {
        if (!source_allowed(event, policy)) continue;
        if (!audience_allowed(event, agent)) continue;
        if (!time_allowed(event, tick, policy)) continue;
        result.push_back(event);
    }
    return result;
}

std::size_t ExplicitInformationEnvironment::size() const {
    std::scoped_lock lock(mutex_);
    return events_.size();
}

Observation observe_with_information(
    const WorldRef& world,
    const AgentRef& agent,
    const InformationPolicy& policy,
    const InformationEnvironment& information,
    std::map<std::string, std::string> public_fields,
    std::map<std::string, std::string> private_fields) {
    Observation observation;
    observation.world = world;
    observation.agent = agent;
    observation.policy = policy;
    observation.public_fields = std::move(public_fields);
    observation.private_fields = std::move(private_fields);

    const auto* explicit_env = dynamic_cast<const ExplicitInformationEnvironment*>(&information);
    if (explicit_env) {
        observation.information = explicit_env->visible_to(agent, world.tick, policy);
    } else {
        // A custom implementation owns its own visibility semantics.
        observation.information = information.visible_to(agent, world.tick);
    }
    return observation;
}

} // namespace coagentics::domain
