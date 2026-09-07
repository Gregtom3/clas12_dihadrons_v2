#pragma once
#include "clas12dihadrons/config/AnalysisConfig.hpp"
#include <filesystem>

namespace clas12::dihadron::provenance {
void write_run_manifest(const config::AnalysisConfig& config,
                        const std::filesystem::path& config_path,
                        const std::filesystem::path& output_path);
}  // namespace clas12::dihadron::provenance
