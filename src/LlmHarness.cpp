#include "coagentics/agents/LlmHarness.hpp"
#include "coagentics/util/Json.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <sstream>
namespace coagentics::agents {
static std::string esc(const std::string&s){
 std::string o; o.reserve(s.size()+8);
 for(unsigned char c:s){
  switch(c){
   case '"': o+="\\\""; break; case '\\': o+="\\\\"; break;
   case '\n': o+="\\n"; break; case '\r': o+="\\r"; break; case '\t': o+="\\t"; break;
   default: if(c<0x20){char b[8]; std::snprintf(b,sizeof(b),"\\u%04x",(unsigned)c); o+=b;} else o.push_back((char)c);
  }
 }
 return o;
}
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


ParseResult parse_market_action(const std::string& raw_output){
 ParseResult r; auto j=coagentics::util::parse_json(raw_output); if(!j){r.error="invalid_json";return r;} if(!j->is_object()){r.error="schema_root_not_object";return r;}
 LlmAction a; auto abstain=j->get("abstain"); if(abstain){if(!abstain->is_bool()){r.error="abstain_wrong_type";return r;}a.abstain=abstain->as_bool();} if(a.abstain){r.ok=true;r.action=a;return r;}
 auto time=j->get("time"), asset=j->get("asset"), qty=j->get("quantity"), price=j->get("price"), side=j->get("side");
 if(!time){r.error="missing_time";return r;}if(!time->is_number()||time->as_number()<0||std::floor(time->as_number())!=time->as_number()){r.error="time_not_integer";return r;}
 if(!asset||!asset->is_string()){r.error=asset?"asset_wrong_type":"missing_asset";return r;}if(!qty||!qty->is_number()||std::floor(qty->as_number())!=qty->as_number()){r.error=qty?"quantity_not_integer":"missing_quantity";return r;}
 if(!price||!price->is_number()){r.error=price?"price_wrong_type":"missing_price";return r;}if(!side||!side->is_string()){r.error=side?"side_wrong_type":"missing_side";return r;}
 if(side->as_string()!="buy"&&side->as_string()!="sell"&&side->as_string()!="Buy"&&side->as_string()!="Sell"){r.error="invalid_side";return r;}
 a.time=(std::uint64_t)time->as_number();a.asset=asset->as_string();a.quantity=(int)qty->as_number();a.price=price->as_number();a.side=(side->as_string()=="sell"||side->as_string()=="Sell")?coagentics::market::Side::Sell:coagentics::market::Side::Buy;r.ok=true;r.action=a;return r;
}
}
