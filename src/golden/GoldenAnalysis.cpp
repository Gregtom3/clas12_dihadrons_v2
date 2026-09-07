#include "clas12dihadrons/golden/GoldenAnalysis.hpp"
#include "clas12dihadrons/Modulation.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <set>
#include <sstream>
#include <stdexcept>

namespace clas12::dihadron::golden {
namespace {
bool close_enough(double expected, double actual, double absolute, double relative) {
  const auto scale = std::max(std::abs(expected), std::abs(actual));
  return std::abs(expected - actual) <= absolute + relative * scale;
}
std::string difference(std::string field, double expected, double actual) {
  std::ostringstream out;
  out << std::setprecision(12) << field << ": expected " << expected
      << ", observed " << actual;
  return out.str();
}
void identity(const Snapshot& a, const Snapshot& b, Comparison& out) {
  if (a.schema_version != b.schema_version) out.differences.push_back("schema_version differs");
  const auto check = [&out](std::string name, const std::string& x, const std::string& y) {
    if (x != y) out.differences.push_back(name + ": expected '" + x + "', observed '" + y + "'");
  };
  check("dataset", a.dataset, b.dataset);
  check("channel", a.channel, b.channel);
  check("bin_id", a.bin_id, b.bin_id);
  check("background_method", a.background_method, b.background_method);
}
}

Snapshot load_snapshot(const std::filesystem::path& path) {
  std::ifstream input{path};
  if (!input) throw std::runtime_error("cannot open golden snapshot: " + path.string());
  nlohmann::json document;
  input >> document;
  Snapshot snapshot;
  snapshot.schema_version = document.at("schema_version").get<int>();
  snapshot.dataset = document.at("dataset").get<std::string>();
  snapshot.channel = document.at("channel").get<std::string>();
  snapshot.bin_id = document.at("bin_id").get<std::string>();
  snapshot.background_method = document.at("background_method").get<std::string>();
  snapshot.counts = document.at("counts").get<std::map<std::string, std::uint64_t>>();
  for (const auto& [id, estimate] : document.at("amplitudes").items()) {
    snapshot.amplitudes.emplace(id, Estimate{
      estimate.at("value").get<double>(), estimate.at("uncertainty").get<double>()});
  }
  snapshot.covariance =
    document.at("covariance").get<std::vector<std::vector<double>>>();
  validate_snapshot(snapshot);
  return snapshot;
}

void validate_snapshot(const Snapshot& snapshot) {
  if (snapshot.schema_version != 1) throw std::runtime_error("unsupported snapshot schema");
  if (snapshot.background_method != "none" && snapshot.background_method != "sideband")
    throw std::runtime_error("background_method must be 'none' or 'sideband'");
  if (snapshot.amplitudes.size() != beam_spin_modulations().size())
    throw std::runtime_error("snapshot must contain exactly seven amplitudes");
  for (const auto& definition : beam_spin_modulations())
    if (!snapshot.amplitudes.contains(std::string{definition.id}))
      throw std::runtime_error("missing amplitude: " + std::string{definition.id});
  if (snapshot.covariance.size() != 7)
    throw std::runtime_error("covariance matrix must be 7x7");
  for (const auto& row : snapshot.covariance)
    if (row.size() != 7) throw std::runtime_error("covariance matrix must be 7x7");
}

Comparison compare(const Snapshot& reference, const Snapshot& candidate,
                   const Tolerances& t) {
  validate_snapshot(reference);
  validate_snapshot(candidate);
  Comparison result;
  identity(reference, candidate, result);

  std::set<std::string> count_names;
  for (const auto& [name, unused] : reference.counts) count_names.insert(name);
  for (const auto& [name, unused] : candidate.counts) count_names.insert(name);
  for (const auto& name : count_names) {
    const auto expected = reference.counts.find(name);
    const auto actual = candidate.counts.find(name);
    if (expected == reference.counts.end())
      result.differences.push_back("unexpected count: " + name);
    else if (actual == candidate.counts.end())
      result.differences.push_back("missing count: " + name);
    else if (t.require_exact_counts && expected->second != actual->second)
      result.differences.push_back("count." + name + ": expected " +
        std::to_string(expected->second) + ", observed " + std::to_string(actual->second));
  }

  for (const auto& definition : beam_spin_modulations()) {
    const auto id = std::string{definition.id};
    const auto& expected = reference.amplitudes.at(id);
    const auto& actual = candidate.amplitudes.at(id);
    if (!close_enough(expected.value, actual.value,
                      t.amplitude_absolute, t.amplitude_relative))
      result.differences.push_back(difference("amplitude." + id, expected.value, actual.value));
    if (!close_enough(expected.uncertainty, actual.uncertainty,
                      t.uncertainty_absolute, t.uncertainty_relative))
      result.differences.push_back(difference("uncertainty." + id,
                                              expected.uncertainty, actual.uncertainty));
  }

  for (std::size_t row = 0; row < 7; ++row)
    for (std::size_t column = 0; column < 7; ++column)
      if (!close_enough(reference.covariance[row][column],
                        candidate.covariance[row][column],
                        t.covariance_absolute, t.covariance_relative))
        result.differences.push_back(difference(
          "covariance[" + std::to_string(row) + "][" + std::to_string(column) + "]",
          reference.covariance[row][column], candidate.covariance[row][column]));

  result.passed = result.differences.empty();
  return result;
}

void print_report(const Comparison& comparison, std::ostream& output) {
  if (comparison.passed) {
    output << "GOLDEN ANALYSIS: PASS\n";
    return;
  }
  output << "GOLDEN ANALYSIS: FAIL (" << comparison.differences.size()
         << " differences)\n";
  for (const auto& item : comparison.differences) output << "  - " << item << '\n';
}
}  // namespace clas12::dihadron::golden
