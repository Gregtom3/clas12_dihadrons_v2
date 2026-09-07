#include "clas12dihadrons/analysis/CandidateBuilder.hpp"
#include <array>
#include <cmath>
#include <iostream>
#include <set>
#include <string>

using clas12::dihadron::analysis::Event;
using clas12::dihadron::analysis::FourVector;
using clas12::dihadron::analysis::Particle;
using clas12::dihadron::analysis::build_charged_candidates;
using clas12::dihadron::config::Channel;

namespace {
int failures = 0;
void check(bool condition, const std::string& message) {
  if (!condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
}
Particle pion(std::size_t index, int pid, double px) {
  constexpr double mass = 0.13957039;
  return {index, pid, {px, 0, 0, std::sqrt(px * px + mass * mass)}};
}
}

int main() {
  Event event{42, {
    pion(10, 211, 0.3), pion(11, 211, -0.2),
    pion(20, -211, 0.4), pion(21, -211, -0.1),
    Particle{30, 22, {0, 0, 1, 1}}
  }};
  constexpr std::array channels{
    Channel::piplus_piplus, Channel::piminus_piminus, Channel::piplus_piminus};
  const auto candidates = build_charged_candidates(event, channels);

  check(candidates.size() == 6, "2 pi+ and 2 pi- produce 1 ++, 1 --, and 4 +- candidates");
  std::size_t pp = 0, mm = 0, pm = 0;
  std::set<std::pair<std::size_t, std::size_t>> same_charge_pairs;
  for (const auto& item : candidates) {
    check(item.event_id == 42, "event identity is retained");
    check(item.first_source_index != item.second_source_index, "a particle is never paired with itself");
    check(item.momentum.mass() >= 0, "invariant mass is finite and non-negative");
    if (item.channel == Channel::piplus_piplus) {
      ++pp; same_charge_pairs.emplace(item.first_source_index, item.second_source_index);
    } else if (item.channel == Channel::piminus_piminus) {
      ++mm; same_charge_pairs.emplace(item.first_source_index, item.second_source_index);
    } else if (item.channel == Channel::piplus_piminus) ++pm;
  }
  check(pp == 1 && mm == 1 && pm == 4, "candidate counts are channel correct");
  check(same_charge_pairs.size() == 2, "identical pairs are unique and unordered");

  bool rejected_neutral = false;
  try {
    constexpr std::array neutral{Channel::piplus_pi0};
    static_cast<void>(build_charged_candidates(event, neutral));
  } catch (const std::invalid_argument&) { rejected_neutral = true; }
  check(rejected_neutral, "neutral channels cannot silently use the charged builder");

  if (failures == 0) std::cout << "candidate-builder: PASS\n";
  return failures == 0 ? 0 : 1;
}
