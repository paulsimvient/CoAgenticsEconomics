#include "coagentics/agents/LlmHarness.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
namespace coagentics::agents {
static std::string esc(const std::string&s){std::string o;for(char c:s){if(c=='"'||c=='\\')o+='\\';o+=c;}return o;}
static const char* side_name(coagentics::market::Side s){return s==coagentics::market::Side::Buy?"buy":"sell";}
void ModelRegistry::add(ModelIdentity m){if(!contains(m.provider,m.model,m.version))models_.push_back(std::move(m));}
bool ModelRegistry::contains(const std::string&p,const std::string&m,const std::string&v)const{return std::any_of(models_.begin(),models_.end(),[&](auto&x){return x.provider==p&&x.model==m&&x.version==v;});}
bool valid_action(const LlmAction&a,const Observation&o,std::string*why){auto bad=[&](std::string w){if(why)*why=std::move(w);return false;};if(a.abstain)return true;if(a.time!=o.time)return bad("time_mismatch");if(a.asset!=o.asset)return bad("asset_mismatch");if(a.quantity<=0||a.quantity>1000000)return bad("invalid_quantity");if(!std::isfinite(a.price)||a.price<0||a.price>1e12)return bad("invalid_price");return true;}
std::string canonical_observation_json(const ModelRequest&r){std::ostringstream s;s<<std::setprecision(17)<<"{\"request_id\":\""<<esc(r.request_id)<<"\",\"agent_id\":\""<<esc(r.agent_id)<<"\",\"seed\":"<<r.seed<<",\"model\":{\"provider\":\""<<esc(r.model.provider)<<"\",\"name\":\""<<esc(r.model.model)<<"\",\"version\":\""<<esc(r.model.version)<<"\"},\"observation\":{\"time\":"<<r.observation.time<<",\"asset\":\""<<esc(r.observation.asset)<<"\",\"fundamental\":"<<r.observation.fundamental<<",\"public_signal\":"<<r.observation.public_signal<<",\"best_bid\":"<<r.observation.best_bid<<",\"best_ask\":"<<r.observation.best_ask<<",\"last_trade\":"<<r.observation.last_trade<<"}}";return s.str();}
LlmHarnessProvider::LlmHarnessProvider(ModelIdentity i,std::shared_ptr<ModelTransport>t,std::uint64_t seed,std::string log):identity_(std::move(i)),transport_(std::move(t)),seed_(seed),log_path_(std::move(log)){}
coagentics::market::Bid LlmHarnessProvider::request_bid(const std::string&id,const Observation&o){ModelRequest q; q.request_id=identity_.provider+":"+identity_.model+":"+std::to_string(seed_)+":"+std::to_string(seq_++);q.agent_id=id;q.model=identity_;q.observation=o;q.seed=seed_;ModelResponse r=transport_->invoke(q);r.request_id=q.request_id;std::string why;if(!r.action){if(r.error.empty())r.error="missing_action";failures_++;}else if(!valid_action(*r.action,o,&why)){r.error=why;r.action.reset();failures_++;}else if(r.action->abstain){abstentions_++;}HarnessRecord rec{q,r};records_.push_back(rec);append_log(rec);if(!r.action||r.action->abstain)return{o.time,id,o.asset,0,0.0,coagentics::market::Side::Buy};auto&a=*r.action;return{a.time,id,a.asset,a.quantity,a.price,a.side};}
void LlmHarnessProvider::append_log(const HarnessRecord&x)const{if(log_path_.empty())return;std::ofstream f(log_path_,std::ios::app);f<<"{\"request\":"<<canonical_observation_json(x.request)<<",\"response\":{\"latency_ms\":"<<x.response.latency_ms<<",\"error\":\""<<esc(x.response.error)<<"\",\"raw_output\":\""<<esc(x.response.raw_output)<<"\"";if(x.response.action){auto&a=*x.response.action;f<<",\"action\":{\"time\":"<<a.time<<",\"asset\":\""<<esc(a.asset)<<"\",\"quantity\":"<<a.quantity<<",\"price\":"<<a.price<<",\"side\":\""<<side_name(a.side)<<"\",\"abstain\":"<<(a.abstain?"true":"false")<<"}";}f<<"}}\n";}
ModelResponse ReplayTransport::invoke(const ModelRequest&q){
 if(next_>=responses_.size()){ModelResponse e; e.request_id=q.request_id; e.error="replay_exhausted"; return e;}
 auto r=responses_[next_++]; r.request_id=q.request_id; return r;
}
ModelResponse ScriptedTransport::invoke(const ModelRequest&q){
 if(next_>=actions_.size()){ModelResponse e; e.request_id=q.request_id; e.error="script_exhausted"; return e;}
 auto a=actions_[next_++]; std::ostringstream raw; raw<<"{action:"<<side_name(a.side)<<" "<<a.quantity<<"@"<<a.price<<"}";
 ModelResponse r; r.request_id=q.request_id; r.raw_output=raw.str(); r.action=a; r.latency_ms=1; return r;
}

namespace {
bool find_string_field(const std::string& j,const std::string& key,std::string& out){
 const std::string pat="\""+key+"\""; auto p=j.find(pat); if(p==std::string::npos)return false;
 p=j.find(':',p+pat.size()); if(p==std::string::npos)return false;
 p=j.find('"',p+1); if(p==std::string::npos)return false;
 auto q=p+1; std::string v;
 while(q<j.size()){ if(j[q]=='\\'&&q+1<j.size()){v.push_back(j[q+1]);q+=2;continue;} if(j[q]=='"'){out=v;return true;} v.push_back(j[q++]); }
 return false;
}
bool find_number_field(const std::string& j,const std::string& key,double& out){
 const std::string pat="\""+key+"\""; auto p=j.find(pat); if(p==std::string::npos)return false;
 p=j.find(':',p+pat.size()); if(p==std::string::npos)return false; ++p;
 while(p<j.size()&&(j[p]==' '||j[p]=='\t'))++p;
 try{ size_t n=0; out=std::stod(j.substr(p),&n); return n>0; }catch(...){return false;}
}
bool find_bool_field(const std::string& j,const std::string& key,bool& out){
 const std::string pat="\""+key+"\""; auto p=j.find(pat); if(p==std::string::npos)return false;
 p=j.find(':',p+pat.size()); if(p==std::string::npos)return false; ++p;
 while(p<j.size()&&(j[p]==' '||j[p]=='\t'))++p;
 if(j.compare(p,4,"true")==0){out=true;return true;}
 if(j.compare(p,5,"false")==0){out=false;return true;}
 return false;
}
}
ParseResult parse_market_action(const std::string& raw_output){
 ParseResult r;
 std::string s=raw_output;
 auto l=s.find_first_not_of(" \t\r\n"); if(l==std::string::npos){r.error="empty_payload";return r;}
 auto rr=s.find_last_not_of(" \t\r\n"); s=s.substr(l,rr-l+1);
 if(s.rfind("```",0)==0){
  auto nl=s.find('\n'); if(nl!=std::string::npos)s=s.substr(nl+1);
  auto end=s.rfind("```"); if(end!=std::string::npos)s=s.substr(0,end);
  l=s.find_first_not_of(" \t\r\n"); rr=s.find_last_not_of(" \t\r\n");
  if(l==std::string::npos){r.error="empty_payload";return r;}
  s=s.substr(l,rr-l+1);
 }
 auto a=s.find('{'); auto b=s.rfind('}');
 if(a==std::string::npos||b==std::string::npos||b<=a){r.error="missing_json_object";return r;}
 const std::string obj=s.substr(a,b-a+1);
 LlmAction action; bool abstain=false;
 if(find_bool_field(obj,"abstain",abstain)) action.abstain=abstain;
 if(action.abstain){r.ok=true;r.action=action;return r;}
 double time=0,qty=0,price=0; std::string asset,side;
 if(!find_number_field(obj,"time",time)){r.error="missing_time";return r;}
 if(!find_string_field(obj,"asset",asset)){r.error="missing_asset";return r;}
 if(!find_number_field(obj,"quantity",qty)){r.error="missing_quantity";return r;}
 if(!find_number_field(obj,"price",price)){r.error="missing_price";return r;}
 if(!find_string_field(obj,"side",side)){r.error="missing_side";return r;}
 if(side!="buy"&&side!="sell"&&side!="Buy"&&side!="Sell"){r.error="invalid_side";return r;}
 action.time=static_cast<std::uint64_t>(time); action.asset=asset;
 action.quantity=static_cast<int>(qty);
 if(static_cast<double>(action.quantity)!=qty){r.error="quantity_not_integer";return r;}
 action.price=price;
 action.side=(side=="sell"||side=="Sell")?coagentics::market::Side::Sell:coagentics::market::Side::Buy;
 r.ok=true; r.action=action; return r;
}
}
