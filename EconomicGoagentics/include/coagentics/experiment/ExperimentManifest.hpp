#pragma once
#include "coagentics/experiment/Dv026LiveCampaign.hpp"
#include <cstdint>
#include <string>
#include <vector>
namespace coagentics::experiment {
struct ExperimentManifest {
 std::string schema_version{"dv026-preregistration-v1"};
 std::string experiment_id{"dv026-ollama-layer-b-campaign"};
 std::string implementation_revision;
 std::uint64_t created_from_seed{};
 std::string mechanism{"continuous_double_auction"};
 std::string population_design{"reference_counterparty"};
 std::string activation_design{"sequential_interaction"};
 std::vector<agents::ModelIdentity> models;
 std::vector<std::uint64_t> seeds;
 std::vector<std::string> treatments{"baseline"};
 std::vector<std::string> model_visible_fields;
 std::vector<std::string> audit_only_fields;
 double temperature{0.0};
 bool inference_seed_requested{true};
 std::vector<std::string> hypotheses;
 std::vector<std::string> metrics;
 std::vector<std::string> acceptance_rules;
 std::string claim_boundary;
 std::string research_expectations_sha256;
 std::string research_expectations_path;
 std::string research_expectations_json;
};
ExperimentManifest make_live_campaign_manifest(const LiveCampaignSpec&, const std::vector<agents::ModelIdentity>&);
std::string experiment_manifest_canonical_json(const ExperimentManifest&);
std::string sha256_hex(const std::string&);
std::string experiment_manifest_hash(const ExperimentManifest&);
void write_frozen_manifest(const ExperimentManifest&, const std::string& path);
}
