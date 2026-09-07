#include "clas12dihadrons/analysis/CandidateBuilder.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <stdexcept>

namespace clas12::dihadron::analysis {
namespace {
constexpr std::array<ChannelSpec, 5> kSpecs{{
  {config::Channel::piplus_piplus, 211, 211, true, false},
  {config::Channel::piminus_piminus, -211, -211, true, false},
  {config::Channel::piplus_piminus, 211, -211, false, false},
  {config::Channel::piplus_pi0, 211, 111, false, true},
  {config::Channel::piminus_pi0, -211, 111, false, true}
}};

DihadronCandidate candidate(const Event& event, const ChannelSpec& spec,
                            const Particle& first, const Particle& second) {
  return {spec.channel, event.event_id, first.source_index, second.source_index,
          first.momentum + second.momentum};
}
}

FourVector FourVector::operator+(const FourVector& other) const {
  return {px + other.px, py + other.py, pz + other.pz, energy + other.energy};
}

double FourVector::mass_squared() const {
  return energy * energy - px * px - py * py - pz * pz;
}

double FourVector::mass() const {
  // Small negative values can arise from floating-point cancellation.
  return std::sqrt(std::max(0.0, mass_squared()));
}

const ChannelSpec& channel_spec(const config::Channel channel) {
  for (const auto& spec : kSpecs) if (spec.channel == channel) return spec;
  throw std::invalid_argument("unknown dihadron channel");
}

std::vector<DihadronCandidate> build_charged_candidates(
    const Event& event, const std::span<const config::Channel> channels) {
  std::map<int, std::vector<const Particle*>> particles_by_pid;
  for (const auto& particle : event.particles)
    if (particle.pid == 211 || particle.pid == -211)
      particles_by_pid[particle.pid].push_back(&particle);

  std::vector<DihadronCandidate> output;
  for (const auto channel : channels) {
    const auto& spec = channel_spec(channel);
    if (spec.contains_neutral_pion)
      throw std::invalid_argument("neutral channel requires the photon/pi0 builder");

    const auto& first = particles_by_pid[spec.first_pid];
    const auto& second = particles_by_pid[spec.second_pid];
    if (spec.identical_particles) {
      for (std::size_t i = 0; i < first.size(); ++i)
        for (std::size_t j = i + 1; j < first.size(); ++j)
          output.push_back(candidate(event, spec, *first[i], *first[j]));
    } else {
      for (const auto* a : first)
        for (const auto* b : second)
          output.push_back(candidate(event, spec, *a, *b));
    }
  }
  return output;
}

}  // namespace clas12::dihadron::analysis
