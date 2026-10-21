#include "coagentics/experiment/HeterogeneousCampaign.hpp"
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <unistd.h>
using namespace coagentics::experiment;
using coagentics::market::Side;

int main() {
  unsetenv("COAGENTICS_RESUME_RUN_DIR");
  unsetenv("COAGENTICS_INCREMENTAL_EVIDENCE");
  HeterogeneousCampaignSpec s;
  s.smoke = false;
  s.use_live_ollama = false;
  s.require_minimum_distinct_models = false;
  s.minimum_distinct_models = 2;
  s.max_llm_seats = 10;
  s.n_seeds = 1;
  s.inference_repeats = 1;
  s.rounds = 2;
  s.run_counterfactual_substitution = false;
  auto dir = std::filesystem::temp_directory_path() / ("hetero_five_five_mock_" + std::to_string(getpid()));
  s.results_dir = dir.string();
  auto r = run_heterogeneous_campaign(s);
  assert(r.cells.size() == 2);
  assert(r.distinct_models_used == 10);
  for (const auto& cell : r.cells) {
    assert(cell.assignment.size() == 10);
    int buy = 0, sell = 0;
    for (const auto& a : cell.assignment) {
      assert(a.model.provider == "mock");
      if (a.side == Side::Buy) ++buy;
      else ++sell;
    }
    assert(buy == 5 && sell == 5);
    assert(!cell.population.trades.empty());
    for (const auto& t : cell.population.trades) {
      bool buyer_ok = false, seller_ok = false;
      for (const auto& a : cell.assignment) {
        if (a.agent_id == t.buyer && a.side == Side::Buy) buyer_ok = true;
        if (a.agent_id == t.seller && a.side == Side::Sell) seller_ok = true;
      }
      assert(buyer_ok && seller_ok);
    }
    for (const auto& a : cell.population.agents) {
      assert(a.model.provider == "mock");
      assert(!a.llm_turns.empty());
    }
  }
  auto obs = dir / "cell_observations" / "cell_0.json";
  assert(std::filesystem::exists(obs));
  std::cout << "heterogeneous_five_five_mock PASS MOCK_ONLY cells=" << r.cells.size()
            << " models=" << r.distinct_models_used << " dir=" << dir << "\n";
}
