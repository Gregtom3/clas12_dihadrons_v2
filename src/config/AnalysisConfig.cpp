#include "clas12dihadrons/config/AnalysisConfig.hpp"
#include <set>
#include <stdexcept>
#include <string_view>
#include <yaml-cpp/yaml.h>

namespace clas12::dihadron::config {
namespace {
template <typename T>
T required(const YAML::Node& node, const char* key) {
  const auto value = node[key];
  if (!value) throw std::runtime_error(std::string{"missing configuration field: "} + key);
  return value.as<T>();
}
Channel channel(std::string_view value) {
  if (value == "piplus_piplus") return Channel::piplus_piplus;
  if (value == "piminus_piminus") return Channel::piminus_piminus;
  if (value == "piplus_piminus") return Channel::piplus_piminus;
  if (value == "piplus_pi0") return Channel::piplus_pi0;
  if (value == "piminus_pi0") return Channel::piminus_pi0;
  throw std::runtime_error("unsupported channel: " + std::string{value});
}
DatasetKind kind(std::string_view value) {
  if (value == "data") return DatasetKind::data;
  if (value == "monte_carlo") return DatasetKind::monte_carlo;
  throw std::runtime_error("unsupported dataset kind: " + std::string{value});
}
Polarity polarity(std::string_view value) {
  if (value == "inbending") return Polarity::inbending;
  if (value == "outbending") return Polarity::outbending;
  throw std::runtime_error("unsupported polarity: " + std::string{value});
}
RunMode mode(std::string_view value) {
  if (value == "debug") return RunMode::debug;
  if (value == "production") return RunMode::production;
  throw std::runtime_error("unsupported run mode: " + std::string{value});
}
}

AnalysisConfig load(const std::filesystem::path& path) {
  const auto root = YAML::LoadFile(path.string());
  AnalysisConfig config;
  config.schema_version = required<int>(root, "schema_version");

  const auto analysis = root["analysis"];
  if (!analysis) throw std::runtime_error("missing configuration field: analysis");
  config.run_group = required<std::string>(analysis, "run_group");
  for (const auto& item : analysis["channels"])
    config.channels.push_back(channel(item.as<std::string>()));

  for (const auto& item : root["datasets"]) {
    Dataset dataset;
    dataset.id = required<std::string>(item, "id");
    dataset.kind = kind(required<std::string>(item, "kind"));
    dataset.polarity = polarity(required<std::string>(item, "polarity"));
    dataset.beam_energy_gev = required<double>(item, "beam_energy_gev");
    dataset.source_globs = required<std::vector<std::string>>(item, "source_globs");
    config.datasets.push_back(std::move(dataset));
  }

  const auto limits = root["limits"];
  config.limits.mode = mode(required<std::string>(limits, "mode"));
  config.limits.max_files = required<std::size_t>(limits, "max_files");
  config.limits.max_events_per_file = required<std::size_t>(limits, "max_events_per_file");

  const auto paths = root["paths"];
  config.paths.scratch_root = required<std::string>(paths, "scratch_root");
  config.paths.output_root = required<std::string>(paths, "output_root");
  config.paths.log_root = required<std::string>(paths, "log_root");

  const auto scheduler = root["scheduler"];
  config.scheduler.backend = required<std::string>(scheduler, "backend");
  config.scheduler.account = required<std::string>(scheduler, "account");
  config.scheduler.partition = required<std::string>(scheduler, "partition");
  config.scheduler.max_concurrent_jobs = required<std::size_t>(scheduler, "max_concurrent_jobs");

  const auto models = root["photon_models"];
  config.photon_models.policy = required<std::string>(models, "policy");
  config.photon_models.score_threshold = required<double>(models, "score_threshold");

  const auto background = root["neutral_pion_background"];
  config.sideband.method = required<std::string>(background, "method");
  config.sideband.fit_min_gev = required<double>(background, "fit_min_gev");
  config.sideband.fit_max_gev = required<double>(background, "fit_max_gev");
  config.sideband.signal_min_gev = required<double>(background, "signal_min_gev");
  config.sideband.signal_max_gev = required<double>(background, "signal_max_gev");
  config.sideband.signal_model = required<std::string>(background, "signal_model");
  config.sideband.background_model = required<std::string>(background, "background_model");
  config.sideband.background_order = required<int>(background, "background_order");

  validate(config);
  return config;
}

void validate(const AnalysisConfig& config) {
  if (config.schema_version != 1) throw std::runtime_error("schema_version must be 1");
  if (config.run_group != "RGA") throw std::runtime_error("initial release supports run_group RGA only");
  if (config.channels.empty()) throw std::runtime_error("at least one channel is required");
  std::set<std::string> channels;
  for (const auto value : config.channels)
    if (!channels.insert(to_string(value)).second) throw std::runtime_error("duplicate channel");

  if (config.datasets.empty()) throw std::runtime_error("at least one dataset is required");
  std::set<std::string> datasets;
  bool has_data = false, has_mc = false;
  for (const auto& dataset : config.datasets) {
    if (dataset.id.empty() || !datasets.insert(dataset.id).second)
      throw std::runtime_error("dataset ids must be non-empty and unique");
    if (dataset.beam_energy_gev <= 0 || dataset.source_globs.empty())
      throw std::runtime_error("each dataset needs beam energy and source globs");
    has_data |= dataset.kind == DatasetKind::data;
    has_mc |= dataset.kind == DatasetKind::monte_carlo;
  }
  if (!has_data || !has_mc) throw std::runtime_error("RGA analysis requires data and Monte Carlo");

  if (config.limits.mode == RunMode::debug &&
      (config.limits.max_files == 0 || config.limits.max_events_per_file == 0))
    throw std::runtime_error("debug mode requires positive file and event limits");
  if (config.paths.scratch_root.empty() || config.paths.output_root.empty() ||
      config.paths.log_root.empty()) throw std::runtime_error("all paths are required");
  if (config.scheduler.backend != "slurm" && config.scheduler.backend != "local")
    throw std::runtime_error("scheduler backend must be slurm or local");
  if (config.scheduler.backend == "slurm" &&
      (config.scheduler.account.empty() || config.scheduler.partition.empty()))
    throw std::runtime_error("slurm account and partition are required");
  if (config.photon_models.policy != "reuse_or_train" &&
      config.photon_models.policy != "reuse_only" &&
      config.photon_models.policy != "train")
    throw std::runtime_error("unsupported photon model policy");
  if (config.photon_models.score_threshold < 0 || config.photon_models.score_threshold > 1)
    throw std::runtime_error("photon score threshold must be within [0,1]");
  if (config.sideband.method != "sideband")
    throw std::runtime_error("neutral-pion background method must be sideband");
  if (!(config.sideband.fit_min_gev < config.sideband.signal_min_gev &&
        config.sideband.signal_min_gev < config.sideband.signal_max_gev &&
        config.sideband.signal_max_gev < config.sideband.fit_max_gev))
    throw std::runtime_error("sideband mass ranges are not ordered");
  if (config.sideband.background_order < 0)
    throw std::runtime_error("sideband background order cannot be negative");
}

std::string to_string(Channel v) {
  switch (v) {
    case Channel::piplus_piplus: return "piplus_piplus";
    case Channel::piminus_piminus: return "piminus_piminus";
    case Channel::piplus_piminus: return "piplus_piminus";
    case Channel::piplus_pi0: return "piplus_pi0";
    case Channel::piminus_pi0: return "piminus_pi0";
  }
  throw std::runtime_error("invalid channel");
}
std::string to_string(DatasetKind v) { return v == DatasetKind::data ? "data" : "monte_carlo"; }
std::string to_string(Polarity v) { return v == Polarity::inbending ? "inbending" : "outbending"; }
std::string to_string(RunMode v) { return v == RunMode::debug ? "debug" : "production"; }

}  // namespace clas12::dihadron::config
