#include "coagentics/util/Http.hpp"
#include <curl/curl.h>
#include <mutex>
namespace coagentics::util {
namespace {
std::once_flag curl_once;
void ensure_curl(){ std::call_once(curl_once, []{ curl_global_init(CURL_GLOBAL_DEFAULT); }); }
size_t write_cb(char* ptr, size_t size, size_t nmemb, void* userdata){
 auto* out=static_cast<std::string*>(userdata);
 out->append(ptr, size*nmemb);
 return size*nmemb;
}
}
HttpResponse http_get(const std::string& url, int timeout_s){
 ensure_curl();
 HttpResponse r;
 CURL* curl=curl_easy_init();
 if(!curl){ r.error="curl_init_failed"; return r; }
 curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
 curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
 curl_easy_setopt(curl, CURLOPT_WRITEDATA, &r.body);
 curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(timeout_s));
 curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
 curl_easy_setopt(curl, CURLOPT_USERAGENT, "coagentics-dv026/1.0");
 const CURLcode code=curl_easy_perform(curl);
 if(code!=CURLE_OK){
  r.error=curl_easy_strerror(code);
  curl_easy_cleanup(curl);
  return r;
 }
 long status=0;
 curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
 r.status=static_cast<int>(status);
 curl_easy_cleanup(curl);
 return r;
}
HttpResponse http_post_json(const std::string& url, const std::string& json_body,
 const std::string& bearer_token, int timeout_s){
 ensure_curl();
 HttpResponse r;
 CURL* curl=curl_easy_init();
 if(!curl){ r.error="curl_init_failed"; return r; }
 struct curl_slist* headers=nullptr;
 headers=curl_slist_append(headers, "Content-Type: application/json");
 if(!bearer_token.empty()){
  const std::string auth="Authorization: Bearer "+bearer_token;
  headers=curl_slist_append(headers, auth.c_str());
 }
 curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
 curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
 curl_easy_setopt(curl, CURLOPT_POST, 1L);
 curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body.c_str());
 curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(json_body.size()));
 curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
 curl_easy_setopt(curl, CURLOPT_WRITEDATA, &r.body);
 curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(timeout_s));
 curl_easy_setopt(curl, CURLOPT_USERAGENT, "coagentics-dv026/1.0");
 const CURLcode code=curl_easy_perform(curl);
 curl_slist_free_all(headers);
 if(code!=CURLE_OK){
  r.error=curl_easy_strerror(code);
  curl_easy_cleanup(curl);
  return r;
 }
 long status=0;
 curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
 r.status=static_cast<int>(status);
 curl_easy_cleanup(curl);
 return r;
}
}
