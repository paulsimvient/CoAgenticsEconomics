#include "coagentics/analysis/Phenotype.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <memory>
#include <sstream>
namespace coagentics::analysis {
using namespace coagentics::agents; using coagentics::market::Side;
namespace {
std::unique_ptr<AgentPolicy> make(const std::string&m,std::uint64_t seed){
 if(m=="information-blind")return std::make_unique<InformationBlindAgent>(m,100,Side::Buy);
 if(m=="signal-responsive")return std::make_unique<SignalResponsiveAgent>(m,100,Side::Buy,1.0);
 if(m=="risk-sensitive")return std::make_unique<RiskSensitiveAgent>(m,100,Side::Buy,.25);
 if(m=="peer-responsive")return std::make_unique<PeerResponsiveAgent>(m,100,Side::Buy,.6);
 if(m=="adaptive")return std::make_unique<AdaptiveAgent>(m,100,Side::Buy,.2);
 return std::make_unique<ZeroIntelligenceAgent>(m,100,Side::Buy,seed,1,40);
}
Observation obs(std::uint64_t t,double sig=0,double last=100){return{t,"asset",100,sig,95,105,last};}
double price(AgentPolicy&a,const Observation&o){return a.act(o).price;}
double clamp01(double x){return std::clamp(x,0.,1.);}
std::string esc(std::string s){std::string o;for(char c:s){if(c=='"')o+="\\\"";else if(c=='\n')o+="\\n";else o+=c;}return o;}
}
PhenotypeResult phenotype_known_mechanism(const std::string&m,std::uint64_t seed){
 BehavioralFingerprint f;f.agent_id=m;f.mechanism=m;
 auto a=make(m,seed);double p0=price(*a,obs(0));f.value_shading=(p0-100.)/100.;
 auto ni=make(m,seed);double n0=price(*ni,obs(0,0,100)),n1=price(*ni,obs(1,10,100));f.news_sensitivity=(n1-n0)/10.;
 auto pe=make(m,seed);double q0=price(*pe,obs(0,0,100)),q1=price(*pe,obs(1,0,120));f.peer_susceptibility=(q1-q0)/20.;
 // latency under a persistent signal
 auto lat=make(m,seed);double base=price(*lat,obs(0));for(int t=1;t<=5;++t){double p=price(*lat,obs(t,10,100));if(f.response_latency<0&&std::abs(p-base)>=2.0)f.response_latency=t-1;}
 // recovery: compare signal-on price to first signal-off price using a fresh stateful probe
 auto rec=make(m,seed);double rb=price(*rec,obs(0));double ron=price(*rec,obs(1,10,100));double roff=price(*rec,obs(2,0,100));double moved=std::abs(ron-rb);f.recovery=moved>1e-9?clamp01(1.-std::abs(roff-rb)/moved):1.;
 // adaptation: repeated peer/news evidence changes the same stateful policy beyond its first response
 auto ad=make(m,seed);(void)price(*ad,obs(0));double first=price(*ad,obs(1,10,120));double last=first;for(int t=2;t<=6;++t)last=price(*ad,obs(t,10,120));f.adaptation=clamp01(std::abs(last-first)/20.);
 // Controlled proxies; actual live-provider operational counts are populated by the LLM harness, not invented here.
 f.price_influence=clamp01((std::abs(f.news_sensitivity)*.55+std::abs(f.peer_susceptibility)*.45));
 f.utility_capture=clamp01(1.-std::min(1.,std::abs(p0-100.)/40.));f.withholding=0;
 std::vector<ExplanationScore> e;
 double direct=clamp01(std::abs(f.news_sensitivity));double peer=clamp01(std::abs(f.peer_susceptibility));double adapt=clamp01(f.adaptation*4);double artifact=clamp01(1.-std::max(direct,peer));
 e.push_back({"H1 direct information response",direct,"information ON / peer visibility OFF"});
 e.push_back({"H2 peer-mediated response",peer,"information OFF / peer visibility ON"});
 e.push_back({"H3 adaptive state update",adapt,"repeat perturbation then withdraw it"});
 e.push_back({"H4 market/institution artifact",artifact,"replace focal policy with information-blind matched control"});
 auto best=std::max_element(e.begin(),e.end(),[](auto&a,auto&b){return a.support<b.support;});
 PhenotypeResult r;r.fingerprint=f;r.explanations=e;r.next_experiment=best->discriminator;return r;
}
PhenotypeReport run_behavioral_phenotyping(std::uint64_t seed){PhenotypeReport r;r.seed=seed;for(auto&m:{"information-blind","signal-responsive","risk-sensitive","peer-responsive","adaptive"})r.agents.push_back(phenotype_known_mechanism(m,seed));r.interpretation="Behavioral fingerprints are mechanism-recovery controls, not live-LLM findings. Efficiency is intentionally absent from the fingerprint: v13 showed that institutionally disciplined markets can hide distinct policies behind similar efficiency. Operational failure fields remain zero for these in-process controls and must be populated from the v8 provider harness for live models.";return r;}
std::string phenotype_markdown(const PhenotypeReport&r){std::ostringstream o;o<<"# V14 Behavioral Phenotyping\n\n**Live LLMs used:** NO — known-mechanism controls validate feature recovery before live-model use.\n\n| Mechanism | News sens. | Latency | Peer susc. | Adaptation | Recovery | Influence proxy | Utility proxy |\n|---|---:|---:|---:|---:|---:|---:|---:|\n";for(auto&a:r.agents){auto&f=a.fingerprint;o<<"| "<<f.mechanism<<" | "<<std::fixed<<std::setprecision(3)<<f.news_sensitivity<<" | "<<f.response_latency<<" | "<<f.peer_susceptibility<<" | "<<f.adaptation<<" | "<<f.recovery<<" | "<<f.price_influence<<" | "<<f.utility_capture<<" |\n";}o<<"\n## Competing explanations and discriminators\n";for(auto&a:r.agents){o<<"\n### "<<a.fingerprint.mechanism<<"\n";for(auto&e:a.explanations)o<<"- "<<e.hypothesis<<": support="<<std::setprecision(3)<<e.support<<"; discriminator: "<<e.discriminator<<"\n";o<<"- **Next experiment:** "<<a.next_experiment<<"\n";}o<<"\n## Interpretation boundary\n"<<r.interpretation<<"\n";return o.str();}
std::string phenotype_json(const PhenotypeReport&r){std::ostringstream o;o<<"{\"version\":\"v14\",\"seed\":"<<r.seed<<",\"live_llms_used\":false,\"interpretation\":\""<<esc(r.interpretation)<<"\",\"agents\":[";for(size_t i=0;i<r.agents.size();++i){if(i)o<<",";auto&a=r.agents[i];auto&f=a.fingerprint;o<<"{\"agent_id\":\""<<f.agent_id<<"\",\"mechanism\":\""<<f.mechanism<<"\",\"value_shading\":"<<f.value_shading<<",\"news_sensitivity\":"<<f.news_sensitivity<<",\"response_latency\":"<<f.response_latency<<",\"peer_susceptibility\":"<<f.peer_susceptibility<<",\"price_influence\":"<<f.price_influence<<",\"utility_capture\":"<<f.utility_capture<<",\"adaptation\":"<<f.adaptation<<",\"withholding\":"<<f.withholding<<",\"recovery\":"<<f.recovery<<",\"abstentions\":"<<f.abstentions<<",\"malformed_actions\":"<<f.malformed_actions<<",\"provider_failures\":"<<f.provider_failures<<",\"next_experiment\":\""<<esc(a.next_experiment)<<"\",\"explanations\":[";for(size_t j=0;j<a.explanations.size();++j){if(j)o<<",";auto&e=a.explanations[j];o<<"{\"hypothesis\":\""<<esc(e.hypothesis)<<"\",\"support\":"<<e.support<<",\"discriminator\":\""<<esc(e.discriminator)<<"\"}";}o<<"]}";}o<<"]}";return o.str();}
}
