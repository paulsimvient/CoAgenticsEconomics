#include "coagentics/analysis/TemporalValidation.hpp"
#include <fstream>
#include <iostream>
int main(int argc,char**argv){auto r=coagentics::analysis::validate_temporal_scientist();auto md=coagentics::analysis::temporal_markdown(r);std::cout<<md;if(argc>1){std::ofstream(std::string(argv[1])+".md")<<md;std::ofstream(std::string(argv[1])+".json")<<coagentics::analysis::temporal_json(r);}return 0;}
