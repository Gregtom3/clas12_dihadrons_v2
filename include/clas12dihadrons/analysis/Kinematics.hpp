#pragma once
#include "clas12dihadrons/analysis/CandidateBuilder.hpp"

namespace clas12::dihadron::analysis {

struct DihadronKinematics {
  double Q2{}, x{}, y{}, W{};
  double Mh{}, z{}, z1{}, z2{};
  double xF{}, xF1{}, xF2{};
  double pT{}, pT1{}, pT2{};
  double phi_h{}, phi_R{};
  double missing_mass{};
  double P1{}, P2{};
};

DihadronKinematics compute_dihadron_kinematics(
    double beam_energy_gev, const FourVector& electron,
    const FourVector& first_hadron, const FourVector& second_hadron);

bool passes_pippim_analysis_cuts(const DihadronKinematics& value);

}  // namespace clas12::dihadron::analysis

