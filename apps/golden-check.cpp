#include "clas12dihadrons/golden/GoldenAnalysis.hpp"
#include <exception>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
void usage() {
  std::cerr << "Usage: clas12-golden-check --reference FILE --candidate FILE "
               "[--amplitude-abs N] [--amplitude-rel N] "
               "[--uncertainty-abs N] [--uncertainty-rel N] "
               "[--covariance-abs N] [--covariance-rel N]\n";
}
double number(const char* value, std::string_view option) {
  try { return std::stod(value); }
  catch (...) { throw std::runtime_error("invalid value for " + std::string{option}); }
}
}

int main(int argc, char** argv) {
  std::filesystem::path reference;
  std::filesystem::path candidate;
  clas12::dihadron::golden::Tolerances tolerance;
  try {
    for (int i = 1; i < argc; ++i) {
      const std::string_view option{argv[i]};
      if (i + 1 >= argc) throw std::runtime_error("missing value for " + std::string{option});
      const char* value = argv[++i];
      if (option == "--reference") reference = value;
      else if (option == "--candidate") candidate = value;
      else if (option == "--amplitude-abs") tolerance.amplitude_absolute = number(value, option);
      else if (option == "--amplitude-rel") tolerance.amplitude_relative = number(value, option);
      else if (option == "--uncertainty-abs") tolerance.uncertainty_absolute = number(value, option);
      else if (option == "--uncertainty-rel") tolerance.uncertainty_relative = number(value, option);
      else if (option == "--covariance-abs") tolerance.covariance_absolute = number(value, option);
      else if (option == "--covariance-rel") tolerance.covariance_relative = number(value, option);
      else throw std::runtime_error("unknown option: " + std::string{option});
    }
    if (reference.empty() || candidate.empty()) { usage(); return 2; }
    const auto result = clas12::dihadron::golden::compare(
      clas12::dihadron::golden::load_snapshot(reference),
      clas12::dihadron::golden::load_snapshot(candidate), tolerance);
    clas12::dihadron::golden::print_report(result, std::cout);
    return result.passed ? 0 : 1;
  } catch (const std::exception& error) {
    std::cerr << "golden-check: " << error.what() << '\n';
    usage();
    return 2;
  }
}
