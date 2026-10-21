#pragma once
#include "coagentics/analysis/Evidence.hpp"
#include "coagentics/experiment/Dv026MarketLayers.hpp"
#include "coagentics/experiment/Dv026Wave3.hpp"
#include "coagentics/experiment/Dv026Wave4.hpp"
#include <string>
#include <vector>
namespace coagentics::experiment {

enum class PhaseIGroup { MarketMechanics, LlmInterface, MultiAgent, Dv026Qualification };

struct PhaseICheck {
 PhaseIGroup group{};
 std::string id;
 std::string status; // PASS | FAIL | NOT_CLAIMED | PARTIAL
 std::string evidence;
 std::string limitation;
};

struct PhaseISuiteReport {
 std::string version{"phase-i-suite-v1"};
 std::vector<PhaseICheck> checks;
 bool software_green{false};      // all required PASS checks succeeded
 bool darpa_claim_ready{false};   // true only when campaign artifact passes local_ollama_poc gate
 bool phase_ii_deferred{true};
 std::string claim_boundary{
  "Phase I software evidence suite: market environment, LLM interface, multi-agent mediation, "
  "operational measurement wiring. Does not claim Phase I classifier accuracy metrics (FAQ 31) "
  "or Phase II bias/deception/collaboration. darpa_claim_ready may flip only under scope=local_ollama_poc "
  "when Ollama Layer B campaign artifacts pass the readiness gate."};
 analysis::EvidenceStore evidence;
};

// Runs Waves 1–4 APIs under fixed seeds. Deterministic; no network.
PhaseISuiteReport run_phase_i_qualification_suite(std::uint64_t seed=424242);

std::string phase_i_suite_markdown(const PhaseISuiteReport&);
std::string phase_i_suite_json(const PhaseISuiteReport&);
}
