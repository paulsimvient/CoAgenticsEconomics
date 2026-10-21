#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace coagentics::analysis {

enum class KnownMechanism { InformationBlind, SignalResponsive, RiskSensitive, PeerResponsive, Adaptive };
const char* known_mechanism_name(KnownMechanism);

struct MechanismProbeResult {
    std::string mechanism;
    std::string predicted;
    double information_sensitivity{};
    double peer_sensitivity{};
    double risk_attenuation{};
    double repeated_response_change{};
    bool recovered{false};
};

struct ReferenceMechanismCalibrationReport {
    std::uint64_t seed{};
    std::size_t trials{};
    std::size_t correct{};
    double recovery_rate{};
    double null_false_positive_rate{};
    bool pass{false};
    std::vector<MechanismProbeResult> results;
};

ReferenceMechanismCalibrationReport calibrate_reference_mechanisms(
    std::uint64_t seed=91026, std::size_t trials_per_mechanism=20);
std::string reference_mechanism_calibration_markdown(const ReferenceMechanismCalibrationReport&);
std::string reference_mechanism_calibration_json(const ReferenceMechanismCalibrationReport&);

} // namespace coagentics::analysis
