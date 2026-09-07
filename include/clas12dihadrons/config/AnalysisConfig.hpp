#pragma once
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace clas12::dihadron::config {

enum class Channel { piplus_piplus, piminus_piminus, piplus_piminus, piplus_pi0, piminus_pi0 };
enum class DatasetKind { data, monte_carlo };
enum class Polarity { inbending, outbending };
enum class RunMode { debug, production };

struct Dataset {
  std::string id;
  DatasetKind kind{};
  Polarity polarity{};
  double beam_energy_gev{};
  std::vector<std::string> source_globs;
};

struct Limits {
  RunMode mode{RunMode::debug};
  std::size_t max_files{};
  std::size_t max_events_per_file{};
};

struct Paths {
  std::string scratch_root;
  std::string output_root;
  std::string log_root;
};

struct Scheduler {
  std::string backend{"slurm"};
  std::string account;
  std::string partition;
  std::size_t max_concurrent_jobs{100};
};

struct PhotonModels {
  std::string policy{"reuse_or_train"};
  double score_threshold{0.9};
};

struct Sideband {
  std::string method{"sideband"};
  double fit_min_gev{};
  double fit_max_gev{};
  double signal_min_gev{};
  double signal_max_gev{};
  std::string signal_model;
  std::string background_model;
  int background_order{};
};

struct AnalysisConfig {
  int schema_version{};
  std::string run_group;
  std::vector<Channel> channels;
  std::vector<Dataset> datasets;
  Limits limits;
  Paths paths;
  Scheduler scheduler;
  PhotonModels photon_models;
  Sideband sideband;
};

AnalysisConfig load(const std::filesystem::path& path);
void validate(const AnalysisConfig& config);
std::string to_string(Channel value);
std::string to_string(DatasetKind value);
std::string to_string(Polarity value);
std::string to_string(RunMode value);

}  // namespace clas12::dihadron::config
