#include "coagentics/experiment/Dv026Wave3.hpp"
#include "coagentics/analysis/Behavior.hpp"
#include "coagentics/market/Mechanism.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <iomanip>
#include <map>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
namespace coagentics::experiment {
namespace {
double inv_of(const market::Account& a, const std::string& asset){
 auto it=a.inventory.find(asset); return it==a.inventory.end()?0:it->second;
}
AgentRole role_of(market::Side s){ return s==market::Side::Buy?AgentRole::Buyer:AgentRole::Seller; }

MarketState market_state_from_mechanism(const market::MarketMechanism& mech, std::uint64_t time,
 const std::string& asset, double fundamental, double public_signal){
 MarketState s; s.time=time; s.asset=asset; s.fundamental=fundamental; s.public_signal=public_signal;
 s.best_bid=mech.best_bid_opt(asset);
 s.best_ask=mech.best_ask_opt(asset);
 if(!mech.trades().empty()){
  for(auto it=mech.trades().rbegin(); it!=mech.trades().rend(); ++it){
   if(it->asset==asset){ s.last_trade=it->price; break; }
  }
 }
 return s;
}
}

std::vector<agents::ModelIdentity> dv026_distinct_llm_catalog(){
 return {
  {"openai","gpt-4o","2024-08-06","chat"},
  {"openai","gpt-4o-mini","2024-07-18","chat"},
  {"openai","gpt-4.1","2025-04-14","chat"},
  {"anthropic","claude-3-5-sonnet","20241022","chat"},
  {"anthropic","claude-3-5-haiku","20241022","chat"},
  {"anthropic","claude-3-opus","20240229","chat"},
  {"google","gemini-1.5-pro","002","chat"},
  {"google","gemini-1.5-flash","002","chat"},
  {"meta","llama-3.1-70b-instruct","202407","open-weight"},
  {"mistral","mistral-large","2407","chat"},
 };
}

PopulationRunResult run_population_market(const PopulationSpec& population){
 if(population.agents.empty()) throw std::invalid_argument("PopulationSpec requires agents");

 PopulationRunResult out; out.spec=population;
 market::MarketConfig cfg;
 cfg.fundamental_value[population.market.asset]=population.market.fundamental;
 int supply=0;
 for(const auto& slot:population.agents){
  const std::string key=slot.agent_id+"\n"+population.market.asset;
  if(slot.side==market::Side::Buy) cfg.private_values.buy_values[key]={slot.private_value_or_cost};
  else { cfg.private_values.sell_costs[key]={slot.private_value_or_cost}; supply+=std::max(1, slot.inventory); }
 }
 cfg.total_supply[population.market.asset]=std::max(1, supply);

 auto mech=market::make_mechanism(population.market.mechanism, cfg);
 std::map<std::string,std::unique_ptr<agents::AgentPolicy>> programmed;
 std::map<std::string,std::unique_ptr<LlmAgentAdapter>> llm_adapters;
 std::map<std::string, PopulationAgentOutcome> outcomes;
 // Per-LLM state transition (mirrors single-agent run_llm_market_experiment).
 struct LlmTrans {
  std::optional<MarketAction> prev_action;
  std::optional<FillSummary> prev_fill;
  double payoff{0};
  int remaining{1};
 };
 std::map<std::string, LlmTrans> llm_trans;

 for(const auto& slot:population.agents){
  market::Account acct{slot.cash,{}};
  if(slot.side==market::Side::Sell) acct.inventory[population.market.asset]=std::max(1, slot.inventory);
  mech->add_account(slot.agent_id, acct);
  PopulationAgentOutcome oa; oa.agent_id=slot.agent_id; oa.kind=slot.kind; oa.model=slot.model;
  outcomes[slot.agent_id]=oa;
  if(slot.kind==AgentKind::Llm){
   if(!slot.transport) throw std::invalid_argument("LLM slot requires transport: "+slot.agent_id);
   llm_adapters[slot.agent_id]=std::make_unique<LlmAgentAdapter>(
    slot.model, slot.transport, population.seed, "coagentics-llm-adapter/0.1", "",
    "pop:"+std::to_string(population.seed)+":"+slot.agent_id);
   llm_trans[slot.agent_id]=LlmTrans{};
   if(slot.side==market::Side::Sell) llm_trans[slot.agent_id].remaining=std::max(1, slot.inventory);
  }else if(slot.kind==AgentKind::ProgrammedZI){
   programmed[slot.agent_id]=std::make_unique<agents::ZeroIntelligenceAgent>(
    slot.agent_id, slot.private_value_or_cost, slot.side, population.seed+std::hash<std::string>{}(slot.agent_id));
  }else{
   programmed[slot.agent_id]=std::make_unique<agents::HeuristicAgent>(
    slot.agent_id, slot.private_value_or_cost, slot.side, 1, 0.03);
  }
 }

 std::mt19937_64 rng(population.seed);
 std::vector<MarketHistoryEntry> shared_history;
 for(int r=0;r<population.rounds;++r){
  std::vector<std::string> order;
  for(const auto& slot:population.agents) order.push_back(slot.agent_id);
  std::shuffle(order.begin(), order.end(), rng);
  for(const auto& id:order){
   const AgentSlot* slot=nullptr;
   for(const auto& s:population.agents) if(s.agent_id==id){ slot=&s; break; }
   if(!slot) continue;
   const auto& acct=mech->accounts().at(id);
   if(slot->side==market::Side::Buy && inv_of(acct, population.market.asset)>0) continue;
   if(slot->side==market::Side::Sell && inv_of(acct, population.market.asset)<=0) continue;

   auto mstate=market_state_from_mechanism(*mech, static_cast<std::uint64_t>(r), population.market.asset,
    population.market.fundamental, 0.0);
   if(slot->kind==AgentKind::Llm){
    auto& tr=llm_trans[id];
    const AgentRole role=role_of(slot->side);
    auto astate=build_agent_state_from_accounts(mech->accounts(), id, population.market.asset,
     slot->private_value_or_cost, tr.remaining, role, tr.prev_action, tr.prev_fill, tr.payoff);
    DecisionContext ctx;
    ctx.market=mstate;
    ctx.agent=astate;
    ctx.information=slot->information;
    ctx.information.information_condition=slot->information_condition;
    ctx.information.history=shared_history;
    ctx.information.constraints=ActionConstraints::for_role(role);
    AgentTurnRecord turn=llm_adapters[id]->decide(ctx);
    ++outcomes[id].turns;
    if(!turn.parse.success) ++outcomes[id].parse_failures;
    else if(!turn.action_validation.valid) ++outcomes[id].action_validation_failures;
    else if(turn.parsed_action && turn.parsed_action->action!=ActionType::Hold){
     auto bid=to_bid(*turn.parsed_action, id);
     if(bid){
      auto sub=mech->submit_detailed(*bid);
      turn.submission=sub;
      if(sub.accepted){
       out.bids.push_back(*bid);
       outcomes[id].fills+=static_cast<std::size_t>(sub.filled_quantity);
       if(sub.filled_quantity>0 && sub.average_fill_price){
        if(role==AgentRole::Buyer)
         tr.payoff += (slot->private_value_or_cost - *sub.average_fill_price) * sub.filled_quantity;
        else
         tr.payoff += (*sub.average_fill_price - slot->private_value_or_cost) * sub.filled_quantity;
        tr.remaining=std::max(0, tr.remaining - sub.filled_quantity);
        tr.prev_fill=FillSummary{sub.filled_quantity, sub.average_fill_price, sub.trades};
       }
      }else{
       ++outcomes[id].market_rejections;
      }
     }
    }else if(turn.held){
     turn.submission.accepted=false;
     turn.submission.rejection_reason="hold_no_submission";
    }
    tr.prev_action=turn.parsed_action;
    turn.agent_state_after=build_agent_state_from_accounts(mech->accounts(), id, population.market.asset,
     slot->private_value_or_cost, tr.remaining, role, tr.prev_action, tr.prev_fill, tr.payoff);
    llm_adapters[id]->persist_turn(turn);
    outcomes[id].llm_turns.push_back(turn);
   }else{
    auto bid=programmed[id]->act(to_public_observation(mstate));
    auto sub=mech->submit_detailed(bid);
    if(sub.accepted) out.bids.push_back(bid);
   }
  }
  if(mech->kind()==market::MechanismKind::SealedBidDoubleAuction){
   mech->clear(static_cast<std::uint64_t>(r));
  }
  MarketHistoryEntry he;
  he.time=static_cast<std::uint64_t>(r);
  he.best_bid=mech->best_bid_opt(population.market.asset);
  he.best_ask=mech->best_ask_opt(population.market.asset);
  if(!mech->trades().empty()) he.last_trade=mech->trades().back().price;
  shared_history.push_back(he);
 }

 out.trades=mech->trades();
 out.metrics=mech->metrics();
 for(auto& [_, oa]:outcomes) out.agents.push_back(oa);

 // SYNTHETIC WIRING ONLY: self-paired bids prove classifier plumbing, not behavior.
 out.classifier_mode="synthetic_wiring";
 out.classifier_synthetic=true;
 out.features={0,0,0, out.metrics.allocative_efficiency};
 auto prop=analysis::compare_paired_bids(out.bids, out.bids, 0, {});
 out.features.information_sensitivity=prop.targeted_mean_abs_shift;
 out.features.peer_sensitivity=prop.non_target_mean_abs_shift;
 out.features.persistence=prop.propagation_ratio;
 analysis::MechanismClassifier clf;
 out.classification=clf.classify(out.features);
 out.claim_boundary=
  "Wave 3 population run: observable market outcomes. "
  "Classifier label from synthetic self-paired bids (wiring only) — not a behavioral experiment. "
  "Use run_population_behavioral_contrast / run_operational_classifier for control/treatment. "
  "No Phase I classifier performance metrics; no Phase I classifier accuracy; no Phase II constructs.";

 auto refs=analysis::empirical_market_reference_catalog();
 if(refs.has("allocative_efficiency"))
  out.human_reference.push_back(refs.compare("allocative_efficiency", out.metrics.allocative_efficiency));

 // Provenance-gated AI–human comparison path (literature refs only; LiveProvider when LLM seats present).
 std::vector<analysis::ModelObservation> obs;
 const bool any_llm=std::any_of(population.agents.begin(), population.agents.end(),
  [](const AgentSlot& s){ return s.kind==AgentKind::Llm; });
 if(refs.has("allocative_efficiency")){
  const auto& ref=refs.get("allocative_efficiency");
  analysis::ModelObservation mo;
  mo.metric="allocative_efficiency";
  mo.condition=ref.condition;
  mo.model_id=any_llm?"population-llm":"population-programmed";
  mo.run_id="pop:"+std::to_string(population.seed);
  mo.value=out.metrics.allocative_efficiency;
  mo.origin=any_llm?analysis::EvidenceOrigin::LiveProvider:analysis::EvidenceOrigin::ScriptedControl;
  obs.push_back(mo);
 }
 out.human_comparison=analysis::compare_human_behavior(refs, obs);

 analysis::EvidenceEnvelope env;
 env.experiment_id="dv026-wave3-population";
 env.seed=population.seed;
 env.treatment_run_id="population:"+std::to_string(population.agents.size());
 env.evidence.hypothesis_id="H_MULTIAGENT_OBSERVABLE_RUN";
 env.evidence.discriminator_id="D_OPERATIONAL_CLASSIFIER_SYNTHETIC";
 env.evidence.direction=analysis::EvidenceDirection::Inconclusive;
 env.evidence.rationale=
  "Population run with "+std::to_string(population.agents.size())+" agents; classifier_mode=synthetic_wiring; "
  "label="+out.classification.label+"; efficiency="+std::to_string(out.metrics.allocative_efficiency)+
  "; mechanism="+(population.market.mechanism==market::MechanismKind::SealedBidDoubleAuction?"sealed":"cda")+
  ". Operational demonstration only.";
 out.evidence.append(std::move(env));
 return out;
}

PopulationBehavioralContrastReport run_population_behavioral_contrast(
 const PopulationSpec& control,
 const PopulationSpec& treatment,
 std::uint64_t intervention_time,
 const std::vector<std::string>& targeted_agents){
 PopulationBehavioralContrastReport rep;
 rep.control=run_population_market(control);
 rep.treatment=run_population_market(treatment);
 rep.classifier=run_operational_classifier(rep.control.bids, rep.treatment.bids,
  intervention_time, targeted_agents);
 rep.treatment.classifier_mode="control_treatment";
 rep.treatment.classifier_synthetic=false;
 rep.treatment.classification=rep.classifier.classification;
 rep.treatment.features=rep.classifier.features;
 return rep;
}

namespace {
PopulationSpec base_info_pop(std::uint64_t seed, bool treatment){
 PopulationSpec pop; pop.seed=seed; pop.rounds=1;
 pop.market.asset="ASSET"; pop.market.fundamental=100;
 AgentSlot buyer_llm;
 buyer_llm.agent_id="LLM-B0";
 buyer_llm.kind=AgentKind::Llm;
 buyer_llm.side=market::Side::Buy;
 buyer_llm.private_value_or_cost=120;
 buyer_llm.cash=10000;
 buyer_llm.model={"mock","info-contrast","v1"};
 buyer_llm.transport=std::make_shared<RawJsonTransport>(std::vector<std::string>{
  R"({"action":"BUY","asset":"ASSET","quantity":1,"price":95,"time":0})"
 });
 buyer_llm.information_condition=treatment?"public_book+news+peer":"public_book";
 if(treatment){
  buyer_llm.information.news.present=true;
  buyer_llm.information.news.headline="public demand shock";
  buyer_llm.information.news.signal=1.0;
  buyer_llm.information.news.reliability=0.8;
  buyer_llm.information.peer.peer_visibility=true;
  buyer_llm.information.peer.peer_mid_quote=92.0;
  buyer_llm.information.information_condition="public_book+news+peer";
 }
 AgentSlot seller;
 seller.agent_id="S0";
 seller.kind=AgentKind::ProgrammedHeuristic;
 seller.side=market::Side::Sell;
 seller.private_value_or_cost=80;
 seller.inventory=1;
 seller.cash=0;
 pop.agents={buyer_llm, seller};
 return pop;
}
}

InformationContrastReport run_information_contrast_experiment(std::uint64_t seed){
 InformationContrastReport out;
 auto control=base_info_pop(seed, false);
 auto treatment=base_info_pop(seed, true);
 // Distinct treatment bid so classifier sees a real contrast (scripted BUY vs slightly higher BUY).
 treatment.agents[0].transport=std::make_shared<RawJsonTransport>(std::vector<std::string>{
  R"({"action":"BUY","asset":"ASSET","quantity":1,"price":105,"time":0})"
 });
 out.contrast=run_population_behavioral_contrast(control, treatment, 0, {"LLM-B0"});
 return out;
}

TenLlmQualificationReport run_ten_llm_interface_qualification(
 const std::vector<agents::ModelIdentity>& catalog, std::uint64_t seed){
 TenLlmQualificationReport rep;
 std::map<std::string,int> seen;
 for(const auto& model:catalog){
  const std::string key=model.provider+"|"+model.model+"|"+model.version;
  seen[key]++;
  TenLlmCell cell; cell.model=model;
  agents::LlmAction action{0,"ASSET",1,95.0,market::Side::Buy,false};
  auto transport=std::make_shared<agents::ScriptedTransport>(std::vector<agents::LlmAction>{action});
  LlmAgentAdapter adapter(model, transport, seed, "coagentics-llm-adapter/0.1");
  DecisionContext ctx;
  ctx.market={0,"ASSET",100,0,std::nullopt,90.0,std::nullopt};
  ctx.agent={"LLM-seat",AgentRole::Buyer,10000,0,120,1,{},{},0};
  ctx.information.constraints=ActionConstraints::for_role(AgentRole::Buyer);
  auto turn=adapter.decide(ctx);
  cell.parse_ok=turn.parse.success;
  cell.action_valid=turn.action_validation.valid;
  cell.request_id=turn.request_id;
  cell.provenance_ok=!turn.request_id.empty() && !turn.canonical_request.empty()
   && turn.model.provider==model.provider && turn.model.model==model.model
   && turn.model.version==model.version
   && turn.canonical_request.find("\"role\":\"BUYER\"")!=std::string::npos;
  if(!cell.parse_ok) cell.error=turn.parse.error;
  else if(!cell.action_valid && !turn.action_validation.errors.empty())
   cell.error=turn.action_validation.errors.front();
  rep.cells.push_back(cell);
 }
 rep.distinct_models=seen.size();
 std::set<std::string> ok_distinct;
 for(const auto& c:rep.cells){
  if(c.parse_ok && c.action_valid && c.provenance_ok){
   ok_distinct.insert(c.model.provider+"|"+c.model.model+"|"+c.model.version);
  }
 }
 rep.qualifies=(ok_distinct.size()>=10);

 analysis::EvidenceEnvelope env;
 env.experiment_id="dv026-wave3-ten-llm-interface";
 env.seed=seed;
 env.evidence.hypothesis_id="H_TEN_LLM_INTERFACE";
 env.evidence.discriminator_id="D_DISTINCT_MODEL_PROVENANCE";
 env.evidence.direction=rep.qualifies?analysis::EvidenceDirection::Supports:analysis::EvidenceDirection::Challenges;
 env.evidence.rationale=
  "distinct_models="+std::to_string(rep.distinct_models)+
  " distinct_interface_ok="+std::to_string(ok_distinct.size())+
  ". Interface/provenance only; not economic performance.";
 rep.evidence.append(std::move(env));
 return rep;
}

OperationalClassifierReport run_operational_classifier(
 const std::vector<market::Bid>& control_bids,
 const std::vector<market::Bid>& treatment_bids,
 std::uint64_t intervention_time,
 const std::vector<std::string>& targeted_agents){
 OperationalClassifierReport rep;
 auto prop=analysis::compare_paired_bids(control_bids, treatment_bids, intervention_time, targeted_agents);
 rep.features.information_sensitivity=prop.targeted_mean_abs_shift;
 rep.features.peer_sensitivity=prop.non_target_mean_abs_shift;
 rep.features.persistence=prop.propagation_ratio;
 rep.features.efficiency_delta=0;
 analysis::MechanismClassifier clf;
 rep.classification=clf.classify(rep.features);
 return rep;
}

std::string ten_llm_json(const TenLlmQualificationReport& r){
 std::ostringstream o;
 o<<"{\"distinct_models\":"<<r.distinct_models<<",\"cells\":"<<r.cells.size()
  <<",\"qualifies\":"<<(r.qualifies?"true":"false")<<"}";
 return o.str();
}

std::string population_run_json(const PopulationRunResult& r){
 std::ostringstream o;
 o<<std::fixed<<std::setprecision(4)
  <<"{\"agents\":"<<r.agents.size()<<",\"trades\":"<<r.trades.size()
  <<",\"efficiency\":"<<r.metrics.allocative_efficiency
  <<",\"classifier\":\""<<r.classification.label<<"\""
  <<",\"classifier_mode\":\""<<r.classifier_mode<<"\""
  <<",\"classifier_synthetic\":"<<(r.classifier_synthetic?"true":"false")<<"}";
 return o.str();
}
}
