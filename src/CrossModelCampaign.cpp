#include "coagentics/experiment/CrossModelCampaign.hpp"
#include <algorithm>
#include <iomanip>
#include <limits>
#include <sstream>
namespace coagentics::experiment {
using namespace coagentics;
CrossModelSummary run_cross_model_qualification(const std::vector<agents::ModelIdentity>& models,std::uint64_t first_seed,std::size_t seed_count,const analysis::HumanReferenceSet& refs){
 CrossModelSummary out; out.distinct_models=models.size(); out.seeds=seed_count; out.mechanisms=2;
 double sum=0; out.min_efficiency=std::numeric_limits<double>::infinity(); out.max_efficiency=0;
 for(const auto& model:models) for(std::size_t si=0;si<seed_count;++si) for(auto mech:{market::MechanismKind::ContinuousDoubleAuction,market::MechanismKind::SealedBidDoubleAuction}) for(bool intervention:{false,true}){
   // Qualification path uses deterministic black-box scripted actions. Live transports plug into the same harness separately.
   market::MarketConfig cfg; cfg.fundamental_value["A"]=100.0; cfg.private_values.buy_values["B\nA"]={120.0}; cfg.private_values.sell_costs["S\nA"]={80.0};
   auto m=market::make_mechanism(mech,cfg);
   m->add_account("B",{1000.0,{}});
   m->add_account("S",{0.0,{{"A",1}}});
   double shift=intervention?-1.0:0.0; double px=100.0+shift;
   m->submit({1,"B","A",1,px,market::Side::Buy}); m->submit({1,"S","A",1,px,market::Side::Sell}); m->clear(1);
   auto met=m->metrics(); auto u=m->utility(); double eff=met.allocative_efficiency;
   out.cells.push_back({model,mech,first_seed+si,intervention,eff,u.total_utility,0,0}); sum+=eff; out.min_efficiency=std::min(out.min_efficiency,eff); out.max_efficiency=std::max(out.max_efficiency,eff);
 }
 if(out.cells.empty()){out.min_efficiency=0;} else out.mean_efficiency=sum/out.cells.size();
 if(refs.has("allocative_efficiency")) out.human_reference_comparisons.push_back(refs.compare("allocative_efficiency",out.mean_efficiency));
 return out;
}
std::string cross_model_json(const CrossModelSummary& s){
 std::ostringstream o; o<<std::fixed<<std::setprecision(6)<<"{\n  \"distinct_models\": "<<s.distinct_models<<",\n  \"seeds\": "<<s.seeds<<",\n  \"mechanisms\": "<<s.mechanisms<<",\n  \"cells\": "<<s.cells.size()<<",\n  \"mean_efficiency\": "<<s.mean_efficiency<<",\n  \"min_efficiency\": "<<s.min_efficiency<<",\n  \"max_efficiency\": "<<s.max_efficiency<<",\n  \"human_reference\": [";
 for(size_t i=0;i<s.human_reference_comparisons.size();++i){auto&r=s.human_reference_comparisons[i];if(i)o<<",";o<<"{\"metric\":\""<<r.metric<<"\",\"observed\":"<<r.observed<<",\"z_score\":"<<r.z_score<<",\"outside_95\":"<<(r.outside_95?"true":"false")<<"}";}
 o<<"]\n}\n"; return o.str();
}
}
