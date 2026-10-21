#pragma once
#include "coagentics/market/Market.hpp"
#include <map>
#include <memory>
#include <optional>
namespace coagentics::market {
enum class MechanismKind { ContinuousDoubleAuction, SealedBidDoubleAuction };
struct UtilitySummary { double buyer_utility{}; double seller_utility{}; double total_utility{}; };
class MarketMechanism {
public:
 virtual ~MarketMechanism()=default;
 virtual void add_account(std::string,Account)=0;
 virtual bool submit(const Bid&)=0;
 // Prefer submit_detailed for LLM paths (acceptance ≠ fill). Default wraps submit().
 virtual SubmitResult submit_detailed(const Bid& b){
  SubmitResult r; r.submitted_quantity=b.quantity;
  if(!submit(b)){ r.accepted=false; r.rejection_reason="mechanism_rejected"; return r; }
  r.accepted=true; r.resting_quantity=b.quantity; return r;
 }
 virtual void clear(std::uint64_t time)=0;
 virtual const std::vector<Trade>& trades() const=0;
 virtual Metrics metrics() const=0;
 virtual UtilitySummary utility() const=0;
 virtual const std::map<std::string,Account>& accounts() const=0;
 virtual std::optional<double> best_bid_opt(const std::string&) const { return std::nullopt; }
 virtual std::optional<double> best_ask_opt(const std::string&) const { return std::nullopt; }
 virtual MechanismKind kind() const=0;
};
std::unique_ptr<MarketMechanism> make_mechanism(MechanismKind, MarketConfig);
}
