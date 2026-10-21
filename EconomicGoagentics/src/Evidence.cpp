#include "coagentics/analysis/Evidence.hpp"
#include <sstream>
namespace coagentics::analysis {
static std::string esc(const std::string&s){std::string o;for(char c:s){if(c=='"'||c=='\\')o+='\\';o+=c;}return o;}
std::string EvidenceStore::jsonl()const{std::ostringstream os;for(const auto&r:records_){os<<"{\"experiment_id\":\""<<esc(r.experiment_id)<<"\",\"control_run_id\":\""<<esc(r.control_run_id)<<"\",\"treatment_run_id\":\""<<esc(r.treatment_run_id)<<"\",\"seed\":"<<r.seed<<",\"hypothesis_id\":\""<<esc(r.evidence.hypothesis_id)<<"\",\"discriminator_id\":\""<<esc(r.evidence.discriminator_id)<<"\",\"direction\":\""<<to_string(r.evidence.direction)<<"\",\"targeted_mean_abs_shift\":"<<r.evidence.targeted_mean_abs_shift<<",\"non_target_mean_abs_shift\":"<<r.evidence.non_target_mean_abs_shift<<",\"propagation_ratio\":"<<r.evidence.propagation_ratio<<",\"rationale\":\""<<esc(r.evidence.rationale)<<"\"}\n";}return os.str();}
}
