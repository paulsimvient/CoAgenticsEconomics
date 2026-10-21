#include "coagentics/llm/LlmHarnessV2.hpp"

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace coagentics::llm {
namespace {
std::string key_for(const std::string& p, const std::string& m, const std::string& v) {
    return p + "|" + m + "|" + v;
}
std::uint64_t fnv1a(const std::string& s) {
    std::uint64_t h = 1469598103934665603ULL;
    for (unsigned char c : s) { h ^= c; h *= 1099511628211ULL; }
    return h;
}
std::string hex64(std::uint64_t x) {
    std::ostringstream o; o << std::hex << std::setw(16) << std::setfill('0') << x; return o.str();
}
}

const char* availability_name(Availability a) {
    switch (a) {
        case Availability::Unknown: return "unknown";
        case Availability::Available: return "available";
        case Availability::Unavailable: return "unavailable";
        case Availability::RateLimited: return "rate_limited";
        case Availability::Error: return "error";
    }
    return "unknown";
}

const char* invocation_status_name(InvocationStatus s) {
    switch (s) {
        case InvocationStatus::Executed: return "executed";
        case InvocationStatus::CacheHit: return "cache_hit";
        case InvocationStatus::BudgetDenied: return "budget_denied";
        case InvocationStatus::Unavailable: return "unavailable";
        case InvocationStatus::Timeout: return "timeout";
        case InvocationStatus::ProviderError: return "provider_error";
        case InvocationStatus::InvalidResponse: return "invalid_response";
    }
    return "provider_error";
}

void ModelRegistry::register_model(ModelIdentity identity) {
    if (identity.provider.empty() || identity.model.empty() || identity.version.empty())
        throw std::invalid_argument("model_identity_requires_provider_model_version");
    if (!contains(identity.provider, identity.model, identity.version)) {
        statuses_[key_for(identity.provider, identity.model, identity.version)] =
            ModelStatus{identity, Availability::Unknown, {}, {}};
        models_.push_back(std::move(identity));
    }
}

bool ModelRegistry::contains(const std::string& p, const std::string& m, const std::string& v) const {
    return statuses_.find(key_for(p,m,v)) != statuses_.end();
}

std::optional<ModelIdentity> ModelRegistry::find(const std::string& p, const std::string& m, const std::string& v) const {
    auto it = statuses_.find(key_for(p,m,v));
    if (it == statuses_.end()) return std::nullopt;
    return it->second.identity;
}

void ModelRegistry::set_status(const std::string& p, const std::string& m, const std::string& v,
                               Availability a, std::string detail) {
    auto it = statuses_.find(key_for(p,m,v));
    if (it == statuses_.end()) throw std::invalid_argument("model_not_registered");
    it->second.availability = a;
    it->second.detail = std::move(detail);
}

std::optional<ModelStatus> ModelRegistry::status(const std::string& p, const std::string& m, const std::string& v) const {
    auto it = statuses_.find(key_for(p,m,v));
    if (it == statuses_.end()) return std::nullopt;
    return it->second;
}

void LlmCache::put(const std::string& request_fingerprint, const LlmResponse& response) {
    entries_[request_fingerprint] = response;
}

std::optional<LlmResponse> LlmCache::get(const std::string& request_fingerprint) const {
    auto it = entries_.find(request_fingerprint);
    if (it == entries_.end()) return std::nullopt;
    return it->second;
}

LlmHarness::LlmHarness(ModelRegistry registry) : registry_(std::move(registry)) {}

void LlmHarness::bind_provider(const std::string& provider, ProviderFn fn) {
    if (provider.empty() || !fn) throw std::invalid_argument("provider_binding_requires_name_and_function");
    providers_[provider] = std::move(fn);
}

std::string LlmHarness::fingerprint(const std::string& text) { return hex64(fnv1a(text)); }

std::string LlmHarness::canonical_request(const LlmRequest& r) {
    std::ostringstream o;
    o << r.model.provider << '\n' << r.model.model << '\n' << r.model.version << '\n'
      << r.model.endpoint << '\n' << std::setprecision(17)
      << r.config.temperature << '\n' << r.config.top_p << '\n'
      << r.config.max_output_tokens << '\n' << r.config.timeout_ms << '\n'
      << r.config.deterministic << '\n' << r.config.system_prompt_version << '\n'
      << r.observation_fingerprint << '\n' << r.canonical_observation << '\n' << r.run_id;
    return o.str();
}

LlmResponse LlmHarness::invoke(const LlmRequest& input) {
    LlmRequest request = input;
    if (request.model.provider.empty() || request.model.model.empty() || request.model.version.empty()) {
        LlmResponse r; r.status = InvocationStatus::InvalidResponse; r.error = "missing_model_identity";
        records_.push_back({request,r,++sequence_}); return r;
    }
    if (!registry_.contains(request.model.provider, request.model.model, request.model.version)) {
        LlmResponse r; r.status = InvocationStatus::Unavailable; r.error = "model_not_registered";
        records_.push_back({request,r,++sequence_}); return r;
    }
    request.request_fingerprint = fingerprint(canonical_request(request));
    if (auto cached = cache_.get(request.request_fingerprint)) {
        auto r = *cached; r.status = InvocationStatus::CacheHit; r.from_cache = true;
        records_.push_back({request,r,++sequence_}); return r;
    }
    auto st = registry_.status(request.model.provider, request.model.model, request.model.version);
    if (st && st->availability != Availability::Available) {
        LlmResponse r; r.status = InvocationStatus::Unavailable; r.error = st->detail.empty() ? availability_name(st->availability) : st->detail;
        records_.push_back({request,r,++sequence_}); return r;
    }
    auto p = providers_.find(request.model.provider);
    if (p == providers_.end()) {
        LlmResponse r; r.status = InvocationStatus::Unavailable; r.error = "provider_not_bound";
        records_.push_back({request,r,++sequence_}); return r;
    }
    if (budget_.max_requests != 0 && budget_.requests_used >= budget_.max_requests) {
        LlmResponse r; r.status = InvocationStatus::BudgetDenied; r.error = "request_budget_exhausted";
        records_.push_back({request,r,++sequence_}); return r;
    }
    ++budget_.requests_used;
    LlmResponse response;
    try { response = p->second(request); }
    catch (const std::exception& e) { response.status = InvocationStatus::ProviderError; response.error = e.what(); }
    catch (...) { response.status = InvocationStatus::ProviderError; response.error = "unknown_provider_error"; }
    if (response.raw_text.empty() && response.status == InvocationStatus::Executed)
        response.status = InvocationStatus::InvalidResponse;
    if (!response.raw_text.empty()) response.response_fingerprint = fingerprint(response.raw_text);
    budget_.cost_used_usd += response.estimated_cost_usd;
    if (response.status == InvocationStatus::Executed) cache_.put(request.request_fingerprint, response);
    records_.push_back({request,response,++sequence_});
    return response;
}

void LlmHarness::clear_records() { records_.clear(); sequence_ = 0; }

} // namespace coagentics::llm
