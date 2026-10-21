#pragma once
#include <optional>
#include <string>
#include <vector>
namespace coagentics::util {

struct HttpResponse {
 int status{0};
 std::string body;
 std::string error;
};

/** GET via libcurl (no shell). Empty body on transport failure; check error/status. */
HttpResponse http_get(const std::string& url, int timeout_s=5);

/** POST JSON via libcurl (no shell). */
HttpResponse http_post_json(const std::string& url, const std::string& json_body,
 const std::string& bearer_token, int timeout_s=60);

}
