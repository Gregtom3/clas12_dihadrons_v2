#include "clas12dihadrons/Modulation.hpp"
#include <stdexcept>
#include <string>

namespace clas12::dihadron {
namespace {
constexpr std::array<ModulationDefinition, 7> kDefinitions{{
  {"sin_2phiR_minus_phih", "A", R"(\sin(2\phi_R-\phi_h))", "sin(2*phi_R-phi_h)"},
  {"sin_phiR", "B", R"(\sin(\phi_R))", "sin(phi_R)"},
  {"sin_phih", "C", R"(\sin(\phi_h))", "sin(phi_h)"},
  {"sin_phih_minus_phiR", "D", R"(\sin(\phi_h-\phi_R))", "sin(phi_h-phi_R)"},
  {"sin_2phih_minus_phiR", "E", R"(\sin(2\phi_h-\phi_R))", "sin(2*phi_h-phi_R)"},
  {"sin_2phih_minus_2phiR", "F", R"(\sin(2\phi_h-2\phi_R))", "sin(2*phi_h-2*phi_R)"},
  {"sin_3phih_minus_2phiR", "G", R"(\sin(3\phi_h-2\phi_R))", "sin(3*phi_h-2*phi_R)"}
}};
}

const std::array<ModulationDefinition, 7>& beam_spin_modulations() {
  return kDefinitions;
}

const ModulationDefinition& modulation(std::string_view id) {
  for (const auto& definition : kDefinitions) {
    if (definition.id == id) return definition;
  }
  throw std::invalid_argument("unknown beam-spin modulation: " + std::string{id});
}
}  // namespace clas12::dihadron
