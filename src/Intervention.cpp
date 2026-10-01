#include "coagentics/experiment/Intervention.hpp"
#include <algorithm>
namespace coagentics::experiment {
void InformationTimeline::add(InformationEvent e){ events_.push_back(std::move(e)); std::stable_sort(events_.begin(),events_.end(),[](auto&a,auto&b){return a.time<b.time;}); }
double InformationTimeline::signal_for(const std::string& agent,const std::string& asset,std::uint64_t time) const{
 double s=0.0; for(const auto&e:events_) if(e.time<=time && (!e.expires_at || time<*e.expires_at) && e.asset==asset && (!e.visible_to || *e.visible_to==agent)) s += e.signal*e.reliability; return s;
}
}
