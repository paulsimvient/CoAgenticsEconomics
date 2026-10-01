#include "coagentics/experiment/Scientist.hpp"
#include "coagentics/agents/Agents.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <memory>
#include <numeric>
#include <sstream>
#include <stdexcept>
namespace coagentics::experiment {
namespace {
double predict(const Candidate&c,const Probe&p){return c.news_gain*p.news+c.peer_gain*p.peer+c.adaptation_gain*(p.repetitions-1)*p.news;}
std::unique_ptr<agents::AgentPolicy> agent(const std::string&m,std::uint64_t seed){using namespace agents;using market::Side;
 if(m=="information-blind")return std::make_unique<InformationBlindAgent>(m,100,Side::Buy);
 if(m=="signal-responsive")return std::make_unique<SignalResponsiveAgent>(m,100,Side::Buy,1.);
 if(m=="risk-sensitive")return std::make_unique<RiskSensitiveAgent>(m,100,Side::Buy,.25);
 if(m=="peer-responsive")return std::make_unique<PeerResponsiveAgent>(m,100,Side::Buy,.6);
 if(m=="adaptive")return std::make_unique<AdaptiveAgent>(m,100,Side::Buy,.2);
 return std::make_unique<ZeroIntelligenceAgent>(m,100,Side::Buy,seed,1,40);
}
}
PairedObservation AgentProbeDomain::run(const Probe&p,std::uint64_t seed)const{
 auto control=agent(mechanism_,seed),treatment=agent(mechanism_,seed);
 auto obs=[](std::uint64_t t,double news,double peer){return agents::Observation{t,"asset",100,news,95,105,peer};};
 double c=0,t=0;for(int i=0;i<std::max(1,p.repetitions);++i){c=control->act(obs(i,0,100)).price;t=treatment->act(obs(i,p.news,100+p.peer)).price;}
 return {c,t,seed};
}
ScientistReport run_scientist(const ExperimentalDomain&domain,const std::vector<Candidate>&candidates,const std::vector<Probe>&probes,std::uint64_t seed,const ScientistConfig&cfg){
 if(candidates.size()<2||probes.empty()||cfg.replicates<2||cfg.noise_sd<=0)throw std::invalid_argument("invalid scientist configuration");
 ScientistReport r;r.domain=domain.name();r.candidates=candidates;
 std::vector<double> posterior;for(const auto&c:candidates)posterior.push_back(c.prior);
 double sum=std::accumulate(posterior.begin(),posterior.end(),0.);if(sum<=0)throw std::invalid_argument("invalid priors");for(double&v:posterior)v/=sum;
 std::vector<bool>used(probes.size(),false);
 for(std::size_t step=0;step<std::min(cfg.max_experiments,probes.size());++step){
  // Weighted expected separation is an information-gain surrogate, preregistered before observations.
  std::size_t chosen=probes.size();double best=-1;
  for(std::size_t k=0;k<probes.size();++k){if(used[k])continue;double mean=0,var=0;for(std::size_t j=0;j<candidates.size();++j)mean+=posterior[j]*predict(candidates[j],probes[k]);for(std::size_t j=0;j<candidates.size();++j){double d=predict(candidates[j],probes[k])-mean;var+=posterior[j]*d*d;}if(var>best){best=var;chosen=k;}}
  if(chosen==probes.size()||best<cfg.minimum_separation*cfg.minimum_separation){r.status="UNRESOLVED: no discriminating probe";break;}
  used[chosen]=true;const auto&p=probes[chosen];std::vector<double>deltas;deltas.reserve(cfg.replicates);
  for(std::size_t n=0;n<cfg.replicates;++n){auto pair=domain.run(p,seed+step*100000+n);deltas.push_back(pair.treatment-pair.control);}
  double mean=std::accumulate(deltas.begin(),deltas.end(),0.)/deltas.size();double ss=0;for(double d:deltas)ss+=(d-mean)*(d-mean);double se=std::sqrt(ss/(deltas.size()-1)/deltas.size());
  // The configured measurement floor prevents deterministic fixtures from yielding certainty.
  double variance=cfg.noise_sd*cfg.noise_sd+se*se;std::vector<double>logw;
  for(std::size_t j=0;j<candidates.size();++j){double diff=mean-predict(candidates[j],p);logw.push_back(std::log(std::max(1e-300,posterior[j]))-diff*diff/(2*variance));}
  double top=*std::max_element(logw.begin(),logw.end());double z=0;for(double&v:logw){v=std::exp(v-top);z+=v;}for(std::size_t j=0;j<posterior.size();++j)posterior[j]=logw[j]/z;
  auto leader=std::max_element(posterior.begin(),posterior.end());std::size_t lead=std::distance(posterior.begin(),leader);
  ExperimentStep record{p,mean,se,posterior,"CONTINUE"};
  for(std::size_t j=0;j<candidates.size();++j){analysis::EvidenceRecord e;e.hypothesis_id=candidates[j].id;e.discriminator_id=p.id;e.targeted_mean_abs_shift=std::abs(mean);e.direction=std::abs(mean-predict(candidates[j],p))<=2*std::sqrt(variance)?analysis::EvidenceDirection::Supports:analysis::EvidenceDirection::Challenges;e.rationale="Matched policy-state probe; likelihood update with declared measurement floor";r.evidence.append({"v15-step-"+std::to_string(step),"control-"+std::to_string(step),"treatment-"+std::to_string(step),seed,e});}
  if(*leader>=cfg.stop_posterior){record.decision="SEPARATED (controlled-model assumption)";r.status=record.decision;r.leading_hypothesis=candidates[lead].id;}
  r.steps.push_back(std::move(record));if(!r.status.empty())break;
 }
 if(r.status.empty())r.status="UNRESOLVED: experiment budget exhausted";
 if(r.leading_hypothesis.empty()&&!r.steps.empty()){auto&post=r.steps.back().posterior;r.leading_hypothesis=candidates[std::distance(post.begin(),std::max_element(post.begin(),post.end()))].id;}
 return r;
}
std::string scientist_markdown(const ScientistReport&r){std::ostringstream o;o<<"# CoAgentics v15 — Automated Scientist\n\nDomain adapter: "<<r.domain<<"\n\nStatus: **"<<r.status<<"**\n\nLeading explanation (not necessarily established): "<<r.leading_hypothesis<<"\n\n| Probe | Matched delta | SE | Decision |\n|---|---:|---:|---|\n";for(auto&s:r.steps)o<<"| "<<s.probe.id<<" | "<<std::fixed<<std::setprecision(3)<<s.observed_delta<<" | "<<s.standard_error<<" | "<<s.decision<<" |\n";o<<"\n## Posterior by step (conditional on candidate set and noise model)\n";for(auto&s:r.steps){o<<"\n"<<s.probe.id<<": ";for(size_t j=0;j<r.candidates.size();++j)o<<r.candidates[j].id<<"="<<std::setprecision(3)<<s.posterior[j]<<" ";o<<"\n";}o<<"\nThese are controlled in-process agents, not live LLMs. Posterior probabilities are model-conditional, not empirical probabilities of intent.\n";return o.str();}
}
