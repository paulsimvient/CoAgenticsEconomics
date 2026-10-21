#include "coagentics/abm/PopulationComposition.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace coagentics::abm {
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

std::size_t rotation(std::uint64_t seed, std::size_t n) {
    if (n == 0) return 0;
    return static_cast<std::size_t>(seed % static_cast<std::uint64_t>(n));
}

std::string canonical_assignment(const std::vector<AgentSlotSpec>& a) {
    std::ostringstream out;
    for (const auto& x : a) {
        out << x.seat_id << '|' << x.role << '|' << x.side << '|'
            << x.provider << '|' << x.model << '|' << x.model_version << '|'
            << reference_strategy_name(x.reference_strategy) << '|'
            << std::setprecision(17) << x.reservation_value << '|'
            << x.strategy_parameter << '|' << x.private_seed << '\n';
    }
    return out.str();
}

class ReferencePolicyAdapter final : public domain::AgentAdapter {
public:
    ReferencePolicyAdapter(ReferenceStrategy strategy, double reservation, double parameter)
        : strategy_(strategy), reservation_(reservation), parameter_(parameter) {}

    domain::Action decide(const domain::AgentInvocation& invocation) override {
        const auto& o = invocation.observation;
        domain::Action a;
        a.action_type = "HOLD";
        a.categorical_parameters["asset"] = "ASSET";

        double signal = 0.0;
        for (const auto& e : o.information) signal += e.relevance * e.reliability;
        auto find_num = [&](const char* key, double fallback) {
            auto it = o.public_fields.find(key);
            if (it == o.public_fields.end()) return fallback;
            try { return std::stod(it->second); } catch (...) { return fallback; }
        };
        const double reference = find_num("reference_value", reservation_);
        const double last = find_num("last_trade", reference);
        const double peer = find_num("peer_action_score", 0.0);
        const bool seller = invocation.agent.role.find("seller") != std::string::npos;

        double price = reservation_;
        switch (strategy_) {
            case ReferenceStrategy::FixedPrice:
                price = reservation_;
                break;
            case ReferenceStrategy::SignalResponsive:
                price = reservation_ + parameter_ * signal;
                break;
            case ReferenceStrategy::RiskSensitive:
                price = reservation_ + signal - std::abs(parameter_) * std::abs(signal);
                break;
            case ReferenceStrategy::PeerResponsive:
                price = reservation_ + parameter_ * peer;
                break;
            case ReferenceStrategy::Adaptive:
                price = reservation_ + signal + parameter_ * (last - reference);
                break;
            case ReferenceStrategy::Hold:
                return a;
        }
        a.action_type = seller ? "SELL" : "BUY";
        a.numeric_parameters["price"] = std::max(0.0, price);
        a.numeric_parameters["quantity"] = 1.0;
        return a;
    }

private:
    ReferenceStrategy strategy_;
    double reservation_;
    double parameter_;
};

} // namespace

const char* reference_strategy_name(ReferenceStrategy strategy) {
    switch (strategy) {
        case ReferenceStrategy::FixedPrice: return "fixed_price";
        case ReferenceStrategy::SignalResponsive: return "signal_responsive";
        case ReferenceStrategy::RiskSensitive: return "risk_sensitive";
        case ReferenceStrategy::PeerResponsive: return "peer_responsive";
        case ReferenceStrategy::Adaptive: return "adaptive";
        case ReferenceStrategy::Hold: return "hold";
    }
    return "unknown";
}

std::vector<AgentSlotSpec> balanced_assignments(const PopulationCompositionSpec& spec) {
    if (spec.seats == 0) throw std::invalid_argument("composition_seats_must_be_positive");
    if (spec.models.empty()) throw std::invalid_argument("composition_models_must_not_be_empty");
    if (spec.roles.empty()) throw std::invalid_argument("composition_roles_must_not_be_empty");
    if (spec.reference_strategies.empty()) throw std::invalid_argument("composition_strategies_must_not_be_empty");
    if (spec.reservation_values.empty()) throw std::invalid_argument("composition_values_must_not_be_empty");

    std::vector<AgentSlotSpec> out;
    out.reserve(spec.seats);
    const auto mr = spec.rotate_models ? rotation(spec.seed, spec.models.size()) : 0;
    const auto rr = spec.rotate_roles ? rotation(spec.seed + 17, spec.roles.size()) : 0;
    const auto sr = spec.rotate_strategies ? rotation(spec.seed + 31, spec.reference_strategies.size()) : 0;
    const auto vr = spec.rotate_values ? rotation(spec.seed + 47, spec.reservation_values.size()) : 0;

    for (std::size_t seat = 0; seat < spec.seats; ++seat) {
        const auto& m = spec.models[(seat + mr) % spec.models.size()];
        const auto& role = spec.roles[(seat + rr) % spec.roles.size()];
        const auto strategy = spec.reference_strategies[(seat + sr) % spec.reference_strategies.size()];
        const double value = spec.reservation_values[(seat + vr) % spec.reservation_values.size()];
        AgentSlotSpec a;
        a.seat_id = "SEAT-" + std::to_string(seat);
        a.role = role.role;
        a.side = role.side;
        a.provider = m.provider;
        a.model = m.model;
        a.model_version = m.version;
        a.reference_strategy = strategy;
        a.reservation_value = value;
        a.strategy_parameter = 1.0;
        a.private_seed = spec.seed + static_cast<std::uint64_t>(seat) * 7919ULL;
        a.information_policy.allow_peer_actions = role.role.find("peer") != std::string::npos;
        out.push_back(std::move(a));
    }
    return out;
}

ComposedPopulation compose_population(const PopulationCompositionSpec& spec) {
    ComposedPopulation result;
    result.assignments = balanced_assignments(spec);
    result.owned_adapters.reserve(result.assignments.size());
    result.agents.reserve(result.assignments.size());

    for (const auto& a : result.assignments) {
        auto adapter = std::make_unique<ReferencePolicyAdapter>(
            a.reference_strategy, a.reservation_value, a.strategy_parameter);
        auto* raw = adapter.get();
        result.owned_adapters.push_back(std::move(adapter));

        PopulationAgent p;
        p.ref.agent_id = a.seat_id;
        p.ref.role = a.role.empty() ? a.side : a.role;
        p.ref.provider = a.provider;
        p.ref.model = a.model;
        p.ref.model_version = a.model_version;
        p.information_policy = a.information_policy;
        p.adapter = raw;
        p.active = true;
        result.agents.push_back(std::move(p));
    }

    result.assignment_fingerprint = hex64(fnv1a(canonical_assignment(result.assignments)));
    return result;
}

} // namespace coagentics::abm
