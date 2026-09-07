#pragma once
#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <map>
#include <string>
#include <vector>

namespace clas12::dihadron::golden {
struct Estimate { double value{}; double uncertainty{}; };
struct Snapshot {
  int schema_version{};
  std::string dataset;
  std::string channel;
  std::string bin_id;
  std::string background_method;
  std::map<std::string, std::uint64_t> counts;
  std::map<std::string, Estimate> amplitudes;
  std::vector<std::vector<double>> covariance;
};
struct Tolerances {
  double amplitude_absolute{1e-6};
  double amplitude_relative{1e-4};
  double uncertainty_absolute{1e-6};
  double uncertainty_relative{1e-4};
  double covariance_absolute{1e-8};
  double covariance_relative{1e-4};
  bool require_exact_counts{true};
};
struct Comparison { bool passed{true}; std::vector<std::string> differences; };

Snapshot load_snapshot(const std::filesystem::path& path);
void validate_snapshot(const Snapshot& snapshot);
Comparison compare(const Snapshot& reference, const Snapshot& candidate,
                   const Tolerances& tolerances);
void print_report(const Comparison& comparison, std::ostream& output);
}  // namespace clas12::dihadron::golden
