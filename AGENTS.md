# Contributor guidance

Keep the operator workflow simple and the physics behavior explicit.

- Production code is C++20.
- ROOT is the canonical plotting and fit-result layer.
- Notebooks may explore results but must not contain authoritative logic.
- Do not infer particle channels from filenames.
- Do not use positional labels for physical quantities.
- Do not embed usernames, absolute JLab paths, cut values, datasets, model
  thresholds, or SLURM resources in source code.
- Neutral-pion background correction uses sidebands; do not introduce sWeights.
- Read each HIPO once per production configuration.
- New optimizations must pass the golden-analysis comparison.
- Tests must be deterministic and small enough to run without JLab data.
