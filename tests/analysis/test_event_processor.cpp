#include "clas12dihadrons/analysis/EventProcessor.hpp"
#include <array>
#include <cmath>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

using namespace clas12::dihadron;
using namespace clas12::dihadron::analysis;

namespace {
int failures = 0;
void check(bool condition, const std::string& message) {
  if (!condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
}
Particle pion(std::size_t index, int pid, double px) {
  constexpr double m = 0.13957039;
  return {index, pid, {px, 0, 0, std::sqrt(px * px + m * m)}};
}
class VectorSource final : public EventSource {
 public:
  explicit VectorSource(std::vector<Event> events) : events_(std::move(events)) {}
  bool next(Event& event) override {
    if (index_ == events_.size()) return false;
    event = events_[index_++];
    return true;
  }
  std::string_view description() const override { return "synthetic.hipo"; }
 private:
  std::vector<Event> events_;
  std::size_t index_{};
};
class OddEventSelector final : public EventSelector {
 public:
  bool accept(Event& event) override { return event.event_id % 2 == 1; }
};
class RecordingSink final : public CandidateSink {
 public:
  void begin(std::string_view source) override { began = source == "synthetic.hipo"; }
  void write(const Event&, std::span<const DihadronCandidate> candidates) override {
    ++writes; total_candidates += candidates.size();
  }
  void finish() override { finished = true; }
  void abort() noexcept override { aborted = true; }
  bool began{}, finished{}, aborted{};
  std::size_t writes{}, total_candidates{};
};
std::vector<Event> fixture() {
  return {
    {1, {pion(1, 211, .3), pion(2, 211, -.2), pion(3, -211, .4)}},
    {2, {pion(1, 211, .3), pion(2, -211, -.2)}},
    {3, {pion(1, 211, .3), pion(2, -211, -.2), pion(3, -211, .4)}}
  };
}
}

int main() {
  constexpr std::array channels{
    config::Channel::piplus_piplus,
    config::Channel::piminus_piminus,
    config::Channel::piplus_piminus};

  VectorSource source{fixture()};
  OddEventSelector selector;
  RecordingSink sink;
  const auto summary = process_events(
    source, selector, sink, ProcessingOptions{0, {channels.begin(), channels.end()}});

  check(summary.source_events == 3, "the source is advanced exactly once per event");
  check(summary.selected_events == 2, "selection is applied inside the event loop");
  check(summary.candidates_by_channel.at(config::Channel::piplus_piplus) == 1,
        "pi+pi+ candidates are counted");
  check(summary.candidates_by_channel.at(config::Channel::piminus_piminus) == 1,
        "pi-pi- candidates are counted");
  check(summary.candidates_by_channel.at(config::Channel::piplus_piminus) == 4,
        "mixed-charge candidates are counted");
  check(sink.began && sink.finished && !sink.aborted, "the output transaction commits");
  check(sink.writes == 2 && sink.total_candidates == 6,
        "one candidate batch is written per selected event");

  VectorSource limited_source{fixture()};
  RecordingSink limited_sink;
  const auto limited = process_events(
    limited_source, selector, limited_sink,
    ProcessingOptions{2, {channels.begin(), channels.end()}});
  check(limited.source_events == 2 && limited.selected_events == 1,
        "debug event limits stop the source loop");

  if (failures == 0) std::cout << "event-processor: PASS\n";
  return failures == 0 ? 0 : 1;
}
