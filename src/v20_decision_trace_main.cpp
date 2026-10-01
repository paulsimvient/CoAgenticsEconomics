#include "coagentics/analysis/DecisionTraceValidation.hpp"
#include <iostream>
int main(){auto r=coagentics::analysis::validate_decision_traces();std::cout<<coagentics::analysis::trace_validation_markdown(r);}
