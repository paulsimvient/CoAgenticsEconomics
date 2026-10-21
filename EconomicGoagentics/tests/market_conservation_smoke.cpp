#include "coagentics/market/Market.hpp"
#include <cassert>
#include <cmath>
#include <map>
#include <string>
using namespace coagentics::market;

int main() {
  MarketConfig c;
  c.fundamental_value["A"] = 100;
  c.total_supply["A"] = 5;
  const double buy_v[] = {120, 116, 112, 108, 104};
  const double sell_c[] = {82, 86, 90, 94, 98};
  for (int i = 0; i < 5; ++i) {
    c.private_values.buy_values[std::string("b") + char('0' + i) + "\nA"] = {buy_v[i]};
    c.private_values.sell_costs[std::string("s") + char('0' + i) + "\nA"] = {sell_c[i]};
  }
  Market m(c);
  double cash0 = 0;
  int inv0 = 0;
  for (int i = 0; i < 5; ++i) {
    m.add_account(std::string("b") + char('0' + i), {10000, {}});
    m.add_account(std::string("s") + char('0' + i), {10000, {{"A", 1}}});
    cash0 += 20000;
    inv0 += 1;
  }
  for (int i = 0; i < 5; ++i) {
    assert(m.submit({static_cast<std::uint64_t>(i), std::string("b") + char('0' + i), "A", 1, buy_v[i] - 5, Side::Buy}));
    assert(m.submit({static_cast<std::uint64_t>(i), std::string("s") + char('0' + i), "A", 1, sell_c[i] + 5, Side::Sell}));
  }
  double cash1 = 0;
  int inv1 = 0;
  for (const auto& [id, acct] : m.accounts()) {
    cash1 += acct.cash;
    auto it = acct.inventory.find("A");
    if (it != acct.inventory.end()) inv1 += it->second;
  }
  assert(std::fabs(cash1 - cash0) < 1e-6);
  assert(inv1 == inv0);
  assert(m.trades().size() == 5);
  auto x = m.metrics();
  assert(std::fabs(x.maximum_surplus - 110.0) < 1e-9);
  assert(std::fabs(x.realized_surplus - 110.0) < 1e-9);
  assert(std::fabs(x.allocative_efficiency - 100.0) < 1e-9);

  // Extra cash-financed units do not reuse the seat's single induced value.
  MarketConfig c2;
  c2.fundamental_value["A"] = 100;
  c2.total_supply["A"] = 2;
  c2.private_values.buy_values["b\nA"] = {120};
  c2.private_values.sell_costs["s1\nA"] = {80};
  c2.private_values.sell_costs["s2\nA"] = {90};
  Market m2(c2);
  m2.add_account("b", {10000, {}});
  m2.add_account("s1", {10000, {{"A", 1}}});
  m2.add_account("s2", {10000, {{"A", 1}}});
  assert(m2.submit({0, "b", "A", 1, 110, Side::Buy}));
  assert(m2.submit({0, "s1", "A", 1, 85, Side::Sell}));
  assert(std::fabs(m2.metrics().realized_surplus - 40.0) < 1e-9);
  assert(m2.submit({1, "b", "A", 1, 110, Side::Buy}));
  assert(m2.submit({1, "s2", "A", 1, 95, Side::Sell}));
  assert(m2.trades().size() == 2);
  assert(std::fabs(m2.metrics().realized_surplus - 40.0) < 1e-9);
  assert(std::fabs(m2.metrics().maximum_surplus - 40.0) < 1e-9);
}
