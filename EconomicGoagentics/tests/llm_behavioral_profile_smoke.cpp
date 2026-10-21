#include "coagentics/analysis/LlmBehavioralProfile.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace coagentics::analysis;

int main() {
    // Each probe has 50 matched pairs. The rates are implied by the paired
    // discordant counts so the estimator cannot silently revert to an
    // independent-proportion calculation.
    std::vector<ProbeObservation> probes{
        {"news",   0.82, 0.40, 50, 50, 0.42, 24, 3, true},
        {"price",  0.68, 0.56, 50, 50, 0.12,  8, 2, true},
        {"peer",   0.46, 0.44, 50, 50, 0.02,  2, 1, true},
        {"repeat", 0.70, 0.52, 50, 50, 0.18, 12, 3, true}
    };
    auto p=build_llm_behavioral_profile("test", "unknown-llm", "v1", probes, 0.10);
    assert(p.probes_used==4);
    assert(p.total_observations==400);
    assert(p.sufficient_evidence);
    assert(p.paired_analysis);
    assert(p.dimensions.size()==4);
    assert(p.dimensions[0].name=="information_sensitivity");
    assert(std::abs(p.dimensions[0].effect-0.42)<1e-12);
    assert(p.dimensions[0].supported);
    assert(p.dimensions[0].status==ClaimStatus::Supports);
    assert(p.dimensions[2].name=="peer_dependence");
    assert(p.dimensions[2].status==ClaimStatus::Inconclusive);
    const auto md=llm_behavioral_profile_markdown(p);
    assert(md.find("unknown-llm")!=std::string::npos);
    assert(md.find("matched-pair")!=std::string::npos);
    std::cout << "llm_behavioral_profile_smoke PASS\n" << md;
}
