#pragma once
#include <array>
#include <string_view>

namespace clas12::dihadron {

struct ModulationDefinition {
  std::string_view id;
  std::string_view legacy_label;
  std::string_view latex;
  std::string_view angular_expression;
};

// Stable order preserving the legacy A-G mapping produced by string sorting.
// Physics code, results, and plots must use id; legacy_label is display-only.
const std::array<ModulationDefinition, 7>& beam_spin_modulations();
const ModulationDefinition& modulation(std::string_view id);

}  // namespace clas12::dihadron
