# Configuration

The checked-in `config/rga.yaml` is the initial analysis contract. Validate it
before discovering files or submitting work:

```sh
clas12-analysis validate config/rga.yaml
```

Create a provenance record at the start of a run:

```sh
CLAS12_GIT_COMMIT="$(git rev-parse HEAD)" \
  clas12-analysis manifest config/rga.yaml run/manifest.json
```

Set `CLAS12_SCRATCH`, `CLAS12_OUTPUT`, and `CLAS12_LOGS` for the current
JLab account. No username is stored in source code.

Debug mode requires positive `max_files` and `max_events_per_file`. Production
mode interprets zero limits as unlimited. The scheduler may be `local` for
development or `slurm` at JLab.

The neutral-pion background method is deliberately restricted to `sideband`.
A configuration requesting sWeights is rejected.
