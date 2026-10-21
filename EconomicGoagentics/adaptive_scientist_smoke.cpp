#include "AdaptiveScientist.hpp"
#include <cassert>
#include <iostream>

using namespace coagentics::science;

int main() {
    std::vector<Hypothesis> h{
        {"information", "behavior responds to information"},
        {"price", "behavior responds to recent price"},
        {"peer", "behavior responds to peer action"}
    };
    std::vector<CandidateExperiment> c{
        {"news_only", "change information while holding price and peers fixed", 1.0, true, {1,0,0}},
        {"price_only", "change price while holding information and peers fixed", 1.0, true, {0,1,0}},
        {"peer_only", "change peer action while holding information and price fixed", 1.0, true, {0,0,1}},
        {"confounded", "change everything together", 1.0, true, {1,1,1}},
        {"unavailable", "not runnable", 0.1, false, {1,0,0}}
    };
    auto r = select_next_experiment(h,c);
    assert(r.selected);
    assert(!r.rationale.selected_experiment_id.empty());
    assert(r.rationale.distinguishable_hypothesis_pairs == 2);
    assert(r.rationale.discrimination_score > 0.0);

    std::vector<CandidateExperiment> no_discrimination{{"same", "all hypotheses predict the same outcome", 1.0, true, {1,1,1}}};
    auto n = select_next_experiment(h,no_discrimination);
    assert(!n.selected);
    assert(n.rationale.selected_experiment_id.empty());

    std::cout << "adaptive_scientist_smoke PASS\n";
    std::cout << r.rationale.rationale << "\n";
    std::cout << "no_discrimination gate PASS\n";
}
