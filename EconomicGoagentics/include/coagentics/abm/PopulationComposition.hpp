#pragma once

#include "coagentics/abm/AdvancedPopulation.hpp"
#include "coagentics/domain/AgentBoundary.hpp"
#include "coagentics/domain/AdvancedContracts.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace coagentics::abm {

enum class ReferenceStrategy {
    FixedPrice,
    SignalResponsive,
    RiskSensitive,
    PeerResponsive,
    Adaptive,
    Hold
};

const char* reference_strategy_name(ReferenceStrategy strategy);

struct ModelSlot {
    std::string provider;
    std::string model;
    std::string version;
};

struct RoleSlot {
    std::string role;
    std::string side; // domain-facing metadata: buyer | seller | neutral
};

struct AgentSlotSpec {
    std::string seat_id;
    std::string role;
    std::string side;
    std::string provider;
    std::string model;
    std::string model_version;
    ReferenceStrategy reference_strategy{ReferenceStrategy::Hold};
    double reservation_value{100.0};
    double strategy_parameter{0.0};
    std::uint64_t private_seed{0};
    domain::InformationPolicy information_policy{};
};

struct PopulationCompositionSpec {
    std::uint64_t seed{424242};
    std::size_t seats{10};
    std::vector<ModelSlot> models;
    std::vector<RoleSlot> roles;
    std::vector<ReferenceStrategy> reference_strategies;
    std::vector<double> reservation_values;
    bool rotate_models{true};
    bool rotate_roles{true};
    bool rotate_strategies{true};
    bool rotate_values{true};
    std::string composition_id{"population-composition-v1"};
};

struct ComposedPopulation {
    std::vector<PopulationAgent> agents;
    std::vector<std::unique_ptr<domain::AgentAdapter>> owned_adapters;
    std::vector<AgentSlotSpec> assignments;
    std::string assignment_fingerprint;
};

// Creates a population from reusable model/role/strategy slots. Assignment rotation is
// deterministic for a given seed and intentionally independent of adapter implementation.
ComposedPopulation compose_population(const PopulationCompositionSpec& spec);

// Returns only the assignment metadata. Useful for preregistration, balancing checks,
// and proving that a seed changes seat identity rather than the underlying model catalog.
std::vector<AgentSlotSpec> balanced_assignments(const PopulationCompositionSpec& spec);

} // namespace coagentics::abm
