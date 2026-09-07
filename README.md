# CLAS12 Dihadron Analysis

A compact C++ analysis suite for producing beam-spin asymmetries from CLAS12
RG-A data and Monte Carlo. Development is led by Gregory Matousek.

The implemented charged-channel vertical slice reads RG-A HIPO files once,
builds π+π− candidates directly into a fit-ready ROOT tree, simultaneously fits
all seven explicitly named beam-spin modulations, exports the full covariance,
and creates uniform ROOT PDF/PNG plots. A C++ ROOT GUI configures and launches
the pipeline locally or through Slurm, monitors jobs/logs/storage, and displays
the saved ROOT canvas. The fit can be rerun without rereading HIPO. Neutral-pion
background handling is sideband-only; sWeights are rejected.

```sh
cmake -S . -B build
cmake --build build
./build/clas12-analysis validate config/rga.yaml
```

The portable physics core and regression checks build without JLab software.
See [the π+π− run guide](docs/pippim.md) to enable the HIPO/ROOT adapter and run
from configured RG-A paths. See `docs/configuration.md` for the configuration
contract and `docs/architecture.md` for the pipeline design.

