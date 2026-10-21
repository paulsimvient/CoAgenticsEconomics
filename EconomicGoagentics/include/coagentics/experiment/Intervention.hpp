#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
namespace coagentics::experiment {
struct InformationEvent {
 std::uint64_t time{}; std::string id; std::string asset; double signal{}; double reliability{1.0};
 std::optional<std::string> visible_to; std::string content; std::optional<std::uint64_t> expires_at;
};
class InformationTimeline {
public:
 void add(InformationEvent event);
 double signal_for(const std::string& agent_id,const std::string& asset,std::uint64_t time) const;
 const std::vector<InformationEvent>& events() const { return events_; }
private: std::vector<InformationEvent> events_;
};
}
