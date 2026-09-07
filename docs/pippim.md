# π+π− vertical slice

`clas12-rga-pippim` is the concrete RG-A path from HIPO files to the seven
integrated beam-spin amplitudes. It reads every HIPO event once, immediately
builds all accepted π+π− pairs, and writes a compact fit-ready ROOT tree. There
is no intermediate particle tree and no physical merge stage.

## JLab build

Load the standard CLAS12 software environment so ROOT and clas12root are on the
library path, then configure the adapter:

```sh
cmake -S . -B build \
  -DCLAS12_ENABLE_JLAB=ON \
  -DCLAS12ROOT_INCLUDE_DIR="$CLAS12ROOT/Clas12Root"
cmake --build build -j
```

If the clas12root library is not named `Clas12Root` at the site, pass the full
link item with `-DCLAS12ROOT_LIBRARIES=...`.

Set `CLAS12_RCDB_ROOT` to a local `rcdb.root` snapshot when the standard JLab
environment does not already configure the RCDB connection. The path is never
hard-coded into the analysis.

## GUI

The JLab build also produces `clas12-gui`:

```sh
./build/clas12-gui
```

The Pipeline tab loads dataset choices from the selected YAML file and exposes
the pipeline executable, output directory, debug file/event limits, local or
Slurm execution, and separate run/build/fit actions. The Jobs & Logs tab tails
the active log, displays the local process or the user's live `squeue`, and the
status bar reports available storage. The ROOT Results tab embeds the saved
`c_asymmetry` canvas. A fit can therefore be adjusted or repeated without
rereading any HIPO files.

For Slurm, the GUI writes a reproducible `gui-submit.sh` beside the outputs and
submits it with the account and partition shown in the form. It never embeds a
username or private filesystem path.

## Configure HIPO locations

Edit only `source_globs` in `config/rga.yaml` if the production paths have
moved. Globs and `${ENVIRONMENT_VARIABLE}` placeholders are accepted. In debug
mode `limits.max_files` and `limits.max_events_per_file` bound the run. Set
either value to `0` on the command line for no corresponding limit.

## Run

```sh
./build/clas12-rga-pippim run config/rga.yaml \
  Fall2018_RGA_inbending results/fall18-inbending 2 100000
```

The output directory contains:

- the compact candidate ROOT file;
- `piplus_piminus_asymmetry.root`, including the estimates and covariance;
- a golden-check-compatible JSON snapshot;
- matching PDF and PNG ROOT canvases.

For production, omit the last two debug overrides after setting both limits to
zero in the configuration. `build` and `fit` subcommands are also available so
a fit can be repeated without rereading HIPO.

The charged channel uses no background subtraction. Neutral channels will use
sidebands only; sWeights are not part of the suite.

## Selection contract

The `rga_baseline_v1` profile applies QADB asymmetry quality, corrected helicity,
the RG-A run polarization, the legacy forward-electron status/angle, vertex,
PCAL edge, sampling-fraction and momentum requirements, charged-pion angle,
status, χ² PID and electron-vertex matching, DIS `Q² >= 1 GeV²` and `y <= 0.8`,
then the legacy π+π− cuts `z < 0.95`, `xF1 > 0`, `xF2 > 0`, `Mx > 1.5 GeV`,
and both pion momenta above `1.25 GeV`.

Before publication, a maintainer must run the same pinned RG-A sample through
the legacy and V2 paths and commit the resulting V2 JSON as the real golden
fixture. The existing synthetic fixture tests the gate itself, not detector
agreement.

