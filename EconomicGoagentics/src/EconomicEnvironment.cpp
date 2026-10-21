#include "coagentics/domain/EconomicEnvironment.hpp"

#include <cmath>
#include <iomanip>
#include <limits>
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

bool has_asset(const market::MarketConfig& c, const std::string& asset) {
    return c.fundamental_value.find(asset) != c.fundamental_value.end();
}

} // namespace

EconomicEnvironment::EconomicEnvironment(Config config) : config_(std::move(config)) {
    if (config_.scenario_id.empty()) throw std::invalid_argument("EconomicEnvironment requires scenario_id");
    if (config_.scenario_version.empty()) throw std::invalid_argument("EconomicEnvironment requires scenario_version");
    if (config_.default_asset.empty()) throw std::invalid_argument("EconomicEnvironment requires default_asset");
    if (!has_asset(config_.market, config_.default_asset)) {
        throw std::invalid_argument("default_asset_not_in_market_config");
    }
    mechanism_ = market::make_mechanism(config_.mechanism, config_.market);
}

void EconomicEnvironment::add_account(const std::string& agent_id, market::Account account) {
    if (agent_id.empty()) throw std::invalid_argument("missing_agent_id");
    initial_accounts_[agent_id] = account;
    mechanism_->add_account(agent_id, std::move(account));
}

void EconomicEnvironment::publish_information(InformationEvent event) {
    information_.publish(std::move(event));
}

WorldRef EconomicEnvironment::reset(const RunSpec& run) {
    std::string why;
    if (!valid_run_spec(run, &why)) throw std::invalid_argument("invalid_run_spec: " + why);

    mechanism_ = market::make_mechanism(config_.mechanism, config_.market);
    for (const auto& [id, account] : initial_accounts_) mechanism_->add_account(id, account);
    world_ = {config_.scenario_id, config_.scenario_version, run.seed, 0};
    return world_;
}

std::string EconomicEnvironment::text_number(double value) {
    std::ostringstream s;
    s << std::setprecision(17) << value;
    return s.str();
}

std::string EconomicEnvironment::action_asset(const Action& action) const {
    auto it = action.categorical_parameters.find("asset");
    return it == action.categorical_parameters.end() || it->second.empty()
        ? config_.default_asset : it->second;
}

int EconomicEnvironment::action_quantity(const Action& action) const {
    auto it = action.numeric_parameters.find("quantity");
    if (it == action.numeric_parameters.end()) return 0;
    if (!std::isfinite(it->second) || it->second <= 0.0 || it->second > static_cast<double>(std::numeric_limits<int>::max())) return 0;
    const int q = static_cast<int>(std::llround(it->second));
    return (std::abs(it->second - static_cast<double>(q)) < 1e-9 && q > 0) ? q : 0;
}

double EconomicEnvironment::action_price(const Action& action) const {
    auto it = action.numeric_parameters.find("price");
    return it == action.numeric_parameters.end() ? std::numeric_limits<double>::quiet_NaN() : it->second;
}

Observation EconomicEnvironment::observe(const AgentRef& agent, const InformationPolicy& policy) const {
    std::map<std::string, std::string> pub;
    const std::string asset = config_.default_asset;
    if (policy.allow_price_history) {
        pub["best_bid"] = text_number(mechanism_->best_bid_opt(asset).value_or(0.0));
        pub["best_ask"] = text_number(mechanism_->best_ask_opt(asset).value_or(0.0));
        // The existing mechanism interface exposes last trade only through its
        // trade ledger, so use the most recent trade without exposing private values.
        pub["last_trade"] = mechanism_->trades().empty()
            ? "0" : text_number(mechanism_->trades().back().price);
    }

    std::map<std::string, std::string> priv;
    if (policy.allow_private_state) {
        auto it = mechanism_->accounts().find(agent.agent_id);
        if (it != mechanism_->accounts().end()) {
            priv["cash"] = text_number(it->second.cash);
            auto inv = it->second.inventory.find(asset);
            priv["inventory"] = std::to_string(inv == it->second.inventory.end() ? 0 : inv->second);
        }
    }
    return observe_with_information(world_, agent, policy, information_, std::move(pub), std::move(priv));
}

ActionResult EconomicEnvironment::validate(const AgentInvocation& invocation, const Action& action) const {
    ActionResult result;
    result.action = action;
    if (action.action_type == "HOLD" || action.action_type == "ABSTAIN") {
        result.accepted = true; result.abstained = true; result.status = "abstained"; return result;
    }
    if (action.action_type != "BUY" && action.action_type != "SELL") {
        result.status = "rejected"; result.rejection_reason = "unsupported_action_type"; return result;
    }
    const std::string asset = action_asset(action);
    if (!has_asset(config_.market, asset)) {
        result.status = "rejected"; result.rejection_reason = "unknown_asset"; return result;
    }
    const int q = action_quantity(action);
    const double price = action_price(action);
    if (q <= 0) { result.status = "rejected"; result.rejection_reason = "invalid_quantity"; return result; }
    if (!std::isfinite(price) || price < 0.0) { result.status = "rejected"; result.rejection_reason = "invalid_price"; return result; }

    auto account = mechanism_->accounts().find(invocation.agent.agent_id);
    if (account == mechanism_->accounts().end()) {
        result.status = "rejected"; result.rejection_reason = "unknown_agent"; return result;
    }
    if (action.action_type == "BUY" && account->second.cash + 1e-9 < price * q) {
        result.status = "rejected"; result.rejection_reason = "insufficient_cash"; return result;
    }
    if (action.action_type == "SELL") {
        auto inv = account->second.inventory.find(asset);
        const int held = inv == account->second.inventory.end() ? 0 : inv->second;
        if (held < q) { result.status = "rejected"; result.rejection_reason = "insufficient_inventory"; return result; }
    }
    result.accepted = true; result.status = "accepted";
    return result;
}

TransitionResult EconomicEnvironment::step(const AgentInvocation& invocation, const ActionResult& action_result) {
    TransitionResult transition;
    transition.next_world = world_;
    transition.next_world.tick += 1;

    if (!action_result.accepted || action_result.abstained) {
        transition.outcomes["accepted"] = action_result.accepted ? 1.0 : 0.0;
        transition.outcomes["abstained"] = action_result.abstained ? 1.0 : 0.0;
        transition.outcomes["filled_quantity"] = 0.0;
        transition.outcomes["market_trades_total"] = static_cast<double>(mechanism_->trades().size());
        world_ = transition.next_world;
        transition.outcomes["outcome_fingerprint"] = 0.0; // numeric map cannot carry string; see evidence layer for canonical ID.
        return transition;
    }

    const Action& a = action_result.action;
    market::Bid bid;
    bid.time = world_.tick;
    bid.agent_id = invocation.agent.agent_id;
    bid.asset = action_asset(a);
    bid.quantity = action_quantity(a);
    bid.price = action_price(a);
    bid.side = a.action_type == "BUY" ? market::Side::Buy : market::Side::Sell;

    const auto detailed = mechanism_->submit_detailed(bid);
    transition.outcomes["accepted"] = detailed.accepted ? 1.0 : 0.0;
    transition.outcomes["filled_quantity"] = static_cast<double>(detailed.filled_quantity);
    transition.outcomes["resting_quantity"] = static_cast<double>(detailed.resting_quantity);
    transition.outcomes["market_trades_total"] = static_cast<double>(mechanism_->trades().size());
    transition.outcomes["allocative_efficiency"] = mechanism_->metrics().allocative_efficiency;
    transition.outcomes["realized_surplus"] = mechanism_->metrics().realized_surplus;
    transition.outcomes["maximum_surplus"] = mechanism_->metrics().maximum_surplus;
    if (detailed.average_fill_price) transition.outcomes["average_fill_price"] = *detailed.average_fill_price;

    // Sealed-bid auctions require explicit clearing. CDA clears on submit.
    if (mechanism_->kind() == market::MechanismKind::SealedBidDoubleAuction) {
        mechanism_->clear(world_.tick);
        transition.outcomes["market_trades_total"] = static_cast<double>(mechanism_->trades().size());
        transition.outcomes["allocative_efficiency"] = mechanism_->metrics().allocative_efficiency;
    }

    const std::size_t before = mechanism_->trades().size() - detailed.filled_quantity;
    for (std::size_t i = before; i < mechanism_->trades().size(); ++i) {
        transition.execution_ids.push_back("trade-" + std::to_string(i));
    }

    world_ = transition.next_world;
    return transition;
}

std::string EconomicEnvironment::fingerprint_transition(const TransitionResult& t) {
    std::ostringstream s;
    s << t.next_world.scenario_id << ':' << t.next_world.scenario_version << ':'
      << t.next_world.seed << ':' << t.next_world.tick;
    for (const auto& id : t.execution_ids) s << "|exec:" << id;
    for (const auto& [k,v] : t.outcomes) s << "|" << k << '=' << std::setprecision(17) << v;
    return hex64(fnv1a(s.str()));
}

} // namespace coagentics::domain
