#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#if defined(_WIN32)
#error "dv026_workbench_runner_smoke is POSIX-oriented"
#endif
#include <array>
#include <cstdio>

static std::string shell_quote(const std::string& s){
 std::string o="'";
 for(char c:s){ if(c=='\'') o+="'\\''"; else o+=c; }
 o+="'";
 return o;
}

static std::string run_capture(const std::string& cmd){
 std::array<char, 4096> buf{};
 std::string out;
 FILE* pipe=popen(cmd.c_str(), "r");
 if(!pipe) throw std::runtime_error("popen failed");
 while(fgets(buf.data(), static_cast<int>(buf.size()), pipe)) out+=buf.data();
 int rc=pclose(pipe);
 if(rc!=0) throw std::runtime_error("runner exit "+std::to_string(rc)+" output="+out.substr(0,200));
 return out;
}

static void require(bool ok, const std::string& msg){
 if(!ok) throw std::runtime_error(msg);
}

int main(){
 try{
  const char* env=std::getenv("COAGENTICS_DV026_RUNNER");
  std::string exe=env && *env ? std::string(env) : "./dv026-workbench-runner";
  std::string out=run_capture(shell_quote(exe)+" bundle 424242");
  require(out.find("\"command\":\"bundle\"")!=std::string::npos, "command");
  require(out.find("\"phase_i\"")!=std::string::npos, "phase_i");
  require(out.find("\"phase_ii\"")!=std::string::npos, "phase_ii");
  require(out.find("\"layer_a\"")!=std::string::npos, "layer_a");
  require(out.find("\"ten_llm\"")!=std::string::npos, "ten_llm");
  require(out.find("\"llm_slice\"")!=std::string::npos, "llm_slice");
  require(out.find("\"population\"")!=std::string::npos, "population");
  require(out.find("\"adaptive\"")!=std::string::npos, "adaptive");
  require(out.find("\"live_llm\":false")!=std::string::npos, "live_llm");
  require(out.find("\"constructs_validated\":false")!=std::string::npos, "constructs");
  // Bundle is still scripted for nested experiments; darpa_claim_ready follows Phase I
  // campaign artifacts when present (local_ollama_poc), else false.
  const bool ready=out.find("\"darpa_claim_ready\":true")!=std::string::npos;
  const bool not_ready=out.find("\"darpa_claim_ready\":false")!=std::string::npos;
  require(ready || not_ready, "darpa_claim_ready present");
  if(ready) require(out.find("local_ollama_poc")!=std::string::npos, "ready implies local_ollama_poc");
  std::cout<<"dv026_workbench_runner_smoke ok darpa_claim_ready="<<(ready?"true":"false")<<"\n";
  return 0;
 }catch(const std::exception& e){
  std::cerr<<"dv026_workbench_runner_smoke FAIL: "<<e.what()<<"\n";
  return 1;
 }
}
