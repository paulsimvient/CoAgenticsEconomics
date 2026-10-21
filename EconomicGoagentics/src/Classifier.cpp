#include "coagentics/analysis/Classifier.hpp"
#include <algorithm>
#include <cmath>
namespace coagentics::analysis {
Classification MechanismClassifier::classify(const BehavioralFeatureVector&f)const{double i=std::abs(f.information_sensitivity),p=std::abs(f.peer_sensitivity);Classification c;c.evidence={{"information_sensitivity",i},{"peer_sensitivity",p},{"persistence",f.persistence},{"efficiency_delta",f.efficiency_delta}};if(i<1e-6&&p<1e-6){c.label="information_blind";c.score=1;}else if(i>2*p&&i>1){c.label="direct_information_response";c.score=i/(i+p+1e-9);}else if(p>2*i&&p>1){c.label="peer_propagation";c.score=p/(i+p+1e-9);}else{c.label="mixed_or_adaptive";c.score=std::min(1.0,(i+p)/20.0);}return c;}
}
