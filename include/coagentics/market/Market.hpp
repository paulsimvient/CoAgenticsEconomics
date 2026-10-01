#pragma once
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>
namespace coagentics::market {
enum class Side { Buy, Sell };
struct Bid { std::uint64_t time{}; std::string agent_id; std::string asset; int quantity{}; double price{}; Side side{Side::Buy}; };
struct Trade { std::uint64_t time{}; std::string asset; int quantity{}; double price{}; std::string buyer; std::string seller; };
struct Account { double cash{}; std::map<std::string,int> inventory; };
// Private values are unit-level marginal values/costs. They are evaluator truth, never exposed by Market.
struct PrivateValues { std::map<std::string,std::vector<double>> buy_values; std::map<std::string,std::vector<double>> sell_costs; };
struct MarketConfig { std::map<std::string,double> fundamental_value; std::map<std::string,int> total_supply; PrivateValues private_values; };
struct Metrics { double realized_surplus{}; double maximum_surplus{}; double allocative_efficiency{}; int efficient_quantity{}; };
// Acceptance is not the same as a fill. Resting accepted orders have filled_quantity==0.
struct SubmitResult {
 bool accepted{false};
 std::string rejection_reason;
 int submitted_quantity{0};
 int filled_quantity{0};
 int resting_quantity{0};
 std::optional<double> average_fill_price;
 std::vector<Trade> trades;
};
class Market {
public:
 explicit Market(MarketConfig config);
 void add_account(std::string id, Account account);
 bool submit(const Bid& bid);
 SubmitResult submit_detailed(const Bid& bid);
 const std::vector<Trade>& trades() const { return trades_; }
 const std::map<std::string,Account>& accounts() const { return accounts_; }
 Metrics metrics() const;
 double best_bid(const std::string& asset) const;
 double best_ask(const std::string& asset) const;
 double last_trade_price(const std::string& asset) const;
 bool has_bid(const std::string& asset) const;
 bool has_ask(const std::string& asset) const;
 int resting_quantity(const std::string& agent_id, const std::string& asset) const;
private:
 MarketConfig config_;
 std::map<std::string,Account> accounts_;
 std::vector<Bid> buys_, sells_;
 std::vector<Trade> trades_;
 std::map<std::string,int> bought_units_, sold_units_;
 void match(const std::string& asset, std::uint64_t time);
};
}
