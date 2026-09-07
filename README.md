# CLAS12 Dihadron Analysis

A C++ analysis suite for producing beam-spin asymmetries from CLAS12 RG-A data
and Monte Carlo.

The initial scope covers five pion-pair channels containing at most one neutral
pion. The suite provides resumable processing, cached photon-identification
models, sideband-corrected neutral-pion asymmetries, and uniform ROOT plots.

## Start

```sh
cmake -S . -B build
cmake --build build
./build/clas12-analysis validate config/rga.yaml
```

Set `CLAS12_SCRATCH`, `CLAS12_OUTPUT`, and `CLAS12_LOGS` for the current
environment. See `docs/configuration.md` for the analysis contract and
`docs/architecture.md` for the pipeline design.

Development is led by Gregory Matousek.
