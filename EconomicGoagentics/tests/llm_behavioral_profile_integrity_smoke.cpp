#include "coagentics/analysis/LlmBehavioralProfile.hpp"
#include <cassert>
#include <iostream>
#include <stdexcept>
using namespace coagentics::analysis;

static ProbeObservation good() { return {"news", .70, .50, 20, 20, .20, 5, 1, true}; }

int main() {
    bool threw=false;
    try { auto x=good(); x.paired=false; (void)build_llm_behavioral_profile("p","m","v",{x}); }
    catch (const std::invalid_argument&) { threw=true; }
    assert(threw);

    threw=false;
    try { auto x=good(); x.treatment_rate=.80; (void)build_llm_behavioral_profile("p","m","v",{x}); }
    catch (const std::invalid_argument&) { threw=true; }
    assert(threw);

    auto no_effect=ProbeObservation{"news", .50, .50, 40, 40, .20, 4, 4, true};
    auto p=build_llm_behavioral_profile("p","m","v",{no_effect},.10);
    assert(p.dimensions[0].status==ClaimStatus::Inconclusive);

    auto challenged=ProbeObservation{"news", .30, .60, 40, 40, .20, 2, 14, true};
    auto q=build_llm_behavioral_profile("p","m","v",{challenged},.10);
    assert(q.dimensions[0].status==ClaimStatus::Challenges);

    std::cout << "llm_behavioral_profile_integrity_smoke PASS\n";
}
