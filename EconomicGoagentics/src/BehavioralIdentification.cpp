#include "coagentics/analysis/BehavioralIdentification.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <stdexcept>

namespace coagentics::analysis {
namespace {
// Keep prediction logic identical to the scientist's declared candidate model.
double predict(const experiment::Candidate& c, const experiment::Probe& p) {
  return c.news_gain * p.news + c.peer_gain * p.peer +
         c.adaptation_gain * (p.repetitions - 1) * p.news;
}
}

BehavioralIdentificationReport identify_and_validate(
    const std::string& hidden_mechanism,
    const std::vector<experiment::Candidate>& candidates,
    const std::vector<experiment::Probe>& development_probes,
    const std::vector<experiment::Probe>& heldout_probes,
    const IdentificationConfig& cfg) {
  if (candidates.size() < 2 || development_probes.empty() || heldout_probes.empty())
    throw std::invalid_argument("identification requires candidates and both probe partitions");
  if (cfg.heldout_replicates < 2 || cfg.development_replicates < 2)
    throw std::invalid_argument("replicates must be >= 2");

  // IMPORTANT: the analysis sees a black-box domain. The hidden mechanism is used only
  // by the domain adapter to generate observations; it is never supplied to the scientist.
  experiment::AgentProbeDomain domain(hidden_mechanism);
  experiment::ScientistConfig sc;
  sc.replicates = cfg.development_replicates;
  sc.max_experiments = cfg.development_budget;
  sc.noise_sd = cfg.noise_sd;
  sc.stop_posterior = cfg.stop_posterior;
  sc.minimum_separation = cfg.minimum_separation;
  auto development = experiment::run_scientist(domain, candidates, development_probes,
                                                 cfg.development_seed, sc);

  BehavioralIdentificationReport out;
  // Deliberately expose only an opaque test label in the report; the caller/test harness
  // retains the actual mechanism as ground truth for evaluation.
  out.hidden_agent_label = "opaque-agent";
  out.development_status = development.status;
  out.development_steps = development.steps;
  out.frozen_hypothesis = development.leading_hypothesis;
  out.explanation_frozen = !out.frozen_hypothesis.empty();

  // Freeze here. No held-out observation is fed back into the scientist.
  std::vector<double> sse(candidates.size(), 0.0), sae(candidates.size(), 0.0);
  std::size_t n = 0;
  for (std::size_t pi = 0; pi < heldout_probes.size(); ++pi) {
    const auto& p = heldout_probes[pi];
    for (std::size_t r = 0; r < cfg.heldout_replicates; ++r) {
      auto obs = domain.run(p, cfg.heldout_seed + 100000 * pi + r);
      const double y = obs.treatment - obs.control;
      ++n;
      for (std::size_t j = 0; j < candidates.size(); ++j) {
        const double err = y - predict(candidates[j], p);
        sse[j] += err * err;
        sae[j] += std::abs(err);
      }
    }
  }
  out.heldout_trials = n;
  out.heldout_scores.reserve(candidates.size());
  for (std::size_t j = 0; j < candidates.size(); ++j)
    out.heldout_scores.push_back({candidates[j].id, sse[j] / n, sae[j] / n});

  auto best = std::min_element(out.heldout_scores.begin(), out.heldout_scores.end(),
                               [](const auto& a, const auto& b) { return a.mse < b.mse; });
  out.heldout_winner_matches_frozen = out.explanation_frozen && best != out.heldout_scores.end() &&
                                      best->hypothesis_id == out.frozen_hypothesis;
  if (best != out.heldout_scores.end()) {
    double second = std::numeric_limits<double>::infinity();
    for (const auto& s : out.heldout_scores)
      if (s.hypothesis_id != best->hypothesis_id) second = std::min(second, s.mse);
    // Require a visible error separation, not merely a winner by numerical noise.
    out.heldout_separates = second > 0 && best->mse <= 0.75 * second;
  }
  return out;
}

std::string behavioral_identification_markdown(const BehavioralIdentificationReport& r) {
  std::ostringstream o;
  o << "# CoAgentics — Unknown-agent behavioral identification\n\n"
    << "The development phase selects an explanation using only black-box observations. "
    << "The explanation is then frozen before any held-out observations are evaluated.\n\n"
    << "Development status: **" << r.development_status << "**\n\n"
    << "Frozen explanation: **" << (r.explanation_frozen ? r.frozen_hypothesis : "NONE") << "**\n\n"
    << "Held-out trials: " << r.heldout_trials << "\n\n"
    << "| Candidate | Held-out MSE | Mean absolute error |\n|---|---:|---:|\n";
  for (const auto& s : r.heldout_scores)
    o << "| " << s.hypothesis_id << " | " << std::fixed << std::setprecision(5)
      << s.mse << " | " << s.mean_abs_error << " |\n";
  o << "\nHeld-out winner matches frozen explanation: **"
    << (r.heldout_winner_matches_frozen ? "YES" : "NO") << "**\n"
    << "\nHeld-out separation gate: **" << (r.heldout_separates ? "PASS" : "FAIL") << "**\n"
    << "\nInterpretation: this test evaluates predictive agreement with declared candidate response models. "
    << "It does not infer hidden beliefs, intent, or psychological traits. A non-separated result is valid evidence of non-identifiability.\n";
  return o.str();
}
}
