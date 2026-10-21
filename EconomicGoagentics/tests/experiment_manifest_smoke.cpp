#include "coagentics/experiment/ExperimentManifest.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace coagentics::experiment;
int main(){
 assert(sha256_hex("abc")=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
 LiveCampaignSpec s; s.base_seed=100; s.n_seeds=3; s.min_models=2; s.implementation_revision="test-rev";
 std::vector<coagentics::agents::ModelIdentity> models={{"ollama","model-a","v1","chat-completions"},{"ollama","model-b","v2","chat-completions"}};
 auto m=make_live_campaign_manifest(s,models); auto j=experiment_manifest_canonical_json(m); auto h=experiment_manifest_hash(m);
 assert(h.size()==64); assert(h==experiment_manifest_hash(m));
 assert(j.find("\"seeds\":[100,101,102]")!=std::string::npos);
 assert(j.find("model-a")!=std::string::npos && j.find("model-b")!=std::string::npos);
 assert(j.find("audit_only")!=std::string::npos && j.find("treatment_label")!=std::string::npos);
 assert(j.find("Layer A: allocative efficiency >90%")!=std::string::npos);
 auto path=(std::filesystem::temp_directory_path()/"coagentics_prereg_test.json").string(); write_frozen_manifest(m,path);
 std::ifstream f(path); std::stringstream b;b<<f.rdbuf(); auto out=b.str();
 assert(out.find(h)!=std::string::npos); assert(out.find("\"frozen_before_observation\": true")!=std::string::npos);
 std::filesystem::remove(path); std::cout<<h<<"\n";
}
