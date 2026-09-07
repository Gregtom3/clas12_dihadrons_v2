# Event processing

`process_events` is the single source-driving event loop. For each source event
it:

1. advances the source once;
2. applies event and particle selection;
3. indexes selected particles and builds every requested channel in memory;
4. writes one candidate batch;
5. advances to the next source event.

A positive `max_events` implements the debug limit; zero is unlimited. Output is
transactional: a completed source calls `finish`, while an exception calls
`abort` so a partial ROOT shard cannot be mistaken for complete output.

The HIPO adapter implements `EventSource`. It owns `clas12root::HipoChain`,
QADB checks, helicity handling, reconstructed/MC loading, and conversion into the
small core `Event` type. The ROOT adapter implements `CandidateSink`. This
keeps JLab dependencies at the boundary while the actual loop and combinatorics
remain CI-testable.
