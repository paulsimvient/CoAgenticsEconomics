#pragma once
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>
namespace coagentics::util {
struct Json {
 using object=std::map<std::string,Json>; using array=std::vector<Json>;
 using value=std::variant<std::nullptr_t,bool,double,std::string,array,object>;
 value v{nullptr};
 bool is_null()const{return std::holds_alternative<std::nullptr_t>(v);} bool is_bool()const{return std::holds_alternative<bool>(v);}
 bool is_number()const{return std::holds_alternative<double>(v);} bool is_string()const{return std::holds_alternative<std::string>(v);}
 bool is_array()const{return std::holds_alternative<array>(v);} bool is_object()const{return std::holds_alternative<object>(v);}
 const object& as_object()const{return std::get<object>(v);} const array& as_array()const{return std::get<array>(v);}
 const std::string& as_string()const{return std::get<std::string>(v);} double as_number()const{return std::get<double>(v);} bool as_bool()const{return std::get<bool>(v);}
 const Json* get(const std::string& k)const{if(!is_object())return nullptr;auto&i=as_object();auto it=i.find(k);return it==i.end()?nullptr:&it->second;}
};
class JsonParser { const std::string&s; size_t p=0; std::string err;
 void ws(){while(p<s.size()&&std::isspace((unsigned char)s[p]))++p;} bool eat(char c){ws();if(p<s.size()&&s[p]==c){++p;return true;}return false;}
 bool lit(const char*x){ws();size_t n=std::char_traits<char>::length(x);if(s.compare(p,n,x)==0){p+=n;return true;}return false;}
 std::optional<std::string> str(){ws();if(p>=s.size()||s[p++]!='"')return {};std::string o;while(p<s.size()){char c=s[p++];if(c=='"')return o;if((unsigned char)c<0x20)return {};if(c!='\\'){o+=c;continue;}if(p>=s.size())return {};char e=s[p++];switch(e){case '"':o+='"';break;case '\\':o+='\\';break;case '/':o+='/';break;case 'b':o+='\b';break;case 'f':o+='\f';break;case 'n':o+='\n';break;case 'r':o+='\r';break;case 't':o+='\t';break;case 'u':{if(p+4>s.size())return {};unsigned cp=0;for(int i=0;i<4;i++){char h=s[p++];cp*=16;if(h>='0'&&h<='9')cp+=h-'0';else if(h>='a'&&h<='f')cp+=h-'a'+10;else if(h>='A'&&h<='F')cp+=h-'A'+10;else return {};}if(cp<=0x7f)o+=(char)cp;else if(cp<=0x7ff){o+=(char)(0xc0|(cp>>6));o+=(char)(0x80|(cp&63));}else{o+=(char)(0xe0|(cp>>12));o+=(char)(0x80|((cp>>6)&63));o+=(char)(0x80|(cp&63));}break;}default:return {};}}return {};}
 std::optional<Json> val(){ws();if(p>=s.size())return {};if(s[p]=='"'){auto x=str();if(!x)return {};return Json{*x};}if(s[p]=='{'){++p;Json::object o;ws();if(eat('}'))return Json{o};while(true){auto k=str();if(!k||!eat(':'))return {};auto x=val();if(!x)return {};if(!o.emplace(*k,*x).second)return {};if(eat('}'))break;if(!eat(','))return {};}return Json{o};}if(s[p]=='['){++p;Json::array a;ws();if(eat(']'))return Json{a};while(true){auto x=val();if(!x)return {};a.push_back(*x);if(eat(']'))break;if(!eat(','))return {};}return Json{a};}if(lit("true"))return Json{true};if(lit("false"))return Json{false};if(lit("null"))return Json{nullptr};size_t b=p;if(s[p]=='-')++p;if(p>=s.size())return {};if(s[p]=='0')++p;else{if(!std::isdigit((unsigned char)s[p]))return {};while(p<s.size()&&std::isdigit((unsigned char)s[p]))++p;}if(p<s.size()&&s[p]=='.'){++p;if(p>=s.size()||!std::isdigit((unsigned char)s[p]))return {};while(p<s.size()&&std::isdigit((unsigned char)s[p]))++p;}if(p<s.size()&&(s[p]=='e'||s[p]=='E')){++p;if(p<s.size()&&(s[p]=='+'||s[p]=='-'))++p;if(p>=s.size()||!std::isdigit((unsigned char)s[p]))return {};while(p<s.size()&&std::isdigit((unsigned char)s[p]))++p;}try{double d=std::stod(s.substr(b,p-b));if(!std::isfinite(d))return {};return Json{d};}catch(...){return {};}}
public: explicit JsonParser(const std::string&x):s(x){} std::optional<Json> parse(){auto x=val();ws();if(!x||p!=s.size())return {};return x;}
};
inline std::optional<Json> parse_json(const std::string&s){return JsonParser(s).parse();}

/** Canonical JSON string escape for evidence / request payloads (control chars included). */
inline std::string json_escape(const std::string& s){
 std::string o; o.reserve(s.size()+8);
 for(unsigned char c:s){
  switch(c){
   case '"': o+="\\\""; break;
   case '\\': o+="\\\\"; break;
   case '\n': o+="\\n"; break;
   case '\r': o+="\\r"; break;
   case '\t': o+="\\t"; break;
   default:
    if(c<0x20){
     char buf[8];
     std::snprintf(buf,sizeof(buf),"\\u%04x",(unsigned)c);
     o+=buf;
    }else o.push_back(static_cast<char>(c));
  }
 }
 return o;
}
}
