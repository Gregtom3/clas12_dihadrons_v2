#include "clas12dihadrons/provenance/RunManifest.hpp"
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>

namespace clas12::dihadron::provenance {
namespace {
std::string hash_file(const std::filesystem::path& path) {
  std::ifstream input{path, std::ios::binary};
  if (!input) throw std::runtime_error("cannot hash config: " + path.string());
  std::uint64_t hash = 14695981039346656037ull;
  char byte;
  while (input.get(byte)) { hash ^= static_cast<unsigned char>(byte); hash *= 1099511628211ull; }
  std::ostringstream out; out << std::hex << std::setfill('0') << std::setw(16) << hash;
  return "fnv1a64:" + out.str();
}
std::string utc_now() {
  const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::tm value{};
#ifdef _WIN32
  gmtime_s(&value, &now);
#else
  gmtime_r(&now, &value);
#endif
  std::ostringstream out; out << std::put_time(&value, "%Y-%m-%dT%H:%M:%SZ");
  return out.str();
}
std::string environment(const char* name, const char* fallback) {
  if (const auto* value = std::getenv(name)) return value;
  return fallback;
}
}

void write_run_manifest(const config::AnalysisConfig& config,
                        const std::filesystem::path& config_path,
                        const std::filesystem::path& output_path) {
  nlohmann::ordered_json document;
  document["schema_version"] = 1;
  document["created_utc"] = utc_now();
  document["analysis_config"] = config_path.generic_string();
  document["analysis_config_hash"] = hash_file(config_path);
  document["git_commit"] = environment("CLAS12_GIT_COMMIT", "unknown");
  document["host"] = environment("HOSTNAME", environment("COMPUTERNAME", "unknown").c_str());
  document["run_group"] = config.run_group;
  document["mode"] = config::to_string(config.limits.mode);
  document["channels"] = nlohmann::ordered_json::array();
  for (const auto channel : config.channels) document["channels"].push_back(config::to_string(channel));
  document["datasets"] = nlohmann::ordered_json::array();
  for (const auto& dataset : config.datasets) {
    document["datasets"].push_back({
      {"id", dataset.id}, {"kind", config::to_string(dataset.kind)},
      {"polarity", config::to_string(dataset.polarity)},
      {"beam_energy_gev", dataset.beam_energy_gev},
      {"source_globs", dataset.source_globs}});
  }
  document["photon_model_policy"] = config.photon_models.policy;
  document["photon_score_threshold"] = config.photon_models.score_threshold;
  document["neutral_pion_background"] = "sideband";
  document["inputs"] = nlohmann::ordered_json::array();
  document["artifacts"] = nlohmann::ordered_json::array();

  if (!output_path.parent_path().empty()) std::filesystem::create_directories(output_path.parent_path());
  std::ofstream output{output_path};
  if (!output) throw std::runtime_error("cannot write manifest: " + output_path.string());
  output << std::setw(2) << document << '\n';
}
}  // namespace clas12::dihadron::provenance
