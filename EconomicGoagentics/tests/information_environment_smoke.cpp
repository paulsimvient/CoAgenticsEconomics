#include "coagentics/domain/InformationEnvironment.hpp"
#include <cassert>
#include <stdexcept>

using namespace coagentics::domain;

static InformationEvent ev(const char* id, const char* source, std::uint64_t observed,
                            std::optional<std::uint64_t> visible_after = std::nullopt,
                            std::optional<std::uint64_t> expires_at = std::nullopt,
                            std::optional<std::string> visible_to = std::nullopt) {
    return InformationEvent{id, "asset", observed, id, 0.9, 1.0,
                            visible_after, expires_at, visible_to, source};
}

int main() {
    ExplicitInformationEnvironment env;
    env.publish(ev("news", "public-news", 10));
    env.publish(ev("price", "price-history", 10));
    env.publish(ev("peer", "peer-action", 10));
    env.publish(ev("private", "private-state", 10));
    env.publish(ev("delayed", "public-news", 10, 20));
    env.publish(ev("expired", "public-news", 10, std::nullopt, 15));
    env.publish(ev("alice-only", "public-news", 10, std::nullopt, std::nullopt, "alice"));

    AgentRef alice{"alice", "buyer", "test", "reference", "1"};
    InformationPolicy p;
    p.allow_peer_actions = false;
    p.allow_private_state = false;
    p.history_window = 100;
    p.information_delay = 0;

    auto at10 = observe_with_information({"market", "1", 7, 10}, alice, p, env);
    assert(at10.information.size() == 4); // news, price, unexpired event, alice-only

    p.allow_public_news = false;
    auto no_news = observe_with_information({"market", "1", 7, 10}, alice, p, env);
    assert(no_news.information.size() == 1); // price only

    p.allow_public_news = true;
    p.information_delay = 5;
    auto delayed = observe_with_information({"market", "1", 7, 19}, alice, p, env);
    for (const auto& e : delayed.information) assert(e.event_id != "delayed");
    auto visible = observe_with_information({"market", "1", 7, 20}, alice, p, env);
    assert(visible.information.size() == 4); // delayed becomes visible, expired remains gone

    auto expired = observe_with_information({"market", "1", 7, 20}, alice, p, env);
    for (const auto& e : expired.information) assert(e.event_id != "expired");

    p.information_delay = 0;
    p.history_window = 2;
    auto history = observe_with_information({"market", "1", 7, 13}, alice, p, env);
    assert(history.information.empty());

    AgentRef bob{"bob", "seller", "test", "reference", "1"};
    auto bob_obs = observe_with_information({"market", "1", 7, 10}, bob, p, env);
    for (const auto& e : bob_obs.information) assert(e.event_id != "alice-only");

    bool rejected = false;
    try { env.publish(ev("bad", "public-news", 10, 9)); }
    catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected);
    return 0;
}
