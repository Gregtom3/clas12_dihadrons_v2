#pragma once
#include "clas12dihadrons/config/AnalysisConfig.hpp"
#include <cstdint>
#include <span>
#include <vector>

namespace clas12::dihadron::analysis {

struct FourVector {
  double px{};
  double py{};
  double pz{};
  double energy{};

  FourVector operator+(const FourVector& other) const;
  double mass_squared() const;
  double mass() const;
};

struct Particle {
  std::size_t source_index{};
  int pid{};
  FourVector momentum;
};

struct Event {
  std::uint64_t event_id{};
  std::vector<Particle> particles;
};

struct DihadronCandidate {
  config::Channel channel{};
  std::uint64_t event_id{};
  std::size_t first_source_index{};
  std::size_t second_source_index{};
  FourVector momentum;
};

struct ChannelSpec {
  config::Channel channel{};
  int first_pid{};
  int second_pid{};
  bool identical_particles{};
  bool contains_neutral_pion{};
};

const ChannelSpec& channel_spec(config::Channel channel);

// Builds every requested charged channel from one scan of the event particle
// collection. Neutral channels are rejected until the photon/pi0 builder is used.
std::vector<DihadronCandidate> build_charged_candidates(
    const Event& event, std::span<const config::Channel> channels);

}  // namespace clas12::dihadron::analysis
