# PADMe - PAreto Detection Method

An exact/approximate solver for **bi-objective mixed-integer linear programs (BOMILPs)**, implementing the PADMe algorithm (ideal-point-checking variant): each "slice" obtained when integer variables are fixed is a **bi-objective linear program (BOLP)** solved via dichotomic search, and cutting planes derived from previously found solutions are used to skip provably-dominated slices without solving them.

**Authors:** Lavinia Amorosi, Marianna De Santis

## What's in this repository

| File | Role |
|---|---|
| `main.cpp` | The algorithm's driver: reads an instance, runs the main branch-and-bound loop over integer fixings, writes the Pareto front to a results file. |
| `Header.h` | Instance parsing, the results tree (insertion, in-order traversal), and assorted utility functions. |
| `GurobiMPS.h` | Gurobi model construction/solving for the MILP subproblems (`updateMPS_and_solve`) and the ideal-point LP (`computeIdealPoint2`), plus the persistent `IterState` used to reuse Gurobi models (and warm starts) across iterations instead of rebuilding from scratch every time. |
| `dichotomic_search.hpp` | A solver-agnostic, header-only bi-objective dichotomic search algorithm (finds the supported non-dominated extreme points of a bi-objective *linear* problem via a weighted-sum oracle). Not specific to this project — usable standalone against any weighted-sum-solvable oracle. |
| `BolpDichotomic.h` | Wires `dichotomic_search.hpp` up to Gurobi to solve each BOLP slice. This replaces an earlier design that wrote `.vlp` files to disk and shelled out to [BENSOLVE](http://bensolve.org/); the dichotomic-search approach is mathematically equivalent for this problem class (the image of a polyhedron under a linear map is itself a polyhedron, so the non-dominated frontier of a BOLP slice is exactly its supported extreme points) and runs entirely in-process. |
| `Makefile` | Build configuration (see below). |

## Requirements

- A C++17 compiler (`g++` or `clang++`)
- [Gurobi](https://www.gurobi.com/) with a valid license (a free academic license is available) — this project uses Gurobi's C++ API for every LP/MILP solve
- `make`

## Building

The build needs to know where Gurobi is installed. Set the `GUROBI_HOME` environment variable to point at your installation — the directory that directly contains `include/` and `lib/`, e.g. `/opt/gurobi1103/linux64` on Linux or `/Library/gurobi1103/macos_universal2` on macOS.

```bash
# one-off, for a single build:
make GUROBI_HOME=/path/to/your/gurobi/install

# or export it once per shell session:
export GUROBI_HOME=/path/to/your/gurobi/install
make
```

This produces an executable named `bomilp`.

Other Makefile targets:

```bash
make debug              # unoptimized build (-g -O0), for use with gdb/lldb
make run ARGS="1 1"     # build (if needed) and run in one step
make clean              # remove build artifacts
make print-GRB_LIBS     # debugging helper: print any Makefile variable, e.g. to check what got auto-detected
```

**A note on the Gurobi library version:** the Makefile tries to auto-detect the versioned shared library (e.g. `libgurobi110.so`) under `$(GUROBI_HOME)/lib` so you don't have to hardcode a version number. If it can't find one, it fails with a clear error message rather than a wall of missing-header errors — at which point run `ls $GUROBI_HOME/lib` and pass the right one explicitly:

```bash
make GUROBI_HOME=/path/to/gurobi GRB_VERLIB=gurobi110
```

## Adapting the build to your own machine

If you're setting this up fresh, the two things you're most likely to need to change are:

1. **`GUROBI_HOME`** — as above. This is intentionally *not* hardcoded anywhere in the Makefile; you supply it via the environment or the command line every time (or add `export GUROBI_HOME=...` to your own shell profile, e.g. `~/.bashrc`, so you don't have to repeat it).
2. **The instances directory** — see below.

Everything else in the Makefile (compiler flags, platform detection for Linux vs. macOS, `rpath` handling so the built binary finds Gurobi's shared library at runtime) should work unmodified.

## Instance file format

Instances are plain-text `.dat` files, read by `readInstance()` in `GurobiMPS.h`. Layout (whitespace-separated on each line):

```
m                          # number of constraint rows
ncont                      # number of continuous variables
nint                       # number of integer/binary variables
<f1 coefficients, ncont values>    # objective 1, continuous part
<f1 coefficients, nint values>     # objective 1, integer part
<f2 coefficients, ncont values>    # objective 2, continuous part
<f2 coefficients, nint values>     # objective 2, integer part
<constraint row 1, ncont values>   # constraint matrix, continuous columns
...
<constraint row m, ncont values>
<constraint row 1, nint values>    # constraint matrix, integer columns
...
<constraint row m, nint values>
<right-hand side, m values>
<row types, m values>              # e.g. 0 = equality, 1 = <=
<lower bounds, ncont values>       # optional; defaults to 0 if absent
<upper bounds, ncont values>       # optional; defaults to +inf if absent
```

## Usage

```bash
./bomilp <instance_name> <it_index> [budget_fraction] [mip_gap] [verbose]
```

| Argument | Required? | Default | Meaning |
|---|---|---|---|
| `instance_name` | yes | — | Instance file name, without the `.dat` extension. Resolved relative to the instances directory (see below). |
| `it_index` | yes | — | Selects a small working file, `it<it_index>.txt`, used to carry a bit of state between separate invocations. If it doesn't exist yet in the instances directory, the run just starts fresh — you don't need to create it manually first. |
| `budget_fraction` | no | `1.0` | Caps how many LP solves dichotomic search may spend per BOLP slice, as a fraction of that slice's number of continuous variables. `1.0` = unlimited (exact frontier). Values in `(0, 1)` trade exactness for speed — see the warning in `BolpDichotomic.h`. |
| `mip_gap` | no | `1e-4` | Gurobi's `MIPGap` for the MILP subproblems (Gurobi's own default is `1e-4`). Raising it trades exactness for speed — see the warning in `main.cpp` where it's parsed. |
| `verbose` | no | `1` | `0` = silent, anything else = print progress messages. |

Positional arguments — to set a later one you need to supply the ones before it (use the defaults shown above as placeholders).

Examples:

```bash
# exact run, default MIPGap, verbose progress messages:
./bomilp my_instance 1

# silent run, everything else at defaults:
./bomilp my_instance 1 1.0 1e-4 0

# approximate BOLP frontier (20% budget) + looser MIPGap:
./bomilp my_instance 1 0.2 1e-3
```

### Where instance files are read from / results written to

By default, `bomilp` looks for `<instance_name>.dat` (and writes all working/output files) in the **current working directory**. Override this with an environment variable:

```bash
export BOMILP_INSTANCES_DIR=/path/to/your/instances
./bomilp my_instance 1
```

### Output

Results are written to `<instance_name>_solution_ideal_point.txt` in the instances directory:

- One line per non-dominated point/segment found, formatted as MATLAB/Octave `plot(...)` calls (so the file can be run directly through MATLAB or Octave to visualize the Pareto front), e.g.:
  ```
  plot([f1_a,f1_b],[f2_a,f2_b],'-ro');
  ```
- Followed by a summary block:
  ```
  <wall-clock time> sec
  <N> solved MILPs
  <N> solved BOLPs
  <N> #MILPs-Dichotomic      (total LP solves performed by dichotomic search, across all BOLPs)
  <t> sec MILP solves
  <t> sec BOLP/dichotomic solves
  <t> sec ideal-point LP solves
  <t> sec other (I/O, bookkeeping)
  ```
  The last four lines are a wall-clock time breakdown by phase, useful for identifying where time is actually going before tuning anything (in practice, MILP branch-and-bound tends to dominate — see the tuning notes below).

## Performance tuning notes

A few things worth knowing if you're adapting this to larger instances:

- **`mip_gap` is the highest-leverage speed lever** in most cases, since MILP branch-and-bound typically dominates total runtime. Note the correctness caveat, though: the algorithm's cut/discard logic assumes each MILP's reported objective value is the *exact* optimum, so a looser gap can (rarely, near the margin) cause it to silently miss a non-dominated point. Increase it incrementally and compare the resulting Pareto front against an exact run on a small instance before trusting a looser value at scale.
- **`budget_fraction`** only affects the BOLP/dichotomic-search side, which is typically a small fraction of total runtime — check your own timing breakdown before assuming this will help.
- **No-good constraints** (added once per MILP iteration to exclude the previously found integer solution) are marked as Gurobi *lazy constraints* (`GurobiMPS.h`), since they never bind at fractional LP-relaxation points — only at the exact integer point they were built to exclude. This keeps them out of the LP relaxation at most B&B nodes rather than bloating every node's solve, with no effect on correctness.
- **Threads**: solves currently run single-threaded (`GRB_IntParam_Threads = 1`, set in `main.cpp` right after `itState`/`bolpState` are constructed) — this was set for reproducible, thread-count-independent timing comparisons while tuning the parameters above. Remove those three lines (or set them to `0`, Gurobi's "use all available cores" default) to parallelize solves.

## A note on exactness

With default settings (`budget_fraction=1.0`, `mip_gap=1e-4`), the algorithm returns the exact set of supported non-dominated points (up to Gurobi's own default numerical tolerances). Both `budget_fraction < 1.0` and `mip_gap` values looser than Gurobi's default trade some of that exactness for speed — see the warnings in `BolpDichotomic.h` and `main.cpp` respectively for exactly what can go wrong and why.
