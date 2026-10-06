#include <cstdlib>
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
  const bool frozen_snapshot=population.activation_design==ActivationDesign::FrozenSnapshot;
  const auto round_start_accounts=mech->accounts();
  const auto round_start_market=market_state_from_mechanism(*mech, static_cast<std::uint64_t>(r), population.market.asset,
   population.market.fundamental, 0.0);
  std::vector<std::string> order;
  if(!population.activation_order.empty()) order=population.activation_order;
  else {
   for(const auto& slot:population.agents) order.push_back(slot.agent_id);
   std::shuffle(order.begin(), order.end(), rng);
  }
  for(const auto& id:order){
   const AgentSlot* slot=nullptr;
   for(const auto& s:population.agents) if(s.agent_id==id){ slot=&s; break; }
   if(!slot) continue;
   const auto& acct=(frozen_snapshot?round_start_accounts:mech->accounts()).at(id);
   if(slot->side==market::Side::Buy && inv_of(acct, population.market.asset)>0) continue;
   if(slot->side==market::Side::Sell && inv_of(acct, population.market.asset)<=0) continue;

   auto mstate=frozen_snapshot?round_start_market:market_state_from_mechanism(*mech, static_cast<std::uint64_t>(r), population.market.asset,
    population.market.fundamental, 0.0);
   if(slot->kind==AgentKind::Llm){
    auto& tr=llm_trans[id];
    const AgentRole role=role_of(slot->side);
    auto astate=build_agent_state_from_accounts(frozen_snapshot?round_start_accounts:mech->accounts(), id, population.market.asset,
     slot->private_value_or_cost, tr.remaining, role, tr.prev_action, tr.prev_fill, tr.payoff);
    DecisionContext ctx;
    ctx.market=mstate;
    ctx.agent=astate;
    ctx.information=slot->information;
    ctx.information.information_condition=slot->information_condition;
    ctx.information.history = slot->information.history_visible ? shared_history : std::vector<MarketHistoryEntry>{};
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
 for(auto& [id, oa]:outcomes){
  if(oa.kind==AgentKind::Llm){
   const AgentSlot* slot=nullptr; for(const auto& s:population.agents) if(s.agent_id==id){slot=&s;break;}
   if(slot){
    const std::string mech_name=population.market.mechanism==market::MechanismKind::SealedBidDoubleAuction?"sealed_bid":"cda";
    oa.fingerprint=analysis::fingerprint_observed_llm(id,mech_name,slot->private_value_or_cost,role_of(slot->side),oa.llm_turns);
    // Settlement-aware metrics: a resting order may fill on another agent's later turn.
    std::size_t settled_trades=0; double settled_payoff=0;
    for(const auto& tr:out.trades){
     if(tr.buyer==id){ ++settled_trades; settled_payoff+=(slot->private_value_or_cost-tr.price)*tr.quantity; }
     if(tr.seller==id){ ++settled_trades; settled_payoff+=(tr.price-slot->private_value_or_cost)*tr.quantity; }
    }
    oa.fills=settled_trades;
    oa.fingerprint.trade_frequency=oa.turns?double(settled_trades)/oa.turns:0;
    const double opp=std::abs(slot->private_value_or_cost-population.market.fundamental)*std::max<std::size_t>(1,settled_trades);
    oa.fingerprint.surplus_capture=opp>1e-9?std::clamp(settled_payoff/opp,0.0,1.0):0;
    oa.fingerprint_available=true;
   }
  }
  out.agents.push_back(oa);
 }

 // A standalone population run has no behavioral contrast, so it must not emit a classifier result.
 // Classification is reserved for paired observed control/treatment behavior below.
 out.classifier_mode="none";
 out.classifier_available=false;
 out.classifier_synthetic=false;
 out.features={};
 out.classification={};
 out.claim_boundary=
  "Wave 3 standalone population run: observable market outcomes only; no classifier result without a paired behavioral contrast. "
  "Use run_population_behavioral_contrast / run_operational_classifier on observed control/treatment bids. "
  "No Phase I classifier performance metrics; no Phase I classifier accuracy; no Phase II constructs.";

 auto refs=analysis::empirical_market_reference_catalog();
 if(refs.has("allocative_efficiency"))
  out.human_reference.push_back(refs.compare("allocative_efficiency", out.metrics.allocative_efficiency));

 // Provenance-gated AI–human comparison path (literature refs only; LiveProvider when LLM seats present).
 std::vector<analysis::ModelObservation> obs;
 const bool any_llm=std::any_of(population.agents.begin(), population.agents.end(),
  [](const AgentSlot& s){ return s.kind==AgentKind::Llm; });
 const bool all_llm=!population.agents.empty() && std::all_of(population.agents.begin(), population.agents.end(),
  [](const AgentSlot& s){ return s.kind==AgentKind::Llm; });
 bool all_live_transport=all_llm;
 if(all_live_transport){
  for(const auto& oa:outcomes){
   if(oa.second.kind!=AgentKind::Llm){ all_live_transport=false; break; }
   for(const auto& turn:oa.second.llm_turns){
    if(turn.inference_config.find("transport=OpenAiCompatible")!=0){ all_live_transport=false; break; }
   }
  }
 }
 const auto origin=all_live_transport?analysis::EvidenceOrigin::LiveProvider:
  (any_llm?analysis::EvidenceOrigin::Unknown:analysis::EvidenceOrigin::ScriptedControl);
 const std::string model_id=all_live_transport?"heterogeneous-live-llm":(any_llm?"mixed-population":"population-programmed");
 const std::string run_id="pop:"+std::to_string(population.seed);
 if(refs.has("allocative_efficiency")){
  const auto& ref=refs.get("allocative_efficiency");
  analysis::ModelObservation mo;
  mo.metric="allocative_efficiency"; mo.condition=ref.condition; mo.model_id=model_id; mo.run_id=run_id;
  mo.value=out.metrics.allocative_efficiency; mo.origin=origin; obs.push_back(mo);
 }
 // Directional human-market benchmarks from Ikica et al. (2023). These are
 // deliberately separate from numeric human distributions. We only emit them
 // as live-model observations when every seat is a live LLM; mixed populations
 // cannot be interpreted as a pure human-vs-LLM behavioral comparison.
 if(all_live_transport){
  std::map<std::string,double> reservation;
  std::map<std::string,market::Side> side;
  for(const auto& slot:population.agents){ reservation[slot.agent_id]=slot.private_value_or_cost; side[slot.agent_id]=slot.side; }
  double bsum=0,ssum=0; int bn=0,sn=0;
  for(const auto& bid:out.bids){
   if(bid.time!=0) continue;
   auto rit=reservation.find(bid.agent_id); if(rit==reservation.end()||std::abs(rit->second)<1e-9) continue;
   double a=bid.side==market::Side::Buy ? bid.price/rit->second : 2.0-bid.price/rit->second;
   if(bid.side==market::Side::Buy){bsum+=a;++bn;} else {ssum+=a;++sn;}
  }
  if(bn>0 && sn>0){
   analysis::ModelObservation mo; mo.metric="initial_buyer_seller_aggressiveness"; mo.condition="private-information continuous double auctions";
   mo.model_id=model_id; mo.run_id=run_id; mo.value=(bsum/double(bn))-(ssum/double(sn)); mo.origin=origin; obs.push_back(mo);
  }
  if(!out.trades.empty()){
   const double eq=population.market.fundamental;
   auto min_time=out.trades.front().time, max_time=out.trades.back().time;
   double first_sum=0,last_sum=0; int first_n=0,last_n=0;
   for(const auto& tr:out.trades){
    if(tr.time==min_time){first_sum+=tr.price;++first_n;}
    if(tr.time==max_time){last_sum+=tr.price;++last_n;}
   }
   if(first_n>0){
    analysis::ModelObservation mo; mo.metric="initial_price_relative_to_equilibrium"; mo.condition="private-information continuous double auctions";
    mo.model_id=model_id; mo.run_id=run_id; mo.value=(first_sum/double(first_n))-eq; mo.origin=origin; obs.push_back(mo);
   }
   if(first_n>0 && last_n>0 && max_time>min_time){
    const double first_gap=std::abs((first_sum/double(first_n))-eq);
    const double last_gap=std::abs((last_sum/double(last_n))-eq);
    analysis::ModelObservation mo; mo.metric="price_convergence_over_periods"; mo.condition="private-information continuous double auctions";
    mo.model_id=model_id; mo.run_id=run_id; mo.value=first_gap-last_gap; mo.origin=origin; obs.push_back(mo);
   }
  }
 }
 out.human_comparison=analysis::compare_human_behavior(refs, obs);

 analysis::EvidenceEnvelope env;
 env.experiment_id="dv026-wave3-population";
 env.seed=population.seed;
 env.treatment_run_id="population:"+std::to_string(population.agents.size());
 env.evidence.hypothesis_id="H_MULTIAGENT_OBSERVABLE_RUN";
 env.evidence.discriminator_id="D_OBSERVABLE_POPULATION_OUTCOME";
 env.evidence.direction=analysis::EvidenceDirection::Inconclusive;
 env.evidence.rationale=
  "Population run with "+std::to_string(population.agents.size())+" agents; classifier_mode=none; "
  "efficiency="+std::to_string(out.metrics.allocative_efficiency)+
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
 // Complete the feature vector from observed market outcomes, not a fixture/default.
 rep.classifier.features.efficiency_delta = rep.treatment.metrics.allocative_efficiency - rep.control.metrics.allocative_efficiency;
 { analysis::MechanismClassifier clf; rep.classifier.classification=clf.classify(rep.classifier.features); }
 rep.treatment.classifier_mode="observed_control_treatment";
 rep.treatment.classifier_available=true;
 rep.treatment.classifier_synthetic=false;
 rep.treatment.classification=rep.classifier.classification;
 rep.treatment.features=rep.classifier.features;
 return rep;
}

namespace {
const char* treatment_name(InformationTreatment t){
 switch(t){
  case InformationTreatment::News: return "news";
  case InformationTreatment::MarketHistory: return "market_history";
  case InformationTreatment::PeerObservations: return "peer_observations";
 }
 return "unknown";
}
PopulationSpec base_info_pop(std::uint64_t seed, InformationTreatment dimension, bool treatment, bool live=false){
 PopulationSpec pop; pop.seed=seed; pop.rounds=(dimension==InformationTreatment::MarketHistory?2:1);
 pop.market.asset="ASSET"; pop.market.fundamental=100;
 AgentSlot buyer_llm;
 buyer_llm.agent_id="LLM-B0"; buyer_llm.kind=AgentKind::Llm; buyer_llm.side=market::Side::Buy;
 buyer_llm.private_value_or_cost=120; buyer_llm.cash=10000; buyer_llm.model={"mock","info-contrast","v2"};
 // Scripted transport is a deterministic qualification fixture. The treatment framework itself is transport-agnostic.
 std::vector<std::string> responses;
 if(dimension==InformationTreatment::MarketHistory) responses={
  R"({"action":"HOLD","asset":"ASSET","quantity":0,"price":0,"time":0})",
  treatment ? R"({"action":"BUY","asset":"ASSET","quantity":1,"price":105,"time":1})"
            : R"({"action":"BUY","asset":"ASSET","quantity":1,"price":95,"time":1})"};
 else responses={treatment ? R"({"action":"BUY","asset":"ASSET","quantity":1,"price":105,"time":0})"
                           : R"({"action":"BUY","asset":"ASSET","quantity":1,"price":95,"time":0})"};
 if(live){
  const char* model=std::getenv("COAGENTICS_LLM_MODEL");
  buyer_llm.model={"ollama",model&&*model?model:"llama3.2","local"};
  buyer_llm.transport=make_live_openai_compatible_transport(buyer_llm.model);
 }else buyer_llm.transport=std::make_shared<RawJsonTransport>(responses);
 buyer_llm.information.history_visible=false;
 buyer_llm.information_condition="public_book";
 if(treatment){
  if(dimension==InformationTreatment::News){
   buyer_llm.information.news.present=true; buyer_llm.information.news.headline="public demand shock";
   buyer_llm.information.news.signal=1.0; buyer_llm.information.news.reliability=0.8;
   buyer_llm.information_condition="public_book+news";
  }else if(dimension==InformationTreatment::PeerObservations){
   buyer_llm.information.peer.peer_visibility=true; buyer_llm.information.peer.visible_agent_ids={"PEER-PUBLIC"};
   buyer_llm.information.peer.peer_mid_quote=92.0; buyer_llm.information_condition="public_book+peer_observations";
  }else{
   buyer_llm.information.history_visible=true; buyer_llm.information_condition="public_book+market_history";
  }
 }
 AgentSlot seller; seller.agent_id="S0"; seller.kind=AgentKind::ProgrammedHeuristic; seller.side=market::Side::Sell;
 seller.private_value_or_cost=80; seller.inventory=1; seller.cash=0;
 pop.agents={buyer_llm,seller}; return pop;
}
}

static InformationContrastReport run_information_contrast_impl(InformationTreatment dimension, std::uint64_t seed, bool live){
 InformationContrastReport out; out.treatment=dimension; out.matched_seed=seed; out.manipulated_variable=treatment_name(dimension);
 auto control=base_info_pop(seed, dimension, false, live); auto treatment=base_info_pop(seed, dimension, true, live);
 out.control_condition="public_book"; out.treatment_condition=treatment.agents[0].information_condition;
 out.contrast=run_population_behavioral_contrast(control,treatment,dimension==InformationTreatment::MarketHistory?1:0,{"LLM-B0"});
 for(auto& a:out.contrast.treatment.agents) if(a.agent_id=="LLM-B0" && a.fingerprint_available){
  if(dimension==InformationTreatment::PeerObservations) a.fingerprint.peer_sensitivity=out.contrast.classifier.features.information_sensitivity;
  else a.fingerprint.information_sensitivity=out.contrast.classifier.features.information_sensitivity;
 }
 return out;
}
InformationContrastReport run_information_contrast_experiment(InformationTreatment dimension, std::uint64_t seed){ return run_information_contrast_impl(dimension,seed,false); }
InformationContrastReport run_information_contrast_experiment(std::uint64_t seed){
 return run_information_contrast_experiment(InformationTreatment::News, seed);
}
std::vector<InformationContrastReport> run_information_treatment_suite(std::uint64_t seed){
 return {run_information_contrast_experiment(InformationTreatment::News,seed),
         run_information_contrast_experiment(InformationTreatment::MarketHistory,seed),
         run_information_contrast_experiment(InformationTreatment::PeerObservations,seed)};
}
std::vector<InformationContrastReport> run_information_treatment_suite_live(std::uint64_t seed){
 return {run_information_contrast_impl(InformationTreatment::News,seed,true),
         run_information_contrast_impl(InformationTreatment::MarketHistory,seed,true),
         run_information_contrast_impl(InformationTreatment::PeerObservations,seed,true)};
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
  <<",\"classifier_available\":"<<(r.classifier_available?"true":"false")
  <<",\"classifier\":\""<<r.classification.label<<"\""
  <<",\"classifier_mode\":\""<<r.classifier_mode<<"\""
  <<",\"classifier_synthetic\":"<<(r.classifier_synthetic?"true":"false")
  <<",\"fingerprints\":[";
 bool first=true;
 for(const auto& a:r.agents) if(a.fingerprint_available){
  if(!first) o<<",";
  first=false;
  const auto& f=a.fingerprint;
  o<<"{\"agent_id\":\""<<f.agent_id<<"\",\"model\":\""<<a.model.model<<"\""
   <<",\"aggressiveness\":"<<f.aggressiveness
   <<",\"reservation_value_violation_rate\":"<<f.reservation_value_violation_rate
   <<",\"information_sensitivity\":"<<f.information_sensitivity
   <<",\"peer_sensitivity\":"<<f.peer_sensitivity
   <<",\"price_improvement\":"<<f.price_improvement
   <<",\"trade_frequency\":"<<f.trade_frequency
   <<",\"surplus_capture\":"<<f.surplus_capture
   <<",\"action_validity\":"<<f.action_validity
   <<",\"response_consistency\":"<<f.response_consistency<<"}";
 }
 o<<"]}"; return o.str();
}
}
