#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace coagentics::llm {

enum class Availability { Unknown, Available, Unavailable, RateLimited, Error };
const char* availability_name(Availability a);

enum class InvocationStatus { Executed, CacheHit, BudgetDenied, Unavailable, Timeout, ProviderError, InvalidResponse };
const char* invocation_status_name(InvocationStatus s);

struct ModelIdentity {
    std::string provider;
    std::string model;
    std::string version;
    std::string endpoint;
};

struct InferenceConfig {
    double temperature{0.0};
    double top_p{1.0};
    std::size_t max_output_tokens{512};
    std::uint32_t timeout_ms{30000};
    bool deterministic{true};
    std::string system_prompt_version{"coagentics-system-v1"};
};

struct ModelStatus {
    ModelIdentity identity;
    Availability availability{Availability::Unknown};
    std::string checked_at;
    std::string detail;
};

struct QueryBudget {
    std::size_t max_requests{0};       // 0 = unlimited
    std::size_t requests_used{0};
    double max_cost_usd{0.0};           // 0 = unlimited
    double cost_used_usd{0.0};
};

struct LlmRequest {
    ModelIdentity model;
    InferenceConfig config;
    std::string canonical_observation;
    std::string observation_fingerprint;
    std::string request_fingerprint;
    std::string run_id;
};

struct LlmResponse {
    InvocationStatus status{InvocationStatus::ProviderError};
    std::string raw_text;
    std::string response_fingerprint;
    std::string provider_request_id;
    std::size_t input_tokens{0};
    std::size_t output_tokens{0};
    double estimated_cost_usd{0.0};
    bool from_cache{false};
    std::string error;
};

struct InvocationRecord {
    LlmRequest request;
    LlmResponse response;
    std::size_t sequence{0};
};

class ModelRegistry {
public:
    void register_model(ModelIdentity identity);
    bool contains(const std::string& provider, const std::string& model, const std::string& version) const;
    std::optional<ModelIdentity> find(const std::string& provider, const std::string& model, const std::string& version) const;
    void set_status(const std::string& provider, const std::string& model, const std::string& version,
                    Availability availability, std::string detail = {});
    std::optional<ModelStatus> status(const std::string& provider, const std::string& model, const std::string& version) const;
    const std::vector<ModelIdentity>& models() const { return models_; }
private:
    std::vector<ModelIdentity> models_;
    std::unordered_map<std::string, ModelStatus> statuses_;
};

class LlmCache {
public:
    void put(const std::string& request_fingerprint, const LlmResponse& response);
    std::optional<LlmResponse> get(const std::string& request_fingerprint) const;
    std::size_t size() const { return entries_.size(); }
private:
    std::unordered_map<std::string, LlmResponse> entries_;
};

using ProviderFn = std::function<LlmResponse(const LlmRequest&)>;

class LlmHarness {
public:
    explicit LlmHarness(ModelRegistry registry = {});

    ModelRegistry& registry() { return registry_; }
    const ModelRegistry& registry() const { return registry_; }

    void bind_provider(const std::string& provider, ProviderFn fn);
    void set_budget(QueryBudget budget) { budget_ = budget; }
    const QueryBudget& budget() const { return budget_; }

    LlmResponse invoke(const LlmRequest& request);
    const std::vector<InvocationRecord>& records() const { return records_; }
    void clear_records();
    void clear_cache() { cache_ = LlmCache{}; }

    static std::string fingerprint(const std::string& text);
    static std::string canonical_request(const LlmRequest& request);

private:
    ModelRegistry registry_;
    LlmCache cache_;
    QueryBudget budget_;
    std::unordered_map<std::string, ProviderFn> providers_;
    std::vector<InvocationRecord> records_;
    std::size_t sequence_{0};
};

} // namespace coagentics::llm
