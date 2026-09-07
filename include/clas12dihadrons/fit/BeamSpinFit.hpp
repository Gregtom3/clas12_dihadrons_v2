#pragma once
#include <array>
#include <cstddef>
#include <vector>

namespace clas12::dihadron::fit {
struct FitEvent { int helicity{}; double polarization{}; double phi_h{}; double phi_R{}; double weight{1}; };
struct FitOptions { std::size_t max_iterations{100}; double tolerance{1e-10}; bool balance_helicity{true}; };
struct FitResult {
  std::array<double,7> amplitudes{};
  std::array<double,7> errors{};
  std::array<std::array<double,7>,7> covariance{};
  double negative_log_likelihood{};
  std::size_t iterations{}, event_count{};
  bool converged{};
};
std::array<double,7> evaluate_modulations(double phi_h,double phi_R);
FitResult fit_beam_spin(const std::vector<FitEvent>& events,const FitOptions& options={});
}

