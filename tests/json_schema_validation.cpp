#include "coagentics/experiment/LlmExperiment.hpp"
#include "coagentics/util/Json.hpp"
#include <cstdlib>
#include <iostream>
using namespace coagentics::experiment;
static void req(bool x,const char*m){if(!x){std::cerr<<m<<"\n";std::exit(1);}}
int main(){
 req(coagentics::util::parse_json(R"({"x":"a\\\"b","n":1.5,"a":[true,null]})").has_value(),"valid json");
 req(!coagentics::util::parse_json(R"({"x":1,})").has_value(),"reject trailing comma");
 auto ok=parse_canonical_market_action(R"({"action":"BUY","asset":"A","quantity":1,"price":82.0,"time":5})");req(ok.ok,"valid action");
 req(parse_canonical_market_action(R"({"action":"BUY","asset":"A","quantity":"1","price":82,"time":5})").error=="quantity_not_integer","typed quantity");
 req(parse_canonical_market_action(R"({"action":"BUY","asset":"A","quantity":1,"price":"82","time":5})").error=="price_wrong_type","typed price");
 req(parse_canonical_market_action(R"({"action":"BUY","asset":"A","quantity":1,"price":82,"time":"5"})").error=="time_wrong_type","typed time");
 req(parse_canonical_market_action(R"({"action":"BUY","asset":"A","quantity":1,"price":82,"time":5,})").error=="invalid_json","strict syntax");
 req(parse_canonical_market_action(R"([{"action":"HOLD"}])").error=="schema_root_not_object","root schema");
 std::cout<<"json/schema validation PASS\n";
}
