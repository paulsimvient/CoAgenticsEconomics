#pragma once
#include <string>
#include <vector>
namespace coagentics::analysis {
struct CommunicationTrial { unsigned seed{}; bool message_visible{}, private_signal{}, incentive_conflict{}; double statement{}, sender_belief{}, receiver_before{}, receiver_after{}; };
struct InfluenceEvidence { double visibility_effect{}, conflict_effect{}; unsigned paired_trials{}, observed_misrepresentations{}; bool intent_identifiable{}; };
InfluenceEvidence analyze_communication(const std::vector<CommunicationTrial>& control, const std::vector<CommunicationTrial>& treatment, double tolerance=1e-9);
struct NetworkEdge { unsigned from{}, to{}; double weight{}; };
struct PropagationResult { std::vector<double> responses; double reach{}, mean_response{}; };
PropagationResult propagate(unsigned agents, const std::vector<NetworkEdge>& edges, unsigned origin, double impulse, unsigned steps, double retention=0.0);
}
