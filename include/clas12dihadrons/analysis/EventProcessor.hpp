#pragma once
#include "clas12dihadrons/analysis/CandidateBuilder.hpp"
#include <cstddef>
#include <map>
#include <span>
#include <string_view>
#include <vector>

namespace clas12::dihadron::analysis {

class EventSource {
 public:
  virtual ~EventSource() = default;
  virtual bool next(Event& event) = 0;
  virtual std::string_view description() const = 0;
};

class EventSelector {
 public:
  virtual ~EventSelector() = default;
  virtual bool accept(Event& event) = 0;
};

class CandidateSink {
 public:
  virtual ~CandidateSink() = default;
  virtual void begin(std::string_view source) = 0;
  virtual void write(const Event& event,
                     std::span<const DihadronCandidate> candidates) = 0;
  virtual void finish() = 0;
  virtual void abort() noexcept = 0;
};

struct ProcessingOptions {
  // Zero means unlimited. A debug configuration supplies a positive value.
  std::size_t max_events{};
  std::vector<config::Channel> channels;
};

struct ProcessingSummary {
  std::size_t source_events{};
  std::size_t selected_events{};
  std::map<config::Channel, std::size_t> candidates_by_channel;
};

// Owns the one-and-only event loop. EventSource will be implemented by the HIPO
// adapter and CandidateSink by the ROOT shard writer.
ProcessingSummary process_events(EventSource& source,
                                 EventSelector& selector,
                                 CandidateSink& sink,
                                 const ProcessingOptions& options);

}  // namespace clas12::dihadron::analysis
