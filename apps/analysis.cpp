#include "clas12dihadrons/config/AnalysisConfig.hpp"
#include "clas12dihadrons/provenance/RunManifest.hpp"
#include <exception>
#include <filesystem>
#include <iostream>
#include <string_view>

namespace {
void usage() {
  std::cerr << "Usage:\n"
            << "  clas12-analysis validate CONFIG.yaml\n"
            << "  clas12-analysis manifest CONFIG.yaml OUTPUT.json\n";
}
}

int main(int argc, char** argv) {
  if (argc < 3) { usage(); return 2; }
  try {
    const std::string_view command{argv[1]};
    const std::filesystem::path config_path{argv[2]};
    const auto config = clas12::dihadron::config::load(config_path);
    if (command == "validate") {
      if (argc != 3) { usage(); return 2; }
      std::cout << "CONFIGURATION: VALID\n"
                << "  run group: " << config.run_group << '\n'
                << "  channels: " << config.channels.size() << '\n'
                << "  datasets: " << config.datasets.size() << '\n'
                << "  mode: " << clas12::dihadron::config::to_string(config.limits.mode) << '\n'
                << "  neutral-pion background: sideband\n";
      return 0;
    }
    if (command == "manifest") {
      if (argc != 4) { usage(); return 2; }
      clas12::dihadron::provenance::write_run_manifest(config, config_path, argv[3]);
      std::cout << "WROTE MANIFEST: " << argv[3] << '\n';
      return 0;
    }
    usage();
    return 2;
  } catch (const std::exception& error) {
    std::cerr << "clas12-analysis: " << error.what() << '\n';
    return 1;
  }
}
