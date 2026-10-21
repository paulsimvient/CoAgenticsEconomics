#pragma once

#include "coagentics/domain/InformationEnvironment.hpp"
#include "coagentics/market/Mechanism.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace coagentics::domain {

// Section 4: executable world boundary for the existing economic laboratory.
// This is the first concrete DomainEnvironment implementation. It translates
// the domain-neutral Action contract into the existing market mechanism,
// records acceptance separately from execution/fill, advances world time, and
// returns auditable outcome metrics.
class EconomicEnvironment final : public DomainEnvironment {
public:
    struct Config {
        std::string scenario_id{"economic-market"};
        std::string scenario_version{"section4"};
        std::string default_asset{"ASSET"};
        market::MarketConfig market;
        market::MechanismKind mechanism{market::MechanismKind::ContinuousDoubleAuction};
    };

    explicit EconomicEnvironment(Config config);

    WorldRef reset(const RunSpec&) override;
    Observation observe(const AgentRef&, const InformationPolicy&) const override;
    ActionResult validate(const AgentInvocation&, const Action&) const override;
    TransitionResult step(const AgentInvocation&, const ActionResult&) override;

    void add_account(const std::string& agent_id, market::Account account);
    void publish_information(InformationEvent event);

    const market::MarketMechanism& mechanism() const { return *mechanism_; }
    const WorldRef& world() const { return world_; }
    const std::vector<market::Trade>& trades() const { return mechanism_->trades(); }

private:
    Config config_;
    WorldRef world_;
    std::unique_ptr<market::MarketMechanism> mechanism_;
    ExplicitInformationEnvironment information_;
    std::unordered_map<std::string, market::Account> initial_accounts_;
    std::unordered_map<std::string, InformationPolicy> policies_;

    static std::string text_number(double value);
    static std::string fingerprint_transition(const TransitionResult&);
    std::string action_asset(const Action&) const;
    int action_quantity(const Action&) const;
    double action_price(const Action&) const;
};

} // namespace coagentics::domain
