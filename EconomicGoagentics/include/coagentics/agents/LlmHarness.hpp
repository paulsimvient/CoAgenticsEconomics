#pragma once
#include "coagentics/agents/Agents.hpp"
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>
namespace coagentics::agents {
struct ModelIdentity { std::string provider; std::string model; std::string version; std::string endpoint_class{"black-box"}; };
struct LlmAction { std::uint64_t time{}; std::string asset; int quantity{}; double price{}; coagentics::market::Side side{coagentics::market::Side::Buy}; bool abstain{false}; };
struct ModelRequest {
 std::string request_id;
 std::string agent_id;
 ModelIdentity model;
 Observation observation; // public projection only
 std::uint64_t seed{};
 std::string decision_context_json; // market + own agent state; never other agents' private state
};
struct ModelResponse {
 std::string request_id;
 std::string raw_output;
 std::optional<LlmAction> action;
 std::string error;
 std::uint64_t latency_ms{};
 std::uint64_t prompt_tokens{};
 std::uint64_t completion_tokens{};
 std::string inference_config; // opaque provider configuration snapshot (no secrets)
};
// Parse a model payload into LlmAction. Accepts canonical {"action":"BUY|SELL|HOLD",...}
// and legacy {"side":"buy|sell",...}. Does not validate economics.
struct ParseResult { bool ok{false}; std::optional<LlmAction> action; std::string error; };
ParseResult parse_market_action(const std::string& raw_output);
struct HarnessRecord { ModelRequest request; ModelResponse response; };
class ModelTransport { public: virtual ~ModelTransport()=default; virtual ModelResponse invoke(const ModelRequest&)=0; };
class ModelRegistry {
public:
 void add(ModelIdentity m); bool contains(const std::string& provider,const std::string& model,const std::string& version) const;
 const std::vector<ModelIdentity>& models() const{return models_;}
private: std::vector<ModelIdentity> models_;
};
class LlmHarnessProvider final : public LlmBidProvider {
public:
 LlmHarnessProvider(ModelIdentity identity,std::shared_ptr<ModelTransport> transport,std::uint64_t seed=0,std::string log_path={});
 coagentics::market::Bid request_bid(const std::string& agent_id,const Observation&) override;
 const std::vector<HarnessRecord>& records()const{return records_;}
 std::size_t failures()const{return failures_;} std::size_t abstentions()const{return abstentions_;}
private:
 ModelIdentity identity_; std::shared_ptr<ModelTransport> transport_; std::uint64_t seed_; std::uint64_t seq_{0}; std::string log_path_; std::vector<HarnessRecord> records_; std::size_t failures_{0},abstentions_{0};
 void append_log(const HarnessRecord&) const;
};
class ReplayTransport final : public ModelTransport {
public: explicit ReplayTransport(std::vector<ModelResponse> responses):responses_(std::move(responses)){} ModelResponse invoke(const ModelRequest&)override;
private:std::vector<ModelResponse> responses_;std::size_t next_{0};
};
class ScriptedTransport final : public ModelTransport {
public: explicit ScriptedTransport(std::vector<LlmAction> actions):actions_(std::move(actions)){} ModelResponse invoke(const ModelRequest&)override;
private:std::vector<LlmAction> actions_;std::size_t next_{0};
};
bool valid_action(const LlmAction&,const Observation&,std::string* why=nullptr);
std::string canonical_observation_json(const ModelRequest&);
}
