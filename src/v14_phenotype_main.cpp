#include "coagentics/analysis/Phenotype.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
int main(){auto r=coagentics::analysis::run_behavioral_phenotyping();std::filesystem::create_directories("results");std::ofstream("results/v14_behavioral_phenotypes.md")<<coagentics::analysis::phenotype_markdown(r);std::ofstream("results/v14_behavioral_phenotypes.json")<<coagentics::analysis::phenotype_json(r);std::cout<<coagentics::analysis::phenotype_markdown(r);}
