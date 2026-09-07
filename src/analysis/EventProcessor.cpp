#include "clas12dihadrons/analysis/EventProcessor.hpp"
#include <stdexcept>

namespace clas12::dihadron::analysis {

ProcessingSummary process_events(EventSource& source,
                                 EventSelector& selector,
                                 CandidateSink& sink,
                                 const ProcessingOptions& options) {
  if (options.channels.empty())
    throw std::invalid_argument("event processing requires at least one channel");

  ProcessingSummary summary;
  sink.begin(source.description());
  try {
    Event event;
    while ((options.max_events == 0 ||
            summary.source_events < options.max_events) &&
           source.next(event)) {
      ++summary.source_events;

      if (!selector.accept(event)) {
        event = Event{};
        continue;
      }
      ++summary.selected_events;

      // The event particle collection is indexed once inside this call and all
      // requested charged channels are built before the source advances.
      const auto candidates = build_charged_candidates(event, options.channels);
      for (const auto& candidate : candidates)
        ++summary.candidates_by_channel[candidate.channel];

      sink.write(event, candidates);
      event = Event{};
    }
    sink.finish();
  } catch (...) {
    sink.abort();
    throw;
  }
  return summary;
}

}  // namespace clas12::dihadron::analysis
