#include "coagentics/analysis/LlmBehavioralProfile.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace coagentics::analysis {
namespace {

// Acklam's rational approximation for the standard normal inverse CDF.
double normal_quantile(double p) {
    if (!(p > 0.0 && p < 1.0)) throw std::invalid_argument("normal_probability_out_of_range");
    constexpr double a1=-3.969683028665376e+01, a2=2.209460984245205e+02,
                     a3=-2.759285104469687e+02, a4=1.383577518672690e+02,
                     a5=-3.066479806614716e+01, a6=2.506628277459239e+00;
    constexpr double b1=-5.447609879822406e+01, b2=1.615858368580409e+02,
                     b3=-1.556989798598866e+02, b4=6.680131188771972e+01,
                     b5=-1.328068155288572e+01;
    constexpr double c1=-7.784894002430293e-03, c2=-3.223964580411365e-01,
                     c3=-2.400758277161838e+00, c4=-2.549732539343734e+00,
                     c5=4.374664141464968e+00, c6=2.938163982698783e+00;
    constexpr double d1=7.784695709041462e-03, d2=3.224671290700398e-01,
                     d3=2.445134137142996e+00, d4=3.754408661907416e+00;
    constexpr double plow=0.02425, phigh=1.0-plow;
    if (p < plow) {
        const double q=std::sqrt(-2.0*std::log(p));
        return (((((c1*q+c2)*q+c3)*q+c4)*q+c5)*q+c6) /
               ((((d1*q+d2)*q+d3)*q+d4)*q+1.0);
    }
    if (p > phigh) {
        const double q=std::sqrt(-2.0*std::log(1.0-p));
        return -(((((c1*q+c2)*q+c3)*q+c4)*q+c5)*q+c6) /
                ((((d1*q+d2)*q+d3)*q+d4)*q+1.0);
    }
    const double q=p-0.5, r=q*q;
    return (((((a1*r+a2)*r+a3)*r+a4)*r+a5)*r+a6)*q /
           (((((b1*r+b2)*r+b3)*r+b4)*r+b5)*r+1.0);
}

std::string dimension_name(const std::string& id) {
    if (id.find("news") != std::string::npos || id.find("information") != std::string::npos) return "information_sensitivity";
    if (id.find("price") != std::string::npos || id.find("momentum") != std::string::npos) return "recent_price_dependence";
    if (id.find("peer") != std::string::npos) return "peer_dependence";
    if (id.find("repeat") != std::string::npos || id.find("adapt") != std::string::npos) return "persistence_adaptation";
    return id;
}

struct PairedStats { double effect; double se; };

PairedStats paired_stats(const ProbeObservation& p) {
    const double n=static_cast<double>(p.treatment_n);
    const double mean=(static_cast<double>(p.treatment_only)-static_cast<double>(p.control_only))/n;
    const double second=(static_cast<double>(p.treatment_only)+static_cast<double>(p.control_only))/n;
    const double variance=std::max(0.0, second-mean*mean);
    return {mean, std::sqrt(variance/n)};
}
}

std::string claim_status_string(ClaimStatus status) {
    switch (status) {
        case ClaimStatus::Supports: return "SUPPORTS";
        case ClaimStatus::Challenges: return "CHALLENGES";
        default: return "INCONCLUSIVE";
    }
}

LlmBehavioralProfile build_llm_behavioral_profile(
    std::string provider, std::string model, std::string version,
    const std::vector<ProbeObservation>& probes, double minimum_effect, double alpha) {
    if (provider.empty() || model.empty() || version.empty()) throw std::invalid_argument("model_identity_required");
    if (probes.empty()) throw std::invalid_argument("at_least_one_probe_required");
    if (!(minimum_effect >= 0.0 && minimum_effect <= 1.0)) throw std::invalid_argument("minimum_effect_out_of_range");
    if (!(alpha > 0.0 && alpha < 1.0)) throw std::invalid_argument("alpha_out_of_range");

    LlmBehavioralProfile out;
    out.provider=std::move(provider); out.model=std::move(model); out.version=std::move(version);
    out.probes_used=probes.size();
    out.paired_analysis=true;
    const double adjusted_alpha=alpha/static_cast<double>(probes.size());
    const double z=normal_quantile(1.0-adjusted_alpha/2.0);

    for (const auto& p : probes) {
        if (!p.paired) throw std::invalid_argument("paired_observations_required_for_claims");
        if (p.treatment_n == 0 || p.control_n == 0 || p.treatment_n != p.control_n)
            throw std::invalid_argument("paired_probe_requires_equal_partition_sizes");
        if (p.treatment_rate < 0 || p.treatment_rate > 1 || p.control_rate < 0 || p.control_rate > 1)
            throw std::invalid_argument("probe_rate_out_of_range");
        if (p.treatment_only + p.control_only > p.treatment_n)
            throw std::invalid_argument("discordant_pairs_exceed_matched_n");
        if (p.expected_delta < -1.0 || p.expected_delta > 1.0)
            throw std::invalid_argument("expected_delta_out_of_range");

        const auto stats=paired_stats(p);
        const double rate_effect=p.treatment_rate-p.control_rate;
        if (std::abs(rate_effect-stats.effect) > 1e-9)
            throw std::invalid_argument("paired_counts_inconsistent_with_rates");

        BehavioralDimension d;
        d.name=dimension_name(p.probe_id);
        d.probe_id=p.probe_id;
        d.effect=stats.effect;
        d.standard_error=stats.se;
        d.adjusted_alpha=adjusted_alpha;
        d.lower_95=stats.effect-z*stats.se;
        d.upper_95=stats.effect+z*stats.se;
        d.positive_signal=stats.effect >= minimum_effect;

        const double direction = p.expected_delta != 0.0 ? p.expected_delta : stats.effect;
        if (direction > 0.0) {
            d.supported=d.lower_95 >= minimum_effect;
            d.status=d.supported ? ClaimStatus::Supports :
                      (d.upper_95 < minimum_effect ? ClaimStatus::Challenges : ClaimStatus::Inconclusive);
        } else if (direction < 0.0) {
            d.supported=d.upper_95 <= -minimum_effect;
            d.status=d.supported ? ClaimStatus::Supports :
                      (d.lower_95 > -minimum_effect ? ClaimStatus::Challenges : ClaimStatus::Inconclusive);
        } else {
            d.supported=(d.lower_95 <= minimum_effect && d.upper_95 >= -minimum_effect);
            d.status=ClaimStatus::Inconclusive;
        }
        out.total_observations += p.treatment_n + p.control_n;
        out.dimensions.push_back(std::move(d));
    }
    out.sufficient_evidence=out.total_observations >= 20 && !out.dimensions.empty();
    return out;
}

std::string llm_behavioral_profile_markdown(const LlmBehavioralProfile& p) {
    std::ostringstream o;
    o << "# Model behavioral profile\n\n"
      << "Model: **" << p.provider << " / " << p.model << " / " << p.version << "**\n\n"
      << "Probes: " << p.probes_used << "\n\n"
      << "Total observations: " << p.total_observations << "\n\n"
      << "Analysis: **matched-pair**; family-wise alpha adjusted across probes\n\n"
      << "| Dimension | Effect | 95% CI | Status |\n|---|---:|---:|---|\n";
    for (const auto& d : p.dimensions) {
        o << "| " << d.name << " | " << std::fixed << std::setprecision(3) << d.effect
          << " | [" << d.lower_95 << ", " << d.upper_95 << "] | "
          << claim_status_string(d.status) << " |\n";
    }
    o << "\nEvidence sufficient for a profile: **" << (p.sufficient_evidence ? "YES" : "NO") << "**\n\n"
      << "Claim boundary: " << p.claim_boundary << "\n";
    return o.str();
}
}
