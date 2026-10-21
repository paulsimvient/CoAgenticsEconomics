#include "coagentics/experiment/Dv026MarketLayers.hpp"
#include "coagentics/experiment/Runner.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <memory>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
namespace coagentics::experiment {
namespace {
market::MarketConfig population_config(const MarketSpec& s){
 market::MarketConfig c;
 c.fundamental_value[s.asset]=s.fundamental;
 c.total_supply[s.asset]=s.sellers;
 for(int i=0;i<s.buyers;++i){
  const std::string id="B"+std::to_string(i);
  c.private_values.buy_values[id+"\n"+s.asset]={s.fundamental+s.value_step*(s.buyers-i)};
 }
 for(int i=0;i<s.sellers;++i){
  const std::string id="S"+std::to_string(i);
  c.private_values.sell_costs[id+"\n"+s.asset]={s.fundamental-s.value_step*(s.sellers-i)};
 }
 return c;
}

market::Metrics run_cda_reference(const MarketSpec& s, std::uint64_t seed){
 ReferenceAuctionConfig cfg;
 cfg.buyers=s.buyers; cfg.sellers=s.sellers; cfg.rounds=s.periods;
 cfg.fundamental=s.fundamental; cfg.value_step=s.value_step; cfg.starting_cash=s.starting_cash;
 // Programmed heuristic reference agents (not LLMs).
 return run_reference_auction(seed, cfg, {}, true).metrics;
}

market::Metrics run_sealed_reference(const MarketSpec& s, std::uint64_t seed, market::UtilitySummary* util_out, std::size_t* trades_out){
 auto cfg=population_config(s);
 auto mech=market::make_mechanism(market::MechanismKind::SealedBidDoubleAuction, cfg);
 std::vector<std::unique_ptr<agents::AgentPolicy>> agents;
 for(int i=0;i<s.buyers;++i){
  const std::string id="B"+std::to_string(i);
  const double v=cfg.private_values.buy_values[id+"\n"+s.asset][0];
  mech->add_account(id,{s.starting_cash,{}});
  agents.push_back(std::make_unique<agents::HeuristicAgent>(id,v,market::Side::Buy,1,0.02));
 }
 for(int i=0;i<s.sellers;++i){
  const std::string id="S"+std::to_string(i);
  const double cost=cfg.private_values.sell_costs[id+"\n"+s.asset][0];
  mech->add_account(id,{0,{{s.asset,1}}});
  agents.push_back(std::make_unique<agents::HeuristicAgent>(id,cost,market::Side::Sell,1,0.02));
 }
 std::mt19937_64 rng(seed);
 double last=0;
 for(int p=0;p<s.periods;++p){
  std::vector<int> order(agents.size());
  std::iota(order.begin(),order.end(),0);
  std::shuffle(order.begin(),order.end(),rng);
  for(int j:order){
   // Sealed: no contemporaneous book; authorized history = last clearing price only.
   agents::Observation o{static_cast<std::uint64_t>(p), s.asset, s.fundamental, 0.0, 0.0, 0.0, last};
   auto bid=agents[static_cast<size_t>(j)]->act(o);
   mech->submit(bid);
  }
  mech->clear(static_cast<std::uint64_t>(p));
  if(!mech->trades().empty()) last=mech->trades().back().price;
 }
 if(util_out) *util_out=mech->utility();
 if(trades_out) *trades_out=mech->trades().size();
 return mech->metrics();
}

double mean_price(const std::vector<market::Trade>& ts){
 if(ts.empty()) return 0;
 double s=0; for(auto& t:ts) s+=t.price; return s/ts.size();
}
}

MarketQualificationReport run_market_qualification(const MarketSpec& population,
 std::uint64_t first_seed, std::size_t n_trials){
 if(n_trials<1) throw std::invalid_argument("n_trials must be positive");
 MarketQualificationReport rep; rep.spec_template=population;
 double sum_cda=0, sum_sealed=0;
 for(std::size_t i=0;i<n_trials;++i){
  const std::uint64_t seed=first_seed+i;
  MarketQualificationTrial cda;
  cda.seed=seed; cda.mechanism=market::MechanismKind::ContinuousDoubleAuction;
  cda.metrics=run_cda_reference(population, seed);
  cda.trades=0; // reference auction does not expose trade count in Metrics; efficiency is the gate
  sum_cda+=cda.metrics.allocative_efficiency;
  rep.trials.push_back(cda);

  MarketQualificationTrial sealed;
  sealed.seed=seed; sealed.mechanism=market::MechanismKind::SealedBidDoubleAuction;
  sealed.metrics=run_sealed_reference(population, seed, &sealed.utility, &sealed.trades);
  sum_sealed+=sealed.metrics.allocative_efficiency;
  rep.trials.push_back(sealed);
 }
 rep.mean_efficiency_cda=sum_cda/n_trials;
 rep.mean_efficiency_sealed=sum_sealed/n_trials;
 auto all_above_gate=[&](market::MechanismKind kind){
  bool saw=false;
  for(const auto& t:rep.trials){
   if(t.mechanism!=kind) continue;
   saw=true;
   if(!(t.metrics.allocative_efficiency>population.efficiency_gate)) return false;
  }
  return saw;
 };
 rep.cda_qualified=all_above_gate(market::MechanismKind::ContinuousDoubleAuction);
 rep.sealed_qualified=all_above_gate(market::MechanismKind::SealedBidDoubleAuction);
 rep.layer_a_pass=rep.cda_qualified && rep.sealed_qualified;

 analysis::EvidenceEnvelope env;
 env.experiment_id="dv026-layer-a-market-qualification";
 env.control_run_id="programmed-reference";
 env.treatment_run_id="n/a";
 env.seed=first_seed;
 env.evidence.hypothesis_id="H_MARKET_ENVIRONMENT_QUALIFIED";
 env.evidence.discriminator_id="D_ALLOCATIVE_EFFICIENCY_GATE";
 env.evidence.direction=rep.layer_a_pass?analysis::EvidenceDirection::Supports:analysis::EvidenceDirection::Challenges;
 env.evidence.rationale=
  "Layer A only: programmed reference agents on CDA and sealed-bid. "
  "mean_cda="+std::to_string(rep.mean_efficiency_cda)+
  " mean_sealed="+std::to_string(rep.mean_efficiency_sealed)+
  " gate="+std::to_string(population.efficiency_gate)+
  " strict_rule=every_trial>gate trials="+std::to_string(n_trials)+
  ". Not an LLM claim.";
 rep.evidence.append(std::move(env));
 return rep;
}

PairedLlmExperimentResult run_paired_programmed_buyer_vs_llm(const ExperimentSpec& economics,
 const RunSpec& treatment_run,
 double control_buyer_limit_price){
 if(!treatment_run.transport) throw std::invalid_argument("treatment transport required");
 PairedLlmExperimentResult out;
 out.seed=treatment_run.seed;
 out.market.mechanism=market::MechanismKind::ContinuousDoubleAuction;
 out.market.asset=economics.asset;
 out.market.fundamental=economics.fundamental;

 // --- CONTROL: programmed buyer limit + deterministic seller (no LLM) ---
 market::MarketConfig mc;
 mc.fundamental_value[economics.asset]=economics.fundamental;
 mc.total_supply[economics.asset]=economics.counterparty_quantity;
 mc.private_values.buy_values[treatment_run.llm_agent_id+"\n"+economics.asset]={economics.llm_private_value};
 mc.private_values.sell_costs[treatment_run.seller_agent_id+"\n"+economics.asset]={economics.seller_cost};
 market::Market control(mc);
 control.add_account(treatment_run.llm_agent_id,{economics.starting_cash,{}});
 control.add_account(treatment_run.seller_agent_id,{0,{{economics.asset, economics.counterparty_quantity}}});
 for(int r=0;r<economics.rounds;++r){
  const std::uint64_t t=static_cast<std::uint64_t>(r);
  if(economics.deterministic_counterparty){
   control.submit({t, treatment_run.seller_agent_id, economics.asset, economics.counterparty_quantity,
    economics.counterparty_limit_price, market::Side::Sell});
  }
  control.submit({t, treatment_run.llm_agent_id, economics.asset, 1, control_buyer_limit_price, market::Side::Buy});
 }
 out.control_metrics=control.metrics();
 out.control_trades=control.trades().size();
 out.control_mean_price=mean_price(control.trades());

 // --- TREATMENT: Wave-1 LLM path (identical economics / seed) ---
 out.treatment=run_llm_market_experiment(economics, treatment_run);
 out.deltas.delta_efficiency=out.treatment.metrics.allocative_efficiency-out.control_metrics.allocative_efficiency;
 out.deltas.delta_surplus=out.treatment.metrics.realized_surplus-out.control_metrics.realized_surplus;
 out.deltas.delta_trades=static_cast<int>(out.treatment.trades.size())-static_cast<int>(out.control_trades);
 out.deltas.delta_mean_price=mean_price(out.treatment.trades)-out.control_mean_price;

 analysis::EvidenceEnvelope env;
 env.experiment_id="dv026-layer-b-paired-zi-vs-llm";
 env.control_run_id="programmed-buyer-limit";
 env.treatment_run_id=out.treatment.run_id;
 env.seed=treatment_run.seed;
 env.evidence.hypothesis_id="H_PAIRED_OBSERVABLE_DELTA";
 env.evidence.discriminator_id="D_SHARED_SEED_MARKET";
 env.evidence.direction=analysis::EvidenceDirection::Inconclusive; // deltas recorded; no performance claim
 env.evidence.rationale=
  "Layer B paired run under shared seed/config. "
  "delta_efficiency_pp="+std::to_string(out.deltas.delta_efficiency)+
  " delta_surplus="+std::to_string(out.deltas.delta_surplus)+
  " delta_trades="+std::to_string(out.deltas.delta_trades)+
  ". Observable outcome comparison only; not rationality/bias/deception.";
 out.evidence.append(std::move(env));
 return out;
}

std::string market_qualification_json(const MarketQualificationReport& r){
 std::ostringstream o;
 o<<std::fixed<<std::setprecision(6)
  <<"{\"layer\":\"A\",\"mean_efficiency_cda\":"<<r.mean_efficiency_cda
  <<",\"mean_efficiency_sealed\":"<<r.mean_efficiency_sealed
  <<",\"efficiency_gate\":"<<r.spec_template.efficiency_gate
  <<",\"cda_qualified\":"<<(r.cda_qualified?"true":"false")
  <<",\"sealed_qualified\":"<<(r.sealed_qualified?"true":"false")
  <<",\"layer_a_pass\":"<<(r.layer_a_pass?"true":"false")
  <<",\"trials\":"<<r.trials.size()
  <<",\"claim_boundary\":\""<<r.claim_boundary<<"\"}";
 return o.str();
}

std::string paired_llm_json(const PairedLlmExperimentResult& r){
 std::ostringstream o;
 o<<std::fixed<<std::setprecision(6)
  <<"{\"layer\":\"B\",\"seed\":"<<r.seed
  <<",\"control_efficiency\":"<<r.control_metrics.allocative_efficiency
  <<",\"treatment_efficiency\":"<<r.treatment.metrics.allocative_efficiency
  <<",\"delta_efficiency\":"<<r.deltas.delta_efficiency
  <<",\"delta_surplus\":"<<r.deltas.delta_surplus
  <<",\"delta_trades\":"<<r.deltas.delta_trades
  <<",\"delta_mean_price\":"<<r.deltas.delta_mean_price
  <<",\"claim_boundary\":\""<<r.claim_boundary<<"\"}";
 return o.str();
}
}
