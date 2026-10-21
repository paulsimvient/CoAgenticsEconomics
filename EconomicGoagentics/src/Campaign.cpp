#include "coagentics/experiment/Campaign.hpp"
#include "coagentics/analysis/Behavior.hpp"
#include "coagentics/analysis/Hypothesis.hpp"
#include <cmath>
#include <iomanip>
#include <sstream>
namespace coagentics::experiment {
using namespace coagentics;
static double mean(const std::vector<double>& x){double s=0;for(double v:x)s+=v;return x.empty()?0:s/x.size();}
CampaignSummary run_behavior_campaign(const CampaignConfig& c){
 CampaignSummary out;out.config=c;std::vector<double> ce,te,de,ts,ns,pr;
 analysis::BehavioralHypothesis h{"H-peer-propagation","A private signal changes the target and propagates through market interaction",{{"D-private-signal","Private signal: target must move; propagation may remain bounded",1.0,0.0,0.25}}};
 for(std::size_t i=0;i<c.trials;i++){
  auto seed=c.first_seed+i; InformationTimeline ctl,trt; for(const auto& target:c.targeted_agents) trt.add({c.intervention_time,"private-news-"+target,"ASSET",c.signal,c.reliability,target,"controlled private signal",std::nullopt});
  auto a=run_reference_auction(seed,c.auction,ctl,false); auto b=run_reference_auction(seed,c.auction,trt,false);
  auto p=analysis::compare_paired_bids(a.bids,b.bids,c.intervention_time,c.targeted_agents);
  ce.push_back(a.metrics.allocative_efficiency);te.push_back(b.metrics.allocative_efficiency);de.push_back(b.metrics.allocative_efficiency-a.metrics.allocative_efficiency);ts.push_back(p.targeted_mean_abs_shift);ns.push_back(p.non_target_mean_abs_shift);pr.push_back(p.propagation_ratio);
  auto ev=analysis::evaluate(h,h.discriminators.front(),p); if(ev.direction==analysis::EvidenceDirection::Supports)out.supports++;else if(ev.direction==analysis::EvidenceDirection::Challenges)out.challenges++;else out.inconclusive++;
  out.evidence.append({c.id,c.id+"-control-"+std::to_string(seed),c.id+"-treatment-"+std::to_string(seed),seed,std::move(ev)});
 }
 out.control_mean_efficiency=mean(ce);out.treatment_mean_efficiency=mean(te);out.mean_efficiency_delta=mean(de);out.targeted_mean_abs_shift=mean(ts);out.non_target_mean_abs_shift=mean(ns);out.mean_propagation_ratio=mean(pr);
 double ss=0;for(double v:de)ss+=(v-out.mean_efficiency_delta)*(v-out.mean_efficiency_delta);double se=de.size()>1?std::sqrt(ss/(de.size()-1))/std::sqrt((double)de.size()):0;out.ci95_low=out.mean_efficiency_delta-1.96*se;out.ci95_high=out.mean_efficiency_delta+1.96*se;return out;
}
std::string campaign_json(const CampaignSummary&s){std::ostringstream o;o<<std::fixed<<std::setprecision(6);o<<"{\n  \"campaign_id\": \""<<s.config.id<<"\",\n  \"trials\": "<<s.config.trials<<",\n  \"market\": {\"control_mean_efficiency\": "<<s.control_mean_efficiency<<", \"treatment_mean_efficiency\": "<<s.treatment_mean_efficiency<<", \"mean_delta\": "<<s.mean_efficiency_delta<<", \"ci95\": ["<<s.ci95_low<<", "<<s.ci95_high<<"]},\n  \"behavior\": {\"targeted_mean_abs_shift\": "<<s.targeted_mean_abs_shift<<", \"non_target_mean_abs_shift\": "<<s.non_target_mean_abs_shift<<", \"mean_propagation_ratio\": "<<s.mean_propagation_ratio<<"},\n  \"evidence\": {\"supports\": "<<s.supports<<", \"challenges\": "<<s.challenges<<", \"inconclusive\": "<<s.inconclusive<<"}\n}\n";return o.str();}
std::string campaign_html(const CampaignSummary&s){std::ostringstream o;o<<std::fixed<<std::setprecision(3);o<<"<!doctype html><meta charset='utf-8'><title>CoAgentics Behavior Lab</title><style>body{font:15px system-ui;margin:32px;background:#f6f7f8;color:#18202a}h1{margin-bottom:4px}.sub{color:#667085}.grid{display:grid;grid-template-columns:repeat(4,1fr);gap:12px;margin:24px 0}.card{background:white;border:1px solid #ddd;border-radius:8px;padding:16px}.big{font-size:28px;font-weight:650}.flow{background:white;padding:18px;border:1px solid #ddd;border-radius:8px}code{background:#eef1f4;padding:2px 5px}</style><h1>CoAgentics Behavior Lab</h1><div class='sub'>"<<s.config.id<<" · "<<s.config.trials<<" matched seeds · private information intervention</div><div class='grid'><div class='card'>Market<div class='big'>"<<s.treatment_mean_efficiency<<"%</div>treatment efficiency<br>Δ "<<s.mean_efficiency_delta<<" pp</div><div class='card'>Target agent<div class='big'>"<<s.targeted_mean_abs_shift<<"</div>mean |bid shift|</div><div class='card'>Propagation<div class='big'>"<<s.mean_propagation_ratio<<"</div>non-target / target response</div><div class='card'>Evidence<div class='big'>"<<s.supports<<" / "<<s.config.trials<<"</div>trials support preregistered discriminator</div></div><div class='flow'><b>Scientific trace</b><p>Observation → competing hypothesis → private-signal discriminator → matched control/treatment → behavior delta → evidence record.</p><p>95% CI for efficiency Δ: <code>["<<s.ci95_low<<", "<<s.ci95_high<<"]</code></p><p>Interpretation is intentionally downstream of measurement. Correlation is not labeled collusion, conformity, or deception without a discriminating experiment.</p></div>";return o.str();}
}
