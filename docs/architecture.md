# Architecture

The suite exposes one resumable analysis run. Internally it:

1. discovers and validates RG-A inputs;
2. reuses compatible inbending/outbending photon models, or trains them from a
   compact Monte Carlo sample;
3. reads each HIPO once, applies selections, infers photon scores, and builds all
   five channels in memory;
4. fits seven beam-spin modulations, using sideband correction for channels with
   one neutral pion;
5. produces uniform ROOT plots and a provenance manifest.

These nodes exist for restart and debugging. The operator does not manually
coordinate them.

## Scope

Supported channels are piplus_piplus, piminus_piminus, piplus_piminus,
piplus_pi0, and piminus_pi0. pi0_pi0 and sWeight analysis are intentionally out
of scope.

Production output is sharded by source HIPO. Consumers use a manifest and ROOT
chaining; a physical merge is an optional compatibility/export product.

## Golden analysis gate

Behavior-changing work compares a candidate snapshot with a reviewed reference.
The comparison covers cut-flow and candidate counts, seven amplitudes, seven
statistical uncertainties, the 7x7 covariance matrix, and dataset/channel/bin/
background-method identity.

The checked-in fixtures demonstrate the schema and test the verifier. They are
not physics references. Reviewed snapshots must be generated from an agreed
legacy run and stored with input catalog, code revision, configuration, model
hashes, and sideband definition.
