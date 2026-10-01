#pragma once
#include "coagentics/experiment/LlmExperiment.hpp"
#include "coagentics/experiment/Scientist.hpp"
#include <cstdint>
#include <string>
#include <vector>
namespace coagentics::experiment {

// --- Adaptive discovery / confirmation ------------------------------------
// Discovery: prior-weighted probe selection until SEPARATED or budget exhausted.
// Confirmation: held-out seeds re-check that the leading candidate remains favored.
// Posteriors are model-conditional; not empirical Phase I/II claims.

struct ConfirmationConfig {
 std::size_t heldout_seeds{3};
 double min_posterior{0.80}; // confirmation threshold (may differ from discovery stop)
};

struct AdaptiveDiscoveryConfirmationReport {
 ScientistReport discovery;
 bool discovery_separated{false};
 std::string leading_hypothesis;
 std::vector<ScientistReport> confirmation_runs;
 std::size_t confirmation_agree{0};
 bool confirmed{false};
 std::string status;
 std::string claim_boundary{
  "Wave 4 adaptive discovery/confirmation: model-conditional controlled-probe scaffolding only. "
  "Does NOT select DV026 market/LLM interventions (CDA treatments, news, peer visibility). "
  "Does not claim Phase I classifier accuracy or Phase II behavioral constructs."};
 analysis::EvidenceStore evidence;
};

AdaptiveDiscoveryConfirmationReport run_adaptive_discovery_confirmation(
 const ExperimentalDomain& domain,
 const std::vector<Candidate>& candidates,
 const std::vector<Probe>& probes,
 std::uint64_t discovery_seed,
 const ScientistConfig& discovery_cfg=ScientistConfig{},
 const ConfirmationConfig& confirm_cfg=ConfirmationConfig{});

// --- Captured-provider transcript replay + cross-event validation ---------
// NOT historical real-world data replay (no cutoff-T / available_at / EIA/FRED).
// Capture raw provider responses from a completed LLM market run, then replay
// them through ReplayTransport under the same ExperimentSpec/RunSpec seed.

struct CapturedTurnEvent {
 std::uint64_t time{};
 std::string agent_id;
 std::string request_id;
 std::string raw_provider_response;
 std::optional<MarketAction> original_parsed_action;
 int original_filled_quantity{0};
 bool original_parse_ok{false};
 bool original_action_valid{false};
};

// Canonical name. HistoricalReplayLog retained as alias for API compatibility.
struct CapturedProviderTranscriptLog {
 ExperimentSpec experiment;
 RunSpec run_template; // transport cleared; model/seed retained
 std::vector<CapturedTurnEvent> events;
 std::string claim_boundary{
  "Captured-provider transcript replay reproduces stored provider payloads. "
  "This is NOT historical market-data replay with cutoff-T / available_at filtering."};
};
using HistoricalReplayLog = CapturedProviderTranscriptLog;

CapturedProviderTranscriptLog capture_provider_transcript_log(const RunResult& original,
 const ExperimentSpec& experiment, const RunSpec& run);
// Deprecated name — same as capture_provider_transcript_log.
inline HistoricalReplayLog capture_historical_replay_log(const RunResult& original,
 const ExperimentSpec& experiment, const RunSpec& run){
 return capture_provider_transcript_log(original, experiment, run);
}

struct EventValidation {
 std::uint64_t time{};
 std::string agent_id;
 bool raw_match{false};
 bool parse_match{false};
 bool fill_match{false};
 std::string detail;
};

struct CapturedProviderReplayReport {
 CapturedProviderTranscriptLog log;
 RunResult replayed;
 std::vector<EventValidation> events;
 bool byte_identical_raw{false};
 bool outcome_match{false}; // trades + fills + efficiency
 bool cross_event_valid{false}; // every captured event matches on replay
 std::string claim_boundary{
  "Cross-event validation checks captured vs replayed turns only. "
  "Does not claim live multi-provider economic performance or historical-data replay."};
 analysis::EvidenceStore evidence;
};
using HistoricalReplayReport = CapturedProviderReplayReport;

CapturedProviderReplayReport replay_and_validate(const CapturedProviderTranscriptLog& log);

std::string adaptive_discovery_json(const AdaptiveDiscoveryConfirmationReport&);
std::string historical_replay_json(const HistoricalReplayReport&); // serializes captured-provider replay
std::string captured_provider_replay_json(const CapturedProviderReplayReport&);
}
